#ifndef TASKSTRUCT_H
#define TASKSTRUCT_H

#include <functional>

struct Task
{
	std::function<void()> func;
	int durationSeconds;
};

#endif //TASKSTRUCT_H