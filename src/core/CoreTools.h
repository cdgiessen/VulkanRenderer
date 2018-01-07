#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>

std::string GetExecutableFilePath();
void SetExecutableFilePath(std::string file);

std::string GetFilePathFromFullPath(std::string file);

bool fileExists(const std::string &filename);

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}



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