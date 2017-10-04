#pragma once
#include <chrono>
#include <array>
#include "VulkanTools.h"

typedef std::array<float, 50> TimingsHistory;

class TimeManager
{
public:
	TimeManager();
	~TimeManager();

	void StartFrameTimer();
	void EndFrameTimer();

	double GetDeltaTime(); // in seconds
	double GetRunningTime(); // in seconds
	double GetPreviousFrameTime(); //in seconds (how long the last frame took

	TimingsHistory GetFrameTimeHistory();
	float GetFrameTimeMax();
	float GetFrameTimeMin();

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
	SimpleTimer frameTimer;

	double timeSinceStart = 0.0f; //in seconds
	double deltaTime = 0.016f; //in seconds
	double prevFrameTime = 0.0f;
	TimingsHistory frameTimes{};
	float frameTimeMin = 999.0f, frameTimeMax = 0.0f;
};

