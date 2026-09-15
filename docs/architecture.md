# Technical Architecture Specification

This document details the software architecture, memory layout, ownership semantics, and concurrency model of the **High-Performance Matching Engine**.

![System Architecture Diagram](images/architecture_diagram_whiteboard.png)

---

## 1. Component Overview

```
                      +----------------------+
                      |    Order Gateway     |
                      +----------+-----------+
                                 |
                                 v
                      +----------------------+
                      |   Matching Engine    |
                      +----------+-----------+
                                 |
                  +--------------+--------------+
                  |                             |
                  v                             v
         +------------------+          +------------------+
         |    Order Book    |          |  Event Publisher |
         | (Strategy A & B) |          |   (Event Sink)   |
         +------------------+          +------------------+
```

### Key Responsibilities
- **Order Model (`Order`)**: Compact struct storing order metadata and intrusive doubly-linked pointers (`prev`, `next`).
- **Object Pool (`OrderPool`)**: Pre-allocated slab memory allocator for `Order` nodes.
- **Order Index (`OrderIndex`)**: O(1) hash table mapping `OrderId` to raw `Order*` pointers for immediate cancellation and modification.
- **Price Level (`PriceLevel`)**: Intrusive FIFO doubly-linked list of `Order` nodes at a specific price level.
- **Order Book (`MapOrderBook`, `FlatOrderBook`)**: Container for Bid and Ask price levels.
- **Matching Engine (`MatchingEngine`)**: Single-writer deterministic matching core handling order execution policies, sequence counters, and trade matching logic.
- **Event Sink (`IEventSink`)**: Abstract interface for receiving trade executions, fills, cancellations, and order book deltas.

---

## 2. Order Lifecycle

```
[Incoming Event] ──> [Order Accepted] ──> [FOK Check]
                                               │
                       ┌───────────────────────┴───────────────────────┐
                       ▼                                               ▼
               [Executable?]                                    [Not Executable]
                       │                                               │
        ┌──────────────┴──────────────┐                         [Order Cancelled]
        ▼                             ▼
[Match Trade Loop]            [Remaining Qty?]
        │                             │
 [Emit Trade Events]           ┌──────┴──────┐
                               ▼             ▼
                            [GTC]         [IOC/FOK]
                               │             │
                          [Rest Book]   [Cancel Rem.]
```

---

## 3. Data Structures & Memory Strategy

### Zero-Allocation Hot Path
Dynamic heap allocation (`malloc`/`new`) on the hot path causes latency variance due to lock contention and heap fragmentation.
- `OrderPool` allocates raw contiguous slabs of `Order` structs at engine initialization.
- When an order is submitted, `allocate()` pops an `Order*` handle off a lock-free free-list in $O(1)$ time.
- When an order fills or cancels, `deallocate()` pushes the handle back onto the free-list in $O(1)$ time.

### Intrusive Doubly-Linked Queues
`PriceLevel` avoids standard container node allocations (`std::list` node allocations) by embedding `prev` and `next` pointers directly inside the `Order` struct:
- **Insertion**: $O(1)$ append to `tail_`.
- **Cancellation**: $O(1)$ unlinking of `prev` and `next` pointers given an `Order*` handle from `OrderIndex`.

---

## 4. Dual Strategy Performance Comparison

- **Strategy A (`MapOrderBook`)**: Standard `std::map<Price, PriceLevel>` providing logarithmic search and naturally ordered top-of-book lookup.
- **Strategy B (`FlatOrderBook`)**: Contiguous vector of `PriceLevel` structs sorted by price, optimized for cache-line locality in narrow tick spaces.

---

## 5. Concurrency & Sequencing Model

- **Single-Writer Core**: The matching engine executes order mutations on a single authoritative thread to guarantee deterministic price-time priority without lock overhead or race conditions.
- **Monotonic Sequence Numbers**: Every accepted order, trade execution, and cancellation receives a strictly increasing 64-bit sequence number (`sequence_id`).
