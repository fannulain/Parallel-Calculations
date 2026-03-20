#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

//declaration

template<typename TaskType>
class TaskQueue
{
private:
	std::queue<TaskType> innerQueue;
	mutable std::mutex mtx;
	std::condition_variable condVar;
	std::atomic<int> totalTime{ 0 };

	bool stopFlag = false;
	bool pauseFlag = false;

public:
	TaskQueue() = default;
	~TaskQueue() = default;

	bool empty() const;
	int size() const;
	int getTotalTime() const;

	void push(const TaskType& task);
	bool pop(TaskType& task);
	void clear();

	void stop(bool terminateInstantly);
	void pause();
	void resume();
};

//implementation

template<typename TaskType>
bool TaskQueue<TaskType>::empty() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return innerQueue.empty();
}

template<typename TaskType>
int TaskQueue<TaskType>::size() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return innerQueue.size();
}

template<typename TaskType>
int TaskQueue<TaskType>::getTotalTime() const
{
	return totalTime.load();
}

template<typename TaskType>
void TaskQueue<TaskType>::push(const TaskType& task)
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		innerQueue.emplace(task);
		totalTime.fetch_add(task.durationSeconds);
	}
	condVar.notify_all();
}

template<typename TaskType>
bool TaskQueue<TaskType>::pop(TaskType& task)
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

template<typename TaskType>
void TaskQueue<TaskType>::clear()
{
	std::queue<TaskType> emptyQueue;
	{
		std::lock_guard<std::mutex> lock(mtx);
		std::swap(innerQueue, emptyQueue);
		totalTime.store(0);
	}
}

template<typename TaskType>
void TaskQueue<TaskType>::stop(bool terminateInstantly)
{
	std::lock_guard<std::mutex> lock(mtx);
	stopFlag = true;

	if (terminateInstantly)
	{
		std::queue<TaskType> emptyQueue;
		std::swap(innerQueue, emptyQueue);
		totalTime.store(0);
	}

	condVar.notify_all();
}

template<typename TaskType>
void TaskQueue<TaskType>::pause()
{
	std::lock_guard<std::mutex> lock(mtx);
	pauseFlag = true;
}

template<typename TaskType>
void TaskQueue<TaskType>::resume()
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		pauseFlag = false;
	}
	condVar.notify_all();
}

#endif //TASKQUEUE_H