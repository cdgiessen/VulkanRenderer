#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

enum class FileStatus
{
	created,
	modified,
	erased
};

class FileWatcher
{
	public:
	std::string path_to_watch;
	// Time interval at which we check the base folder for changes
	std::chrono::duration<int, std::milli> delay;

	// Keep a record of files from the base directory and their last modification time
	FileWatcher (std::string path_to_watch, std::chrono::duration<int, std::milli> delay);
	// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
	void start (const std::function<void(std::string, FileStatus)>& action);

	private:
	std::unordered_map<std::string, std::filesystem::file_time_type> paths_;

	// Check if "paths_" contains a given key
	// If your compiler supports C++20 use paths_.contains(key) instead of this function
	bool contains (const std::string& key);
};