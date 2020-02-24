#include "Time.h"
#include <algorithm>

using namespace std::chrono;

Time::Time ()
{
	applicationStartTime = high_resolution_clock::now ();
	frameStartTime = high_resolution_clock::now ();
	frameEndTime = high_resolution_clock::now ();
	curFrameTime = (frameEndTime - frameStartTime);

	std::fill (std::begin (frameTimes), std::end (frameTimes), 0.01666);
}

void Time::CollectRuntimeData () { applicationEndTime = high_resolution_clock::now (); }

void Time::StartFrameTimer () { frameStartTime = high_resolution_clock::now (); }

void Time::EndFrameTimer ()
{
	frameEndTime = high_resolution_clock::now ();

	prevFrameTime = curFrameTime;
	curFrameTime = (frameEndTime - frameStartTime);

	// Update frame timings display
	{
		std::rotate (frameTimes.begin (), frameTimes.begin () + 1, frameTimes.end ());

		frameTimes.back () = (float)DeltaTime ();

		if (frameTimes.back () < frameTimeMin)
		{
			frameTimeMin = frameTimes.back ();
		}
		if (frameTimes.back () > frameTimeMax)
		{
			frameTimeMax = frameTimes.back ();
		}
	}
}

double Time::ExactTimeSinceFrameStart ()
{
	DoubleDuration time = high_resolution_clock::now () - frameStartTime;
	return time.count ();
}

double Time::DeltaTime ()
{
	DoubleDuration time = curFrameTime;
	return time.count ();
}


double Time::RunningTime ()
{
	DoubleDuration time = high_resolution_clock::now () - applicationStartTime;
	return time.count ();
}

BriefTimingHistory Time::FrameTimeHistory () { return frameTimes; }

double Time::PreviousFrameTime () { return prevFrameTime.count (); }

double Time::FrameTimeMax () { return frameTimeMax; }
double Time::FrameTimeMin () { return frameTimeMin; }
