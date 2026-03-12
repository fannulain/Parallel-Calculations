#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <iomanip>

unsigned long long rand_seed = 1;

void fillVector(std::vector<int>& arr)
{
	std::mt19937 gen(rand_seed);
	std::uniform_int_distribution<int> dist(-1000, 1000);

	for (int i = 0; i < arr.size(); i++)
	{
		arr[i] = dist(gen);
	}
}

void printVector(std::vector<int>& arr)
{
	for (int i = 0; i < arr.size(); i++)
	{
		std::cout << arr[i] << "\t";
	}
	std::cout << std::endl;
}

struct Result
{
	int count;
	int min;
};

Result defaultSolution(std::vector<int>& arr)
{
	int count = 0;
	int min = INT_MAX;

	for (int i = 0; i < arr.size(); i++)
	{
		if (arr[i] < -42)
		{
			count++;
			min = arr[i] < min ? arr[i] : min;
		}
	}

	return { count, min };
}

int main()
{
	std::vector<int> arr(10, 0);
	fillVector(arr);
	printVector(arr);
	Result res = defaultSolution(arr);

	std::cout << res.count << "\t" << res.min;

	return 0;
}