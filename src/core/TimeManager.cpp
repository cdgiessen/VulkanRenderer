#include "TimeManager.h"
#include <algorithm>

using namespace std::chrono;

TimeManager::TimeManager()
{
	applicationStartTime = high_resolution_clock::now();
	frameStartTime = high_resolution_clock::now();
	frameEndTime = high_resolution_clock::now();
	curFrameTime = (frameEndTime - frameStartTime);

	frameTimes = std::vector<double>(50, 0.01666);
}

void TimeManager::CollectRuntimeData()
{
	applicationEndTime = high_resolution_clock::now();
}

void TimeManager::StartFrameTimer() {
	frameStartTime = high_resolution_clock::now();
}

void TimeManager::EndFrameTimer() {
	frameEndTime = high_resolution_clock::now();

	prevFrameTime = curFrameTime;
	curFrameTime = (frameEndTime - frameStartTime);

	// Update frame timings display
	{
		std::rotate(frameTimes.begin(), frameTimes.begin() + 1, frameTimes.end());

		frameTimes.back() = (float)DeltaTime();

		if (frameTimes.back() < frameTimeMin) {
			frameTimeMin = frameTimes.back();
		}
		if (frameTimes.back() > frameTimeMax) {
			frameTimeMax = frameTimes.back();
		}
	}
}

double TimeManager::ExactTimeSinceFrameStart() {
	DoubleDuration time = high_resolution_clock::now() - frameStartTime;
	return time.count();
}

double TimeManager::DeltaTime() {
	DoubleDuration time = curFrameTime;
	return time.count();
}


double TimeManager::RunningTime() {
	DoubleDuration time = high_resolution_clock::now() - applicationStartTime;
	return time.count();
}

BriefTimingHistory TimeManager::FrameTimeHistory() {
	return frameTimes;
}

double TimeManager::PreviousFrameTime() {
	return prevFrameTime.count();
}

float TimeManager::FrameTimeMax() { return frameTimeMax; }
float TimeManager::FrameTimeMin() { return frameTimeMin; }
