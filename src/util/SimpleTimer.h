#pragma once

#include <chrono>

class SimpleTimer
{
	public:
	SimpleTimer () { startTime = std::chrono::high_resolution_clock::now (); }

	// Begin timer
	void StartTimer () { startTime = std::chrono::high_resolution_clock::now (); }

	// End timer
	void EndTimer ()
	{
		endTime = std::chrono::high_resolution_clock::now ();
		elapsedTime = endTime - startTime;
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> GetStartTime ()
	{
		return startTime;
	}
	std::chrono::time_point<std::chrono::high_resolution_clock> GetEndTime () { return endTime; }

	uint64_t GetElapsedTimeSeconds ()
	{
		return std::chrono::duration_cast<std::chrono::seconds> (elapsedTime).count ();
	}

	uint64_t GetElapsedTimeMilliSeconds ()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds> (elapsedTime).count ();
	}

	uint64_t GetElapsedTimeMicroSeconds ()
	{
		return std::chrono::duration_cast<std::chrono::microseconds> (elapsedTime).count ();
	}

	uint64_t GetElapsedTimeNanoSeconds ()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds> (elapsedTime).count ();
	}

	private:
	std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	std::chrono::nanoseconds elapsedTime;
};
