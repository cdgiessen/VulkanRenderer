#include "TimeManager.h"
#include <algorithm>


TimeManager::TimeManager()
{
	startTime = std::chrono::high_resolution_clock::now();
}


TimeManager::~TimeManager()
{
}

void TimeManager::StartFrameTimer() {
	frameTimer.StartTimer();
}

void TimeManager::EndFrameTimer() {
	frameTimer.EndTimer();

	prevFrameTime = frameTimer.GetElapsedTimeMicroSeconds() / 1.0e6;
	deltaTime = frameTimer.GetElapsedTimeNanoSeconds() / 1.0e9;
	timeSinceStart = std::chrono::duration_cast<std::chrono::microseconds>(frameTimer.GetEndTime() - startTime).count() / 1.0e6;

	// Update frame timings display
	{
		std::rotate(frameTimes.begin(), frameTimes.begin() + 1, frameTimes.end());
		double frameTime = 1000.0f / (deltaTime * 1000.0f);
		frameTimes.back() = (float)frameTime;
		if (frameTime < frameTimeMin) {
			frameTimeMin = (float)frameTime;
		}
		if (frameTime > frameTimeMax) {
			frameTimeMax = (float)frameTime;
		}
	}
}

double TimeManager::GetDeltaTime() {
	return deltaTime;
}


double TimeManager::GetRunningTime() {
	return timeSinceStart;
}

TimingsHistory TimeManager::GetFrameTimeHistory() {
	return frameTimes;
}

double TimeManager::GetPreviousFrameTime() {
	return prevFrameTime;
}

float TimeManager::GetFrameTimeMax(){ return frameTimeMax;}
float TimeManager::GetFrameTimeMin(){ return frameTimeMin;}
