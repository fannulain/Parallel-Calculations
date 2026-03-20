#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <vector>
#include <memory>
#include "TaskQueue.h"

class ThreadPool
{
private:
	std::vector<std::unique_ptr<TaskQueue>> queues;
	std::vector<std::thread> workers;
	bool stopFlag = false;
	bool pauseFlag = false;

	void workerLoop(int queueIndex);
	int getLeastLoadedQueue() const;

public:
	ThreadPool(int queuesNum = 6);
	~ThreadPool();

	void addTask(const Task& task);
	void stop(bool finishTasks);
	void pause();
	void resume();

	bool isWorking() const;
	bool isPaused() const;

	std::vector<int> getQueueSizes() const;
	std::vector<int> getQueueLoadTimes() const;
};

#endif //THREADPOOL_H