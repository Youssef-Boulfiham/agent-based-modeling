#include "../include/MessageLog.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>

MessageLog::MessageLog(const std::string& outputDir)
    : logFilePath(outputDir + "/textbox_history.jsonl") {
    // Ensure output directory exists
    std::filesystem::create_directories(outputDir);
    loadFromFile();
}

MessageLog::~MessageLog() {
    // All entries already written to file
}

std::string MessageLog::getCurrentTimestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void MessageLog::queueUserInput(const std::string& from, const std::string& to, const std::string& text,
                               glm::vec2 pos, const std::string& domain, const std::string& action) {
    // User input: highest priority (add to front)
    LogTask task{LogTask::USER_INPUT, from, to, text, pos, domain, action};
    std::queue<LogTask> newQueue;
    newQueue.push(task);
    while (!taskQueue.empty()) {
        newQueue.push(taskQueue.front());
        taskQueue.pop();
    }
    taskQueue = newQueue;
}

void MessageLog::queueAgentChatter(const std::string& from, const std::string& to, const std::string& text,
                                 glm::vec2 pos, const std::string& domain, const std::string& action) {
    LogTask task{LogTask::AGENT_CHATTER, from, to, text, pos, domain, action};
    taskQueue.push(task);
}

void MessageLog::processQueue() {
    if (taskQueue.empty()) return;

    LogTask task = taskQueue.front();
    taskQueue.pop();

    LogEntry entry;
    entry.seq = nextSeq++;
    entry.timestamp = getCurrentTimestamp();
    entry.from = task.from;
    entry.to = task.to;
    entry.text = task.text;
    entry.state.posX = task.pos.x;
    entry.state.posY = task.pos.y;
    entry.state.domain = task.domain;
    entry.state.action = task.action;

    history.push_back(entry);
    writeToFile(entry);
    trimHistory();
}

void MessageLog::writeToFile(const LogEntry& entry) {
    std::ofstream file(logFilePath, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open message log file: " << logFilePath << "\n";
        return;
    }

    // JSON format (one line per entry)
    file << "{"
         << "\"timestamp\":\"" << entry.timestamp << "\","
         << "\"from\":\"" << entry.from << "\","
         << "\"to\":\"" << entry.to << "\","
         << "\"text\":\"" << entry.text << "\","
         << "\"state\":{"
         << "\"pos\":{\"x\":" << entry.state.posX << ",\"y\":" << entry.state.posY << "},"
         << "\"domain\":\"" << entry.state.domain << "\","
         << "\"action\":\"" << entry.state.action << "\""
         << "}"
         << "}\n";

    file.close();
}

void MessageLog::trimHistory() {
    if (history.size() > MAX_ENTRIES) {
        history.erase(history.begin(), history.begin() + (history.size() - MAX_ENTRIES));
    }
}

void MessageLog::loadFromFile() {
    std::ifstream file(logFilePath);
    if (!file.is_open()) {
        // File doesn't exist yet, that's fine
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Simple JSON parsing (fragile but works for this format)
        LogEntry entry;
        entry.seq = nextSeq++;

        // Extract timestamp
        size_t ts_pos = line.find("\"timestamp\":\"") + 13;
        entry.timestamp = line.substr(ts_pos, 8);

        // Extract from
        size_t from_pos = line.find("\"from\":\"") + 8;
        size_t from_end = line.find("\"", from_pos);
        entry.from = line.substr(from_pos, from_end - from_pos);

        // Extract to
        size_t to_pos = line.find("\"to\":\"") + 6;
        size_t to_end = line.find("\"", to_pos);
        entry.to = line.substr(to_pos, to_end - to_pos);

        // Extract text
        size_t text_pos = line.find("\"text\":\"") + 8;
        size_t text_end = line.find("\"", text_pos);
        entry.text = line.substr(text_pos, text_end - text_pos);

        // Extract state fields
        size_t x_pos = line.find("\"x\":") + 4;
        entry.state.posX = std::stof(line.substr(x_pos, 10));

        size_t y_pos = line.find("\"y\":") + 4;
        entry.state.posY = std::stof(line.substr(y_pos, 10));

        size_t domain_pos = line.find("\"domain\":\"") + 10;
        size_t domain_end = line.find("\"", domain_pos);
        entry.state.domain = line.substr(domain_pos, domain_end - domain_pos);

        size_t action_pos = line.find("\"action\":\"") + 10;
        size_t action_end = line.find("\"", action_pos);
        entry.state.action = line.substr(action_pos, action_end - action_pos);

        history.push_back(entry);
    }

    file.close();
}

void MessageLog::clear() {
    history.clear();
    // Don't delete file; just clear in-memory
}
