#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <iomanip>

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
			if (i == j)
				std::cout << "\033[32m" << matrix[i][j] << "\033[0m" << "\t";
			else
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
		if (matrix[rowNum][i] >= 10)
			sum += matrix[rowNum][i];
	}
	matrix[rowNum][rowNum] = sum - matrix[rowNum][rowNum]; //віднімаємо діагональний елемент для чистоти перевірки
}

//рішення без паралелізації

double defaultSolution(std::vector<std::vector<int>>& matrix)
{
	auto begin = high_resolution_clock::now();

	for (int i = 0; i < matrix.size(); i++)
	{
		calculateRow(matrix, i);
	}

	auto end = high_resolution_clock::now();
	auto elapsed = duration_cast<nanoseconds>(end - begin);

	return elapsed.count() * 1e-9;
}

//рішення з паралелізацією

enum class parallelismType //тип розподілу рядків між потоками
{
	Block,
	Cyclic
};

struct timeResults
{
	double totalTime;
	double threadsTime;
};

timeResults parallelSolution(std::vector<std::vector<int>>& matrix, int threadsNum, parallelismType type)
{
	auto beginTotal = high_resolution_clock::now(); //загальний час

	if (threadsNum > matrix.size()) threadsNum = matrix.size();

	std::vector<std::thread> threads;
	threads.reserve(threadsNum);
	int rowsPerThread = matrix.size() / threadsNum;

	auto beginThreads = high_resolution_clock::now(); //заміряємо час створення потоків

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

	auto endThreads = high_resolution_clock::now();

	for (std::thread& thread : threads) {
		if (thread.joinable()) thread.join();
	}

	auto endTotal = high_resolution_clock::now();
	auto elapsedTotal = duration_cast<nanoseconds>(endTotal - beginTotal);
	auto elapsedThreads = duration_cast<nanoseconds>(endThreads - beginThreads);

	return { elapsedTotal.count() * 1e-9, elapsedThreads.count() * 1e-9 };
}

//тестові сценарії
void testSolutions(const std::vector<int>& matrixSizes, const std::vector<int>& threadsNums)
{
	std::vector<parallelismType> types =
	{
		parallelismType::Block,
		parallelismType::Cyclic
	};

	for (int size : matrixSizes)
	{
		std::cout << "#####\tMatrix size: " << size << "x" << size << "\t#####" << std::endl << std::endl;

		//заміряємо час звичайного рішення
		std::vector<std::vector<int>> matrixDefSol(size, std::vector<int>(size));
		fillMatrix(matrixDefSol);
		double defSolTime = defaultSolution(matrixDefSol);

		std::cout << "-------------------------------------" << std::endl;
		std::cout << "Default Solution Time: " << std::fixed << std::setprecision(6) << defSolTime << " s" << std::endl;
		std::cout << "-------------------------------------" << std::endl << std::endl;
		std::cout << "Parallel Solution: " << std::endl << std::endl;

		for (parallelismType type : types)
		{
			std::string strat = (type == parallelismType::Block) ? "Block" : "Cyclic";
			std::cout << "-----\tStrategy name: " << strat << "\t-----" << std::endl;
			std::cout << std::left << std::setw(12) << "Threads" << std::setw(20) << "Total Time (s)" << std::setw(25) << "Threads Creation Time (s)" << std::endl;

			for (int threads : threadsNums)
			{
				//заміряємо час паралельного рішення для всіх комбінацій
				std::vector<std::vector<int>> matrixParSol(size, std::vector<int>(size));
				fillMatrix(matrixParSol);

				timeResults parSolTime = parallelSolution(matrixParSol, threads, type);

				std::cout << std::left << std::setw(12) << threads << std::setw(20) << std::fixed << std::setprecision(6) << 
					parSolTime.totalTime << std::setw(25) << std::fixed << std::setprecision(6) << parSolTime.threadsTime << std::endl;
			}
			std::cout << std::endl;
		}
		std::cout << "#####################################" << std::endl << std::endl;
	}
}

int main()
{
	int physicalCores = 10;
	int logicalCores = 16;

	std::vector<int> threadsNums =
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
		25000
	};

	testSolutions(matrixSizes, threadsNums);

	/*std::vector<std::vector<int>> matrix(10, std::vector<int>(10, 0));
	fillMatrix(matrix);
	defaultSolution(matrix);
	printMatrix(matrix);*/

	return 0;
}