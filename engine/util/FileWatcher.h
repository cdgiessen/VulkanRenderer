#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>

enum class FileStatus
{
	created,
	modified,
	erased
};

class FileWatcher
{
	// Keep	 a record of files from the base directory and their last modification time
	public:
	FileWatcher (std::string path_to_watch,
	    std::chrono::duration<int, std::milli> delay = std::chrono::duration<int, std::milli> (500));
	~FileWatcher ();

	// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
	void start (const std::function<void (std::string, FileStatus)>& action);

	void stop ();

	private:
	std::string path_to_watch;

	// Time interval at which we check the base folder for changes
	std::chrono::duration<int, std::milli> delay;

	std::unordered_map<std::string, std::filesystem::file_time_type> paths_;

	std::thread watcher;
	bool is_running = true;

	void Watch (const std::function<void (std::string, FileStatus)>& action);

	// Check if "paths_" contains a given key
	// If your compiler supports C++20 use paths_.contains(key) instead of this function
	bool contains (const std::string& key);
};