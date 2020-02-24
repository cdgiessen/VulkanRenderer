#pragma once
#include <array>
#include <chrono>

using PreciseClockPoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using BriefTimingHistory = std::array<double, 50>;

class Time
{
	public:
	Time (); // Starts application timing

	void CollectRuntimeData (); // Ends application timing

	void StartFrameTimer ();
	void EndFrameTimer ();

	double ExactTimeSinceFrameStart ();

	double DeltaTime ();         // in seconds
	double RunningTime ();       // in seconds
	double PreviousFrameTime (); // in seconds (how long the last frame took

	BriefTimingHistory FrameTimeHistory ();
	double FrameTimeMax ();
	double FrameTimeMin ();

	private:
	using DoubleDuration = std::chrono::duration<double>;

	PreciseClockPoint applicationStartTime;
	PreciseClockPoint applicationEndTime;
	PreciseClockPoint frameStartTime;
	PreciseClockPoint frameEndTime;

	DoubleDuration curFrameTime;
	DoubleDuration prevFrameTime;

	BriefTimingHistory frameTimes{};
	double frameTimeMin = 999.0, frameTimeMax = 0.0;
};
