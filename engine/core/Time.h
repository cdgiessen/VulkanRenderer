#pragma once
#include <array>
#include <chrono>

using PreciseClockPoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using BriefTimingHistory = std::array<double, 50>;

class Time
{
	public:
	Time (); // Starts application timing

	void collect_runtime_data (); // Ends application timing

	void start_frame_timer ();
	void end_frame_timer ();

	double exact_time_since_frame_start ();

	double delta_time ();          // in seconds
	double running_time ();        // in seconds
	double previous_frame_time (); // in seconds (how long the last frame took

	BriefTimingHistory frame_time_history ();
	double frame_time_max ();
	double frame_time_min ();

	private:
	using DoubleDuration = std::chrono::duration<double>;

	PreciseClockPoint application_start_time;
	PreciseClockPoint application_end_time;
	PreciseClockPoint frame_start_time;
	PreciseClockPoint frame_end_time;

	DoubleDuration cur_frame_time;
	DoubleDuration prev_frame_time;

	BriefTimingHistory frame_times{};
	double m_frame_time_min = 999.0, m_frame_time_max = 0.0;
};
