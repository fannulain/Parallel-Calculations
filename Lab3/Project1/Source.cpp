#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <mutex>
#include <atomic>
#include <string>
#include <iomanip>
#include "ThreadPool.h"

const std::string COLOR_RESET = "\033[0m";
const std::string COLOR_GREEN = "\033[32m";
const std::string COLOR_CYAN = "\033[36m";
const std::string COLOR_YELLOW = "\033[33m";
const std::string COLOR_MAGENTA = "\033[35m";

std::mutex consoleMutex;

void safePrint(const std::string& msg) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    std::cout << msg << "\n";
}

void simulateTask(int taskId, int durationSeconds) {
    safePrint(COLOR_GREEN + "[WORKER]" + COLOR_RESET + " Task " + std::to_string(taskId) + " STARTED.Time: " + std::to_string(durationSeconds) + "s.");

    std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));

    safePrint(COLOR_GREEN + "[WORKER]" + COLOR_RESET + " Task " + std::to_string(taskId) + " COMPLETED.");
}

void taskGenerator(int generatorId, ThreadPool& pool, std::atomic<int>& globalTaskId, std::atomic<bool>& isTestRunning) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distDuration(8, 14);
    std::uniform_int_distribution<> distDelay(2, 5);

    while (isTestRunning.load()) {
        int duration = distDuration(gen);
        int taskId = globalTaskId++;

        pool.addTask(duration, simulateTask, taskId, duration);

        safePrint(COLOR_CYAN + "[GENERATOR " + std::to_string(generatorId) + "]" + COLOR_RESET + " generated a task " + std::to_string(taskId) +
            " (Time: " + std::to_string(duration) + "s).");

        std::this_thread::sleep_for(std::chrono::seconds(distDelay(gen)));
    }
}

void testSolution(int testDurationSec, int workerThreads, int generatorThreads) {
    safePrint(COLOR_MAGENTA + "=== STARTING A TEST ===" + COLOR_RESET);
    safePrint(COLOR_MAGENTA + "Test duration: " + std::to_string(testDurationSec) + " seconds." + COLOR_RESET);

    ThreadPool pool(workerThreads);
    std::atomic<int> globalTaskId{ 0 };
    std::atomic<bool> isTestRunning{ true };

    std::vector<std::thread> generators;
    for (int i = 0; i < generatorThreads; i++) {
        generators.emplace_back(taskGenerator, i + 1, std::ref(pool), std::ref(globalTaskId), std::ref(isTestRunning));
    }

    std::vector<long long> cumulativeQueueSizes(workerThreads, 0);
    int sampleCount = 0;

    auto startTestTime = std::chrono::steady_clock::now();
    auto endTestTime = startTestTime + std::chrono::seconds(testDurationSec);

    while (std::chrono::steady_clock::now() < endTestTime) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::vector<int> currentSizes = pool.getQueueSizes();
        std::string stats = COLOR_YELLOW + "[PROFILER]" + COLOR_RESET + " Queues size: ";
        for (int i = 0; i < currentSizes.size(); i++) {
            stats += "Q" + std::to_string(i + 1) + ":" + std::to_string(currentSizes[i]) + " ";
            cumulativeQueueSizes[i] += currentSizes[i];
        }
        safePrint(stats);
        sampleCount++;
    }

    safePrint(COLOR_MAGENTA + "\n=== ENDING A TEST ===" + COLOR_RESET);
    isTestRunning = false;

    pool.stop(false);

    for (auto& gen : generators) {
        if (gen.joinable()) gen.join();
    }

    auto actualTestDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTestTime).count();

    safePrint(COLOR_MAGENTA + "\n=== TEST RESULTS ===" + COLOR_RESET);

    int totalThreads = workerThreads + generatorThreads + 1;
    safePrint("1. Total number of threads: " + std::to_string(totalThreads));

    double avgTaskTimeMs = pool.getAverageTaskExecutionTimeMs();
    safePrint("2. Avarage time for a task: " + std::to_string(avgTaskTimeMs / 1000.0) + " s.");

    safePrint("3. Avarage size of each queue:");
    for (int i = 0; i < cumulativeQueueSizes.size(); i++) {
        double avgLength = (sampleCount > 0) ? static_cast<double>(cumulativeQueueSizes[i]) / sampleCount : 0.0;
        safePrint("   - Queue " + std::to_string(i + 1) + ": " + std::to_string(avgLength) + " tasks");
    }

    int tasksCompleted = pool.getTasksCompleted();
    safePrint("4. Tasks statistic:");
    safePrint("   - Tasks submited: " + std::to_string(pool.getTasksSubmitted()));
    safePrint("   - Tasks completed: " + std::to_string(tasksCompleted));

    double avgWaitTimePerWorkerMs = pool.getAverageWaitTimePerWorkerMs();
    safePrint("5. Average time a thread spends in the waiting state: " +
        std::to_string(avgWaitTimePerWorkerMs / 1000.0) + " s.");
}

int main() {
    const int WORKER_THREADS = 6;
    const int GENERATOR_THREADS = 3;
    const int TEST_DURATION_SEC = 30;

    testSolution(TEST_DURATION_SEC, WORKER_THREADS, GENERATOR_THREADS);

    return 0;
}