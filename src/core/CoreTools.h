#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <functional>

std::string GetExecutableFilePath();
void SetExecutableFilePath(std::string file);

std::string GetFilePathFromFullPath(std::string file);

bool fileExists(const std::string &filename);

std::vector<char> readFile(const std::string& filename);



class SimpleTimer {
public:
	SimpleTimer();

	//Begin timer
	void StartTimer();

	//End timer
	void EndTimer();

	std::chrono::time_point<std::chrono::high_resolution_clock> GetStartTime();
	std::chrono::time_point<std::chrono::high_resolution_clock> GetEndTime();

	uint64_t GetElapsedTimeSeconds();
	uint64_t GetElapsedTimeMilliSeconds();
	uint64_t GetElapsedTimeMicroSeconds();
	uint64_t GetElapsedTimeNanoSeconds();

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	std::chrono::nanoseconds elapsedTime;
};

class Job {
public:
	Job(std::function<void()> work) : work(work) {}
private:
	std::function<void()> work;
};

class Task {
public:
	void AddJob(Job&& newJob);

	void operator()();

	void WaitOn();
private:
	std::vector<Job> jobs;

	std::mutex condVar_lock;
	std::condition_variable condVar;
};