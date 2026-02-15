#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

unsigned long long rand_seed = 1;

void fillMatrix(std::vector<std::vector<int>>& matrix)
{
	std::mt19937 gen(rand_seed);
	std::uniform_int_distribution<int> dist(0, 100);

	for (size_t i = 0; i < matrix.size(); i++)
	{
		for (size_t j = 0; j < matrix.size(); j++)
		{
			matrix[i][j] = dist(gen);
		}
	}
}

void calculateRow(std::vector<std::vector<int>>& matrix, int rowNum)
{
	int sum = 0;
	for (size_t i = 0; i < matrix.size(); i++)
	{
		if (matrix[rowNum][i] >= 10)
			sum += matrix[rowNum][i];
	}
	matrix[rowNum][rowNum] = sum;
}

void defaultSolution(std::vector<std::vector<int>>& matrix)
{
	auto begin = high_resolution_clock::now();

	for (size_t i = 0; i < matrix.size(); i++)
	{
		calculateRow(matrix, i);
	}

	auto end = high_resolution_clock::now();
	auto elapsed = duration_cast<nanoseconds>(end - begin);
	std::printf("Payload Time: %.3f seconds.\n", elapsed.count() * 1e-9);
}

int main()
{
	return 0;
}