#include "CoreTools.h"

std::string executableFilePath;

std::string GetFilePathFromFullPath(std::string file) {
	std::string path = file;
	#ifdef _WIN32
	auto pos = path.rfind("\\");
	#else
	auto pos = path.rfind("/");
	#endif // _WIN32
	path.erase(pos, path.size() - pos);

	return path;
}

void SetExecutableFilePath(std::string file) {
	executableFilePath = GetFilePathFromFullPath(file);
}

std::string GetExecutableFilePath() {
	return executableFilePath;
}

bool fileExists(const std::string &filename)
{
	std::ifstream f(filename.c_str());
	return !f.fail();
}

//static std::vector<char> readFile(const std::string& filename) {
//	std::ifstream file(filename, std::ios::ate | std::ios::binary);
//
//	if (!file.is_open()) {
//		throw std::runtime_error("failed to open file!");
//	}
//
//	size_t fileSize = (size_t)file.tellg();
//	std::vector<char> buffer(fileSize);
//
//	file.seekg(0);
//	file.read(buffer.data(), fileSize);
//
//	file.close();
//
//	return buffer;
//}

SimpleTimer::SimpleTimer() {
	startTime = std::chrono::high_resolution_clock::now();
}

void SimpleTimer::StartTimer() {
	startTime = std::chrono::high_resolution_clock::now();
}

void SimpleTimer::EndTimer() {
	endTime = std::chrono::high_resolution_clock::now();
	elapsedTime = endTime - startTime;
}

std::chrono::time_point<std::chrono::high_resolution_clock> SimpleTimer::GetStartTime() {
	return startTime;
}

std::chrono::time_point<std::chrono::high_resolution_clock> SimpleTimer::GetEndTime() {
	return endTime;
}

uint64_t SimpleTimer::GetElapsedTimeSeconds() {
	return std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();
}

uint64_t SimpleTimer::GetElapsedTimeMilliSeconds() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count();
}

uint64_t SimpleTimer::GetElapsedTimeMicroSeconds() {
	return std::chrono::duration_cast<std::chrono::microseconds>(elapsedTime).count();
}

uint64_t SimpleTimer::GetElapsedTimeNanoSeconds() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsedTime).count();
}