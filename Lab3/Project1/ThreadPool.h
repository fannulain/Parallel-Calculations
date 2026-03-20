#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <climits>
#include <chrono>
#include <functional>
#include "TaskQueue.h"
#include "TaskStruct.h"

//declaration

class ThreadPool
{
private:
	std::vector<std::unique_ptr<TaskQueue<Task>>> queues;
	std::vector<std::thread> workers;
	std::vector<std::unique_ptr<std::atomic<int>>> queueLoadTimes;

	std::atomic<bool> stopFlag{ false };
	std::atomic<bool> pauseFlag{ false };

	std::atomic<int> tasksSubmitted{ 0 };
	std::atomic<int> tasksCompleted{ 0 };
	std::atomic<long long> totalProcessingTimeMs{ 0 };

	void workerLoop(int queueIndex);
	int getLeastLoadedQueue() const;

public:
	ThreadPool(int queuesNum = 6);
	~ThreadPool();

	template <typename Func, typename... Args>
	void addTask(int durationSeconds, Func&& func, Args&&... args);

	void stop(bool finishTasks);
	void pause();
	void resume();

	bool isWorking() const;
	bool isPaused() const;

	std::vector<int> getQueueSizes() const;
	std::vector<int> getQueueLoadTimes() const;
	int getTasksSubmitted() const;
	int getTasksCompleted() const;
	double getAverageTaskExecutionTimeMs() const;
};

//template method implementation

template <typename Func, typename... Args>
void ThreadPool::addTask(int durationSeconds, Func&& func, Args&&... args)
{
	if (stopFlag.load() || queues.empty()) return;

	Task task;
	task.durationSeconds = durationSeconds;
	task.func = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

	int bestQueueIndex = getLeastLoadedQueue();
	queueLoadTimes[bestQueueIndex]->fetch_add(durationSeconds);
	queues[bestQueueIndex]->push(task);

	tasksSubmitted.fetch_add(1);
}

#endif //THREADPOOL_H