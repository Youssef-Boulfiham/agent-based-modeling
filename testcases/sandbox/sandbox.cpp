#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <iomanip>
#include <string>
#include <ctime>

// ============================================================================
// Simplified Data Structures
// ============================================================================

struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }

    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalize() const {
        float len = length();
        return len > 0 ? Vec2(x / len, y / len) : Vec2(0, 0);
    }
    float distance(const Vec2& v) const { return (*this - v).length(); }
};

// ============================================================================
// Sandbox Agent - Simplified for testing
// ============================================================================

class SandboxAgent {
private:
    int id;
    Vec2 position;
    Vec2 velocity;
    std::string activity;
    std::string status;
    float speed;
    Vec2 targetPosition;
    bool hasTarget;
    int stepCount;
    float timeAlive;

public:
    SandboxAgent(int id, Vec2 startPos)
        : id(id), position(startPos), velocity(0, 0), activity("idle"),
          status("active"), speed(2.0f), targetPosition(0, 0),
          hasTarget(false), stepCount(0), timeAlive(0) {}

    void step(float deltaTime) {
        stepCount++;
        timeAlive += deltaTime;

        if (hasTarget) {
            Vec2 direction = (targetPosition - position).normalize();
            velocity = direction * speed;
            position = position + velocity * deltaTime;

            // Check if reached target
            if (position.distance(targetPosition) < 1.0f) {
                hasTarget = false;
                velocity = Vec2(0, 0);
                activity = "idle";
                status = "waiting";
            }
        }
    }

    void setTarget(Vec2 target) {
        targetPosition = target;
        hasTarget = true;
        activity = "moving";
        status = "active";
    }

    void idle() {
        hasTarget = false;
        velocity = Vec2(0, 0);
        activity = "idle";
        status = "waiting";
    }

    // Getters
    int getId() const { return id; }
    Vec2 getPosition() const { return position; }
    const std::string& getActivity() const { return activity; }
    const std::string& getStatus() const { return status; }
    int getStepCount() const { return stepCount; }
    float getTimeAlive() const { return timeAlive; }
    float getDistanceTraveled() const { return stepCount * speed * 0.016f; } // rough estimate
    bool hasActiveTarget() const { return hasTarget; }
};

// ============================================================================
// Sandbox Environment - Simplified for testing
// ============================================================================

class SandboxEnv {
private:
    float width, height;
    std::vector<SandboxAgent*> agents;
    int frameCount;
    float simulationTime;
    std::mt19937 rng;
    std::ofstream logFile;

public:
    SandboxEnv(float w, float h, int maxAgents)
        : width(w), height(h), frameCount(0), simulationTime(0),
          rng(std::random_device{}()) {

        // Create agents
        std::uniform_real_distribution<float> distX(0, width);
        std::uniform_real_distribution<float> distY(0, height);

        for (int i = 0; i < maxAgents; i++) {
            Vec2 startPos(distX(rng), distY(rng));
            agents.push_back(new SandboxAgent(i, startPos));
        }
    }

    ~SandboxEnv() {
        if (logFile.is_open()) logFile.close();
        for (auto agent : agents) delete agent;
    }

    void initialize(const std::string& outputFile) {
        logFile.open(outputFile, std::ios::app);
        if (logFile.is_open()) {
            logFile << "=== Sandbox Simulation Started ===" << std::endl;
            logFile << "Environment: " << width << " x " << height << std::endl;
            logFile << "Agents: " << agents.size() << std::endl;
            logFile << "---" << std::endl;
        }
    }

    void step(float deltaTime) {
        frameCount++;
        simulationTime += deltaTime;

        // Update all agents
        for (auto agent : agents) {
            agent->step(deltaTime);

            // Simple behavior: assign random target sometimes
            if (!agent->hasActiveTarget() && (frameCount % 30 == 0)) {
                std::uniform_real_distribution<float> distX(0, width);
                std::uniform_real_distribution<float> distY(0, height);
                Vec2 newTarget(distX(rng), distY(rng));
                agent->setTarget(newTarget);
            }
        }
    }

    void logStats() {
        if (logFile.is_open()) {
            logFile << "Frame " << frameCount << " (t=" << std::fixed
                    << std::setprecision(2) << simulationTime << "s)" << std::endl;

            for (auto agent : agents) {
                logFile << "  Agent " << agent->getId() << ": pos=("
                        << agent->getPosition().x << ", " << agent->getPosition().y << ") "
                        << "activity=" << agent->getActivity() << " "
                        << "status=" << agent->getStatus() << std::endl;
            }
            logFile.flush();
        }
    }

    void run(float duration, float deltaTime = 0.016f) {
        int totalFrames = static_cast<int>(duration / deltaTime);

        std::cout << "Running sandbox simulation for " << duration << " seconds..." << std::endl;

        for (int i = 0; i < totalFrames; i++) {
            step(deltaTime);

            // Log every 60 frames (~1 second at 60 FPS)
            if (i % 60 == 0) {
                logStats();
            }
        }

        logFinalStats();
    }

    void logFinalStats() {
        if (logFile.is_open()) {
            logFile << "---" << std::endl;
            logFile << "=== Simulation Complete ===" << std::endl;
            logFile << "Total frames: " << frameCount << std::endl;
            logFile << "Total time: " << std::fixed << std::setprecision(2)
                    << simulationTime << " seconds" << std::endl;
            logFile << "Agents: " << agents.size() << std::endl;
            logFile << std::endl;
        }
    }

    int getFrameCount() const { return frameCount; }
    float getSimulationTime() const { return simulationTime; }
    const std::vector<SandboxAgent*>& getAgents() const { return agents; }
};

// ============================================================================
// Main - Test Harness
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "ABM Sandbox - Proof of Concept Testing" << std::endl;
    std::cout << "======================================" << std::endl;

    // Create output file with timestamp
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char filename[100];
    strftime(filename, sizeof(filename), "sandbox_log_%Y%m%d_%H%M%S.txt", timeinfo);

    // Ensure results directory exists
    system("mkdir -p results 2>/dev/null || true");

    // Create environment
    SandboxEnv env(100.0f, 100.0f, 10); // 100x100 world, 10 agents
    env.initialize(std::string("results/") + filename);

    std::cout << "Environment: 100x100, 10 agents" << std::endl;
    std::cout << "Output: " << filename << std::endl;

    // Run simulation
    env.run(5.0f); // 5 seconds at default 60 FPS

    std::cout << "Simulation complete." << std::endl;
    std::cout << "Frames: " << env.getFrameCount() << std::endl;
    std::cout << "Time: " << env.getSimulationTime() << " seconds" << std::endl;

    return 0;
}
