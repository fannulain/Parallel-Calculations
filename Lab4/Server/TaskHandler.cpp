#include "TaskHandler.h"

TaskHandler::TaskHandler(int threadsNum, int size, std::vector<std::vector<int>>& matrix)
	: threadsNum(threadsNum), size(size), matrix(matrix)
{}

void TaskHandler::calculateRow(int rowNum)
{
	int sum = 0;
	for (int i = 0; i < matrix[rowNum].size(); i++)
	{
		if (matrix[rowNum][i] >= 10)
			sum += matrix[rowNum][i];
	}
	matrix[rowNum][rowNum] = sum == 0 ? sum : sum - matrix[rowNum][rowNum];
}

void TaskHandler::calculate()
{
	if (threadsNum > size) threadsNum = size;

	std::vector<std::thread> threads;
	threads.reserve(threadsNum);
	int rowsPerThread = size / threadsNum;

	for (int i = 0; i < threadsNum; i++)
	{
		int firstRow = i * rowsPerThread;
		int lastRow = (i == threadsNum - 1) ? size : (firstRow + rowsPerThread);

		threads.emplace_back([this, firstRow, lastRow]()
			{
				for (int row = firstRow; row < lastRow; row++)
					calculateRow(row);
			});
	}

	for (std::thread& thread : threads) {
		if (thread.joinable()) thread.join();
	}
}