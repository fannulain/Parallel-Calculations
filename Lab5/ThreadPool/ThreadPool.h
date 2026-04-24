#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <algorithm>
#include "TaskQueue.h"

class ThreadPool
{
private:
	std::unique_ptr<TaskQueue<std::function<void()>>> queue;
	std::vector<std::thread> workers;

	std::atomic<bool> stopFlag{ false };
	std::atomic<bool> pauseFlag{ false };

	std::atomic<int> tasksSubmitted{ 0 };
	std::atomic<int> tasksCompleted{ 0 };

	void workerLoop();

public:
	ThreadPool(int threadCount = 0);
	~ThreadPool();

	template <typename Func, typename... Args>
	void addTask(Func&& func, Args&&... args);

	void stop(bool finishTasks);
	void pause();
	void resume();

	bool isWorking() const;
	bool isPaused() const;

	int getQueueSize() const;
	int getTasksSubmitted() const;
	int getTasksCompleted() const;
};

template <typename Func, typename... Args>
void ThreadPool::addTask(Func&& func, Args&&... args)
{
	if (stopFlag.load() || !queue) return;

	auto task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
	queue->push(std::move(task));
	tasksSubmitted.fetch_add(1);
}

#endif //THREADPOOL_H