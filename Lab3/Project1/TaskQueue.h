#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct Task
{
	std::function<void()> func;
	int durationSeconds;
};

class TaskQueue
{
private:
	std::queue<Task> innerQueue;
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

	void push(const Task& task);
	bool pop(Task& task);

	void stop(bool terminateInstantly);
	void pause();
	void resume();
};

#endif //TASKQUEUE_H