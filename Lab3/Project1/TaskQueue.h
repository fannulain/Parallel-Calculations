#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

//declaration

template<typename TaskType>
class TaskQueue
{
private:
	std::queue<TaskType> innerQueue;
	mutable std::mutex mtx;
	std::condition_variable condVar;

	bool stopFlag = false;
	bool pauseFlag = false;

public:
	TaskQueue() = default;
	~TaskQueue() = default;

	bool empty() const;
	int size() const;

	template<typename TaskArg>
	void push(TaskArg&& task);

	template <typename... Args>
	void emplace(Args&&... args);

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
template<typename TaskArg>
void TaskQueue<TaskType>::push(TaskArg&& task)
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		innerQueue.push(std::forward<TaskArg>(task));
	}
	condVar.notify_all();
}

template<typename TaskType>
template<typename... Args>
void TaskQueue<TaskType>::emplace(Args&&... args)
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		innerQueue.emplace(std::forward<Args>(args)...);
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

		return true;
	}
	return false;
}

template<typename TaskType>
void TaskQueue<TaskType>::clear()
{
	std::queue<TaskType> emptyQueue;
	std::lock_guard<std::mutex> lock(mtx);
	std::swap(innerQueue, emptyQueue);
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