#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>

using std::chrono::nanoseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

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
	double time;
};

Result defaultSolution(const std::vector<int>& arr)
{
	auto beginClock = high_resolution_clock::now();

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

	auto endClock = high_resolution_clock::now();
	auto elapsed = duration_cast<nanoseconds>(endClock - beginClock);

	return { count, min, elapsed.count() * 1e-9 };
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
	auto beginClock = high_resolution_clock::now();

	int mutexCount = 0;
	int mutexMin = INT_MAX;
	std::mutex mtx;

	std::atomic<int> casCount{ 0 };
	std::atomic<int> casMin{ INT_MAX };

	std::vector<std::thread> threads;
	threads.reserve(threadsNum);

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

	for (std::thread& thread : threads)
	{
		if (thread.joinable()) thread.join();
	}

	auto endClock = high_resolution_clock::now();
	auto elapsed = duration_cast<nanoseconds>(endClock - beginClock);

	if (syncType == SyncType::MutexUnoptimized || syncType == SyncType::MutexOptimized)
		return Result{ mutexCount, mutexMin, elapsed.count() * 1e-9 };
	else
		return Result{ casCount.load(), casMin.load(), elapsed.count() * 1e-9 };
}

void testSolutions(const std::vector<int>& vectorSizes, const std::vector<int>& threadsNums)
{
	std::vector<SyncType> syncTypes =
	{
		SyncType::MutexUnoptimized,
		SyncType::MutexOptimized,
		SyncType::CASUnoptimized,
		SyncType::CASOptimized
	};

	std::ofstream outFile("test_results.csv");
	if (outFile.is_open())
	{
		outFile << "Vector Size,Strategy,Threads,Total Time (s),Count,Min,Status\n";
	}
	else
	{
		std::cerr << "\x1b[31mUnable to open file!\x1b[0m" << std::endl;
	}

	std::vector<int> coldStartVec(100000);
	fillVector(coldStartVec);
	defaultSolution(coldStartVec);
	parallelSolution(coldStartVec, 4, SyncType::MutexOptimized);

	for (int size : vectorSizes)
	{
		std::cout << "==============================================================" << std::endl;
		std::cout << "#####\tVector size: " << size << " elements\t#####" << std::endl;
		std::cout << "==============================================================" << std::endl << std::endl;

		std::vector<int> arr(size);
		fillVector(arr);

		Result defRes = defaultSolution(arr);

		std::cout << "--------------------------------------------------------------" << std::endl;
		std::cout << "Default Solution Time: " << std::fixed << std::setprecision(6) << defRes.time << " s" << std::endl;
		std::cout << "Result -> Count: " << defRes.count << " | Min: " << defRes.min << std::endl;
		std::cout << "--------------------------------------------------------------" << std::endl << std::endl;

		if (outFile.is_open())
		{
			outFile << size << ",Default (1 Thread),1," << std::fixed << std::setprecision(6)
				<< defRes.time << "," << defRes.count << "," << defRes.min << ",OK\n";
		}

		for (SyncType type : syncTypes)
		{
			std::string stratName;
			switch (type)
			{
				case SyncType::MutexUnoptimized:
					stratName = "Mutex Unoptimized";
					break;
				case SyncType::MutexOptimized:
					stratName = "Mutex Optimized";
					break;
				case SyncType::CASUnoptimized:
					stratName = "CAS Unoptimized";
					break;
				case SyncType::CASOptimized:
					stratName = "CAS Optimized";
					break;
			}

			std::cout << "-----\tStrategy name: " << stratName << "\t-----" << std::endl;
			std::cout << std::left << std::setw(12) << "Threads"
				<< std::setw(20) << "Total Time (s)"
				<< std::setw(15) << "Count"
				<< std::setw(15) << "Min" << std::endl;

			for (int threads : threadsNums)
			{
				Result parRes = parallelSolution(arr, threads, type);

				bool isResCorrect = (parRes.count == defRes.count && parRes.min == defRes.min);
				std::string parResStatus = isResCorrect ? "OK" : "ERROR";

				std::cout << std::left << std::setw(12) << threads
					<< std::setw(20) << std::fixed << std::setprecision(6) << parRes.time
					<< std::setw(15) << parRes.count
					<< std::setw(15) << parRes.min;

				if (!isResCorrect) std::cout << "\x1b[31m" << parResStatus << "\x1b[0m";
				std::cout << std::endl;

				if (outFile.is_open())
				{
					outFile << size << "," << stratName << "," << threads << ","
						<< std::fixed << std::setprecision(6) << parRes.time << ","
						<< parRes.count << "," << parRes.min << "," << parResStatus << "\n";
				}
			}
			std::cout << std::endl;
		}
	}

	if (outFile.is_open())
	{
		outFile.close();
		std::cout << "\n\x1b[32mData successfully written to test_results.csv\x1b[0m" << std::endl;
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

	std::vector<int> vectorSizes =
	{
		100000,
		500000,
		1000000,
		5000000,
		10000000,
		50000000,
		100000000
	};

	testSolutions(vectorSizes, threadsNums);

	/*std::vector<int> arr(10, 0);
	fillVector(arr);
	printVector(arr);
	Result res = defaultSolution(arr);

	std::cout << res.count << "\t" << res.min;*/

	return 0;
}