#include "Logger.h"

#include <mutex>

Logger Log;
std::mutex log_lock;

Logger::Logger () : fp_debug ("output.txt"), fp_error ("error.txt") {}

void Logger::Debug (std::string_view str_v)
{
	std::lock_guard<std::mutex> lock (log_lock);
	fmt::print (str_v);
	fmt::print (fp_debug.fp, str_v);
}

void Logger::Error (std::string_view str_v)
{
	std::lock_guard<std::mutex> lock (log_lock);
	fmt::print (str_v);
	fmt::print (fp_debug.fp, str_v);
	fmt::print (fp_error.fp, str_v);
}