#include "CoreTools.h"

SimpleTimer::SimpleTimer () { startTime = std::chrono::high_resolution_clock::now (); }

void SimpleTimer::StartTimer () { startTime = std::chrono::high_resolution_clock::now (); }

void SimpleTimer::EndTimer ()
{
	endTime = std::chrono::high_resolution_clock::now ();
	elapsedTime = endTime - startTime;
}

std::chrono::time_point<std::chrono::high_resolution_clock> SimpleTimer::GetStartTime ()
{
	return startTime;
}

std::chrono::time_point<std::chrono::high_resolution_clock> SimpleTimer::GetEndTime ()
{
	return endTime;
}

uint64_t SimpleTimer::GetElapsedTimeSeconds ()
{
	return std::chrono::duration_cast<std::chrono::seconds> (elapsedTime).count ();
}

uint64_t SimpleTimer::GetElapsedTimeMilliSeconds ()
{
	return std::chrono::duration_cast<std::chrono::milliseconds> (elapsedTime).count ();
}

uint64_t SimpleTimer::GetElapsedTimeMicroSeconds ()
{
	return std::chrono::duration_cast<std::chrono::microseconds> (elapsedTime).count ();
}

uint64_t SimpleTimer::GetElapsedTimeNanoSeconds ()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds> (elapsedTime).count ();
}