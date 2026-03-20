#include "TaskQueue.h"

bool TaskQueue::empty() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return innerQueue.empty();
}

int TaskQueue::size() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return innerQueue.size();
}

int TaskQueue::getTotalTime() const
{
	return totalTime.load();
}

void TaskQueue::push(const Task& task)
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		innerQueue.emplace(task);
	}
	totalTime.fetch_add(task.durationSeconds);
	condVar.notify_all();
}

bool TaskQueue::pop(Task& task)
{
	std::unique_lock<std::mutex> lock(mtx);

	condVar.wait(lock, [this] {
		return stopFlag || (!pauseFlag && !innerQueue.empty());
		});

	if (stopFlag && innerQueue.empty())
		return false;

	if (!innerQueue.empty())
	{
		task = innerQueue.front();
		innerQueue.pop();
		totalTime.fetch_sub(task.durationSeconds);

		return true;
	}
	return false;
}

void TaskQueue::stop(bool terminateInstantly)
{
	std::lock_guard<std::mutex> lock(mtx);
	stopFlag = true;

	if (terminateInstantly)
	{
		std::queue<Task> empyQueue;
		std::swap(innerQueue, empyQueue);
		totalTime.store(0);
	}

	condVar.notify_all();
}

void TaskQueue::pause()
{
	std::lock_guard<std::mutex> lock(mtx);
	pauseFlag = true;
}

void TaskQueue::resume()
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		pauseFlag = false;
	}
	condVar.notify_all();
}