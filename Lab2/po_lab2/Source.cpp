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

void printVector(const std::vector<int>& arr)
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

Result defaultSolution(const std::vector<int>& arr)
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

void processBlockMutexUnoptimized(const std::vector<int>& arr, int start, int end,
	int& count, int& min, std::mutex& mtx)
{
	for (int i = start; i < end; i++)
	{
		if (arr[i] < -42)
		{
			std::lock_guard<std::mutex> lock(mtx);

			count++;
			min = arr[i] < min ? arr[i] : min;
		}
	}
}

void processBlockMutexOptimized(const std::vector<int>& arr, int start, int end,
	int& count, int& min, std::mutex& mtx)
{
	int local_count = 0;
	int local_min = INT_MAX;

	for (int i = start; i < end; i++)
	{
		if (arr[i] < -42)
		{
			local_count++;
			local_min = arr[i] < local_min ? arr[i] : local_min;
		}
	}

	if (local_count > 0)
	{
		std::lock_guard<std::mutex> lock(mtx);

		count += local_count;
		if (local_min < min) min = local_min;
	}
}

void processBlockCASUnoptimized(const std::vector<int>& arr, int start, int end,
	std::atomic<int>& count, std::atomic<int>& min)
{
	for (int i = start; i < end; i++)
	{
		if (arr[i] < -42)
		{
			int curr_count = count.load();
			while (!count.compare_exchange_weak(curr_count, curr_count + 1)) {}

			int curr_min = min.load();
			while (arr[i] < curr_min)
			{
				if (min.compare_exchange_weak(curr_min, arr[i])) break;
			}
		}
	}
}

void processBlockCASOptimized(const std::vector<int>& arr, int start, int end,
	std::atomic<int>& count, std::atomic<int>& min)
{
	int local_count = 0;
	int local_min = INT_MAX;

	for (int i = start; i < end; i++)
	{
		if (arr[i] < -42)
		{
			local_count++;
			local_min = arr[i] < local_min ? arr[i] : local_min;
		}
	}

	if (local_count > 0)
	{
		int curr_count = count.load();
		while (!count.compare_exchange_weak(curr_count, curr_count + local_count)) {}

		int curr_min = min.load();
		while (local_min < curr_min)
		{
			if (min.compare_exchange_weak(curr_min, local_min)) break;
		}
	}
}

enum class SyncType
{
	MutexUnoptimized,
	MutexOptimized,
	CASUnoptimized,
	CASOptimized
};

Result parallelSolution(const std::vector<int>& arr, int threadsNum, SyncType syncType)
{
	int mutexCount = 0;
	int mutexMin = INT_MAX;
	std::mutex mtx;

	std::atomic<int> casCount{ 0 };
	std::atomic<int> casMin{ INT_MAX };

	std::vector<std::thread> threads(threadsNum);

	int elemsPerThread = arr.size() / threadsNum;
	
	for (int i = 0; i < threadsNum; i++)
	{
		int start = i * elemsPerThread;
		int end = (i == threadsNum - 1) ? arr.size() : (start + elemsPerThread);

		switch (syncType)
		{
			case SyncType::MutexUnoptimized:
				threads.emplace_back(processBlockMutexUnoptimized, std::ref(arr), start, end, 
					std::ref(mutexCount), std::ref(mutexMin), std::ref(mtx));
				break;
			case SyncType::MutexOptimized:
				threads.emplace_back(processBlockMutexOptimized, std::ref(arr), start, end, 
					std::ref(mutexCount), std::ref(mutexMin), std::ref(mtx));
				break;
			case SyncType::CASUnoptimized:
				threads.emplace_back(processBlockCASUnoptimized, std::ref(arr), start, end, 
					std::ref(casCount), std::ref(casMin));
				break;
			case SyncType::CASOptimized:
				threads.emplace_back(processBlockCASOptimized, std::ref(arr), start, end, 
					std::ref(casCount), std::ref(casMin));
				break;
			default:
				break;
		}
	}

	for (int i = 0; i < threadsNum; i++)
	{
		if (threads[i].joinable()) threads[i].join();
	}
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