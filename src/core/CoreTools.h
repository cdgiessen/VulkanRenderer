#pragma once

#include <string>
#include <vector>
#include <chrono>
           

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