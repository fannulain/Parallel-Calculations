#include "ThreadPool.h"

ThreadPool::ThreadPool(int queuesNum)
{
	for (int i = 0; i < queuesNum; i++)
	{
		queues.push_back(std::make_unique<TaskQueue>());
	}

	for (int i = 0; i < queuesNum; i++)
	{
		workers.emplace_back(&ThreadPool::workerLoop, this, i);
	}
}

ThreadPool::~ThreadPool()
{
	if (!stopFlag)
		stop(true);
}

void ThreadPool::workerLoop(int queueIndex)
{
	Task task;

	while (queues[queueIndex]->pop(task))
	{
		if (task.func)
			task.func();
	}
}

int ThreadPool::getLeastLoadedQueue() const
{
	int minTime = INT_MAX;
	int bestQueueIndex = 0;

	for (int i = 0; i < queues.size(); i++)
	{
		int currQueueTime = queues[i]->getTotalTime();
		if (currQueueTime < minTime)
		{
			minTime = currQueueTime;
			bestQueueIndex = i;
		}
	}
}

void ThreadPool::addTask(const Task& task)
{
	if (stopFlag || queues.empty()) return;

	int bestQueueIndex = getLeastLoadedQueue();
	queues[bestQueueIndex]->push(task);
}

void ThreadPool::stop(bool finishTasks)
{
	if (stopFlag) return;
	stopFlag = true;

	for (std::unique_ptr<TaskQueue>& queue : queues)
	{
		queue->stop(!finishTasks);
	}

	for (std::thread& worker : workers)
	{
		if (worker.joinable())
			worker.join();
	}
}

void ThreadPool::pause()
{
	if (stopFlag || pauseFlag) return;
	pauseFlag = true;

	for (std::unique_ptr<TaskQueue>& queue : queues)
	{
		queue->pause();
	}
}

void ThreadPool::resume()
{
	if (stopFlag || !pauseFlag) return;
	pauseFlag = false;

	for (std::unique_ptr<TaskQueue>& queue : queues)
	{
		queue->resume();
	}
}

bool ThreadPool::isWorking() const
{
	return !stopFlag;
}

bool ThreadPool::isPaused() const
{
	return pauseFlag;
}

std::vector<int> ThreadPool::getQueueSizes() const
{
	std::vector<int> sizes;
	sizes.reserve(queues.size());
	for (const std::unique_ptr<TaskQueue>& queue : queues)
	{
		sizes.push_back(queue->size());
	}
	return sizes;
}

std::vector<int> ThreadPool::getQueueLoadTimes() const
{
	std::vector<int> loadTimes;
	loadTimes.reserve(queues.size());
	for (const std::unique_ptr<TaskQueue>& queue : queues)
	{
		loadTimes.push_back(queue->getTotalTime());
	}
	return loadTimes;
}