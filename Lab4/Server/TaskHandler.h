#ifndef TASKHANDLER_H
#define TASKHANDLER_H

#include <vector>
#include <thread>

class TaskHandler
{
private:
	int threadsNum;
	int size;
	std::vector<std::vector<int>>& matrix;

	void calculateRow(int rowNum);

public:
	TaskHandler(int threadsNum, int size, std::vector<std::vector<int>>& matrix);
    
	TaskHandler(const TaskHandler&) = delete;
	TaskHandler& operator=(const TaskHandler&) = delete;
    
	void calculate();
};

#endif //TASKHANDLER_H