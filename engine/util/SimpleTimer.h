#pragma once

#include <chrono>

class SimpleTimer
{
	public:
	SimpleTimer () { startTime = std::chrono::high_resolution_clock::now (); }

	// Begin timer
	void start_timer () { startTime = std::chrono::high_resolution_clock::now (); }

	// End timer
	void end_timer ()
	{
		endTime = std::chrono::high_resolution_clock::now ();
		elapsedTime = endTime - startTime;
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> get_start_time ()
	{
		return startTime;
	}
	std::chrono::time_point<std::chrono::high_resolution_clock> get_end_time () { return endTime; }

	uint64_t get_elapsed_time_seconds ()
	{
		return std::chrono::duration_cast<std::chrono::seconds> (elapsedTime).count ();
	}

	uint64_t get_elapsed_time_milli_seconds ()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds> (elapsedTime).count ();
	}

	uint64_t get_elapsed_time_micro_seconds ()
	{
		return std::chrono::duration_cast<std::chrono::microseconds> (elapsedTime).count ();
	}

	uint64_t get_elapsed_time_nano_seconds ()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds> (elapsedTime).count ();
	}

	private:
	std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	std::chrono::nanoseconds elapsedTime;
};
