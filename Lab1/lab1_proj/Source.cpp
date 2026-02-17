#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

unsigned long long rand_seed = 1;

//базові операції над матрицею

void fillMatrix(std::vector<std::vector<int>>& matrix)
{
	std::mt19937 gen(rand_seed);
	std::uniform_int_distribution<int> dist(0, 100);

	for (int i = 0; i < matrix.size(); i++)
	{
		for (int j = 0; j < matrix.size(); j++)
		{
			matrix[i][j] = dist(gen);
		}
	}
}

void printMatrix(std::vector<std::vector<int>>& matrix)
{
	for (int i = 0; i < matrix.size(); i++)
	{
		for (int j = 0; j < matrix.size(); j++)
		{
			std::cout << matrix[i][j] << "\t";
		}
		std::cout << std::endl;
	}
}

//опрацювання одного рядка(будемо використовувати в обох підходах)

void calculateRow(std::vector<std::vector<int>>& matrix, int rowNum)
{
	int sum = 0;
	for (int i = 0; i < matrix.size(); i++)
	{
		if (i == rowNum) continue;

		if (matrix[rowNum][i] >= 10)
			sum += matrix[rowNum][i];
	}
	matrix[rowNum][rowNum] = sum;
}

//рішення без паралелізації

void defaultSolution(std::vector<std::vector<int>>& matrix)
{
	auto begin = high_resolution_clock::now();

	for (int i = 0; i < matrix.size(); i++)
	{
		calculateRow(matrix, i);
	}

	auto end = high_resolution_clock::now();
	auto elapsed = duration_cast<nanoseconds>(end - begin);
	std::cout << "Default solution time: " << elapsed.count() * 1e-9 
		<< " for matrix size " << matrix.size() << "x" << matrix.size() << std::endl;
}

//рішення з паралелізацією

enum class parallelismType //тип розподілу рядків між потоками
{
	Block,
	Cyclic
};

double parallelSolution(std::vector<std::vector<int>>& matrix, int threadsNum, parallelismType type)
{
	auto begin = high_resolution_clock::now();

	if (threadsNum > matrix.size()) threadsNum = matrix.size();

	std::vector<std::thread> threads;
	int rowsPerThread = matrix.size() / threadsNum;

	for (int i = 0; i < threadsNum; i++)
	{
		if (type == parallelismType::Block) //ділимо рядки між потоками по блокам
		{
			int firstRow = i * rowsPerThread;
			int lastRow = (i == threadsNum - 1) ? matrix.size() : (firstRow + rowsPerThread);

			threads.emplace_back([&matrix, firstRow, lastRow]()
				{
					for (int row = firstRow; row < lastRow; row++)
						calculateRow(matrix, row);
				});
		}
		else if (type == parallelismType::Cyclic) //ділимо рядки між потоками циклічно
		{
			threads.emplace_back([&matrix, i, threadsNum]()
				{
					for (int row = i; row < matrix.size(); row += threadsNum)
						calculateRow(matrix, row);
				});
		}
	}

	for (std::thread& thread : threads) {
		if (thread.joinable()) thread.join();
	}

	auto end = high_resolution_clock::now();
	auto elapsed = duration_cast<nanoseconds>(end - begin);

	return elapsed.count() * 1e-9;
}

int main()
{
	int physicalCores = 10;
	int logicalCores = 16;

	std::vector<int> threadsNum =
	{
		static_cast<int>(physicalCores / 2),
		physicalCores,
		logicalCores,
		logicalCores * 2,
		logicalCores * 4,
		logicalCores * 8,
		logicalCores * 16
	};

	std::vector<int> matrixSizes =
	{
		100,
		500,
		1000,
		2500,
		5000,
		10000,
		25000,
		50000
	};

	std::vector<std::vector<int>> matrix(10, std::vector<int>(10, 0));
	fillMatrix(matrix);
	defaultSolution(matrix);
	std::cout << std::endl;
	printMatrix(matrix);

	std::cout << std::endl;
	std::cout << parallelSolution(matrix, threadsNum[1], parallelismType::Cyclic);;

	return 0;
}