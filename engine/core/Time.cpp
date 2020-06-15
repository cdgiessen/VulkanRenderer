#include "Time.h"
#include <algorithm>

using namespace std::chrono;

Time::Time ()
{
	application_start_time = high_resolution_clock::now ();
	frame_start_time = high_resolution_clock::now ();
	frame_end_time = high_resolution_clock::now ();
	cur_frame_time = (frame_end_time - frame_start_time);

	std::fill (std::begin (frame_times), std::end (frame_times), 0.01666);
}

void Time::collect_runtime_data () { application_end_time = high_resolution_clock::now (); }

void Time::start_frame_timer () { frame_start_time = high_resolution_clock::now (); }

void Time::end_frame_timer ()
{
	frame_end_time = high_resolution_clock::now ();

	prev_frame_time = cur_frame_time;
	cur_frame_time = (frame_end_time - frame_start_time);

	// Update frame timings display
	{
		std::rotate (frame_times.begin (), frame_times.begin () + 1, frame_times.end ());

		frame_times.back () = (float)delta_time ();

		if (frame_times.back () < m_frame_time_min)
		{
			m_frame_time_min = frame_times.back ();
		}
		if (frame_times.back () > m_frame_time_max)
		{
			m_frame_time_max = frame_times.back ();
		}
	}
}

double Time::exact_time_since_frame_start ()
{
	DoubleDuration time = high_resolution_clock::now () - frame_start_time;
	return time.count ();
}

double Time::delta_time ()
{
	DoubleDuration time = cur_frame_time;
	return time.count ();
}


double Time::running_time ()
{
	DoubleDuration time = high_resolution_clock::now () - application_start_time;
	return time.count ();
}

BriefTimingHistory Time::frame_time_history () { return frame_times; }

double Time::previous_frame_time () { return prev_frame_time.count (); }

double Time::frame_time_max () { return m_frame_time_max; }
double Time::frame_time_min () { return m_frame_time_min; }
