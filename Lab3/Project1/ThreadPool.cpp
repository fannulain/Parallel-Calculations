#include "ThreadPool.h"

ThreadPool::ThreadPool(int queuesNum)
{
	for (int i = 0; i < queuesNum; i++)
	{
		queues.push_back(std::make_unique<TaskQueue<Task>>());
		queueLoadTimes.push_back(std::make_unique<std::atomic<int>>(0));
	}

	for (int i = 0; i < queuesNum; i++)
	{
		workers.emplace_back(&ThreadPool::workerLoop, this, i);
	}
}

ThreadPool::~ThreadPool()
{
	if (!stopFlag.load())
		stop(true);
}

void ThreadPool::workerLoop(int queueIndex)
{
	Task task;

	while (true)
	{
		auto waitStart = std::chrono::high_resolution_clock::now();

		bool hasTask = queues[queueIndex]->pop(task);

		auto waitEnd = std::chrono::high_resolution_clock::now();
		auto waitDuration = std::chrono::duration_cast<std::chrono::milliseconds>(waitEnd - waitStart).count();

		totalWaitTimeMs.fetch_add(waitDuration);

		if (!hasTask) break;

		if (task.func)
		{
			auto startTimer = std::chrono::high_resolution_clock::now();

			task.func();

			auto endTimer = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTimer - startTimer).count();
		
			totalProcessingTimeMs.fetch_add(duration);
			tasksCompleted.fetch_add(1);

			queueLoadTimes[queueIndex]->fetch_sub(task.durationSeconds);
		}
	}
}

int ThreadPool::getLeastLoadedQueue() const
{
	int minTime = INT_MAX;
	int bestQueueIndex = 0;

	for (int i = 0; i < queues.size(); i++)
	{
		int currQueueTime = queueLoadTimes[i]->load();
		if (currQueueTime < minTime)
		{
			minTime = currQueueTime;
			bestQueueIndex = i;
		}
	}
	return bestQueueIndex;
}

void ThreadPool::stop(bool finishTasks)
{
	if (stopFlag.exchange(true)) return;

	for (int i = 0; i < queues.size(); i++)
	{
		auto droppedTasks = queues[i]->stop(!finishTasks);

		for (const auto& task : droppedTasks)
		{
			queueLoadTimes[i]->fetch_sub(task.durationSeconds);
		}
	}

	for (std::thread& worker : workers)
	{
		if (worker.joinable())
			worker.join();
	}
}

void ThreadPool::pause()
{
	if (stopFlag.load() || pauseFlag.exchange(true)) return;

	for (const auto& queue : queues)
	{
		queue->pause();
	}
}

void ThreadPool::resume()
{
	if (stopFlag.load() || !pauseFlag.exchange(false)) return;

	for (const auto& queue : queues)
	{
		queue->resume();
	}
}

bool ThreadPool::isWorking() const
{
	return !stopFlag.load();
}

bool ThreadPool::isPaused() const
{
	return pauseFlag.load();
}

std::vector<int> ThreadPool::getQueueSizes() const
{
	std::vector<int> sizes;
	sizes.reserve(queues.size());
	for (const auto& queue : queues)
	{
		sizes.push_back(queue->size());
	}
	return sizes;
}

std::vector<int> ThreadPool::getQueueLoadTimes() const
{
	std::vector<int> loadTimes;
	loadTimes.reserve(queueLoadTimes.size());
	for (const auto& loadTime : queueLoadTimes)
	{
		loadTimes.push_back(loadTime->load());
	}
	return loadTimes;
}

int ThreadPool::getTasksSubmitted() const 
{ 
	return tasksSubmitted.load(); 
}

int ThreadPool::getTasksCompleted() const
{
	return tasksCompleted.load();
}

double ThreadPool::getAverageTaskExecutionTimeMs() const
{
	int completed = tasksCompleted.load();
	if (completed == 0) return 0.0;

	return static_cast<double>(totalProcessingTimeMs.load()) / completed;
}

double ThreadPool::getAverageWaitTimePerWorkerMs() const
{
	if (workers.empty()) return 0.0;
	return static_cast<double>(totalWaitTimeMs.load()) / workers.size();
}