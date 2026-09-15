#include "matching_engine/replay.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace matching_engine {

static Side parse_side(std::string_view str) {
    if (str == "BUY" || str == "Buy" || str == "0") return Side::Buy;
    return Side::Sell;
}

static OrderType parse_type(std::string_view str) {
    if (str == "MARKET" || str == "Market" || str == "1") return OrderType::Market;
    return OrderType::Limit;
}

static ExecutionPolicy parse_policy(std::string_view str) {
    if (str == "IOC" || str == "1") return ExecutionPolicy::IOC;
    if (str == "FOK" || str == "2") return ExecutionPolicy::FOK;
    return ExecutionPolicy::GTC;
}

static ReplayAction parse_action(std::string_view str) {
    if (str == "CANCEL" || str == "1") return ReplayAction::CANCEL;
    if (str == "MODIFY" || str == "2") return ReplayAction::MODIFY;
    return ReplayAction::NEW;
}

std::vector<ReplayCommand> ReplayEngine::load_csv(const std::string& filepath) {
    std::vector<ReplayCommand> commands;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open CSV file for replay: " << filepath << "\n";
        return commands;
    }

    std::string line;
    bool header_skipped = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!header_skipped && (line.find("sequence_id") != std::string::npos || line.find("action") != std::string::npos)) {
            header_skipped = true;
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ',')) {
            // Trim whitespace
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            tokens.push_back(token);
        }

        if (tokens.size() < 9) continue;

        ReplayCommand cmd;
        try {
            cmd.sequence_id = std::stoull(tokens[0]);
            cmd.timestamp = std::stoull(tokens[1]);
            cmd.order_id = std::stoull(tokens[2]);
            cmd.side = parse_side(tokens[3]);
            cmd.type = parse_type(tokens[4]);
            cmd.price = std::stoll(tokens[5]);
            cmd.quantity = std::stoull(tokens[6]);
            cmd.policy = parse_policy(tokens[7]);
            cmd.action = parse_action(tokens[8]);
            commands.push_back(cmd);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing CSV line: " << line << " (" << e.what() << ")\n";
        }
    }
    return commands;
}

bool ReplayEngine::save_csv(const std::string& filepath, const std::vector<ReplayCommand>& commands) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "sequence_id,timestamp,order_id,side,type,price,quantity,execution_policy,action\n";
    for (const auto& cmd : commands) {
        std::string action_str;
        switch (cmd.action) {
            case ReplayAction::NEW: action_str = "NEW"; break;
            case ReplayAction::CANCEL: action_str = "CANCEL"; break;
            case ReplayAction::MODIFY: action_str = "MODIFY"; break;
        }

        file << cmd.sequence_id << ","
             << cmd.timestamp << ","
             << cmd.order_id << ","
             << to_string(cmd.side) << ","
             << to_string(cmd.type) << ","
             << cmd.price << ","
             << cmd.quantity << ","
             << to_string(cmd.policy) << ","
             << action_str << "\n";
    }
    return true;
}

} // namespace matching_engine
