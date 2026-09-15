# Order Matching & Execution Rules

This document specifies the matching semantics and execution rules of the **High-Performance Matching Engine**.

---

## 1. Price-Time Priority

All matching logic strictly enforces **Price-Time Priority**:
1. **Price Priority**: A Buy order with a higher price has priority over a lower price. A Sell order with a lower price has priority over a higher price.
2. **Time Priority**: Among orders at the same price level, earlier submitted orders (lower `sequence_id`) have priority over later orders.

---

## 2. Execution Policies & Order Types

### Limit GTC (Good-Til-Cancelled)
- Matches executable opposite liquidity (`best_ask <= limit_price` for Buy; `best_bid >= limit_price` for Sell).
- Any remaining unfilled quantity is placed into the order book at `limit_price` with a new sequence ID.

### Limit IOC (Immediate-Or-Cancel)
- Matches available opposite liquidity up to `limit_price`.
- Any remaining unfilled quantity is immediately cancelled. It is **never** placed into the book.

### Limit FOK (Fill-Or-Kill)
- Performs a **non-mutating liquidity check** across opposite levels up to `limit_price`.
- If available opposite liquidity $\ge \text{order quantity}$, the order matches completely.
- If available opposite liquidity $<\text{order quantity}$, the order is cancelled immediately without executing partial fills or mutating the book.

### Market Orders
- Consumes available opposite-side liquidity starting from top-of-book until completely filled or book is exhausted.
- Any unfilled market order quantity is cancelled immediately. Market orders **never** rest in the book.

---

## 3. Order Management

### Cancellation
- Looked up in $O(1)$ time via `OrderIndex`.
- Unlinked from `PriceLevel` doubly-linked list in $O(1)$ time without disrupting remaining queue ordering.
- Cancellation of non-existent order generates an `OrderRejected` event with `reason="Order not found for cancellation"`.

### Modification
- **Price Modification**: Cancels existing order and re-submits at new price level with new sequence ID (loses priority).
- **Quantity Decrease**: Decreases `remaining_qty` in place without changing position in price level queue (retains priority).
- **Quantity Increase**: Unlinks from current position and re-appends to tail of queue with new sequence ID (loses priority).

---

## 4. Trade Execution Pricing

Trades are priced at the **resting order's price** (passive price):
- When an incoming Buy order matches a resting Ask order at 101.00, the trade price is 101.00 regardless of whether the aggressor's limit price was higher (e.g. 101.50).
