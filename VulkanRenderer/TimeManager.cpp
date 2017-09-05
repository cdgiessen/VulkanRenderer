#include "TimeManager.h"



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

	deltaTime = frameTimer.GetElapsedTimeNanoSeconds() / 1.0e9;
	timeSinceStart = std::chrono::duration_cast<std::chrono::microseconds>(frameTimer.GetEndTime() - startTime).count() / 1.0e6;

	// Update frame timings display
	{
		std::rotate(frameTimes.begin(), frameTimes.begin() + 1, frameTimes.end());
		double frameTime = 1000.0f / (deltaTime * 1000.0f);
		frameTimes.back() = frameTime;
		if (frameTime < frameTimeMin) {
			frameTimeMin = frameTime;
		}
		if (frameTime > frameTimeMax) {
			frameTimeMax = frameTime;
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

float TimeManager::GetFrameTimeMax(){ return frameTimeMax;}
float TimeManager::GetFrameTimeMin(){ return frameTimeMin;}