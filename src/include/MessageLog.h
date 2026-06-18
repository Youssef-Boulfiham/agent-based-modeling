#ifndef MESSAGELOG_H
#define MESSAGELOG_H

#include <string>
#include <vector>
#include <queue>
#include <glm/glm.hpp>
#include <ctime>

// Message entry: timestamp + from + to + text + agent state
struct LogEntry {
    unsigned long seq = 0;   // stable monotonic id; survives front-trim (see trimHistory)
    std::string timestamp;
    std::string from;
    std::string to;
    std::string text;

    struct State {
        float posX, posY;
        std::string domain;
        std::string action;
    } state;
};

// Task in priority queue (user input has highest priority)
struct LogTask {
    enum Type { USER_INPUT, AGENT_CHATTER } type;
    std::string from;
    std::string to;
    std::string text;
    glm::vec2 pos;
    std::string domain;
    std::string action;
};

class MessageLog {
private:
    std::vector<LogEntry> history;
    std::queue<LogTask> taskQueue;
    std::string logFilePath;
    unsigned long nextSeq = 0;   // assigns LogEntry::seq

    static constexpr int MAX_ENTRIES = 100;

    std::string getCurrentTimestamp() const;
    void trimHistory();
    void writeToFile(const LogEntry& entry);

public:
    MessageLog(const std::string& outputDir = "data/logs");
    ~MessageLog();

    // Add message to priority queue
    void queueUserInput(const std::string& from, const std::string& to, const std::string& text,
                       glm::vec2 pos, const std::string& domain, const std::string& action);
    void queueAgentChatter(const std::string& from, const std::string& to, const std::string& text,
                          glm::vec2 pos, const std::string& domain, const std::string& action);

    // Process queue (called once per frame)
    void processQueue();

    // Check if queue has pending tasks
    bool hasQueuedTasks() const { return !taskQueue.empty(); }

    // Access history
    const std::vector<LogEntry>& getHistory() const { return history; }
    int getEntryCount() const { return history.size(); }

    // Load from file
    void loadFromFile();

    // Clear history
    void clear();
};

#endif // MESSAGELOG_H
