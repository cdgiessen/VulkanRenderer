#pragma once

#include <cstdio>
#include <fmt/format.h>
#include <mutex>
#include <string_view>

// namespace Log
// {


class Logger
{
	public:
	Logger ();
	void Debug (std::string_view str_v);
	void Error (std::string_view str_v);

	private:
	class OutputFileHandle
	{
		public:
		OutputFileHandle (std::string file_name)
		{
			fp = std::fopen (file_name.c_str (), "w");
			if (!fp)
			{
				fmt::print ("File opening failed");
			}
		}
		~OutputFileHandle () { std::fclose (fp); }
		FILE* fp = nullptr;
	};

	OutputFileHandle fp_debug;
	OutputFileHandle fp_error;
};

// class Log
// {
// 	public:
// 	Log (std::string fileOut, std::streambuf* consoleOut);

// 	std::ofstream fileOut;
// 	std::ostream consoleOut;

// 	std::mutex log_lock;
// };

// template <typename T> Log& operator<< (Log& log, T const& stream)
// {
// 	std::lock_guard<std::mutex> lg (log.log_lock);
// 	log.consoleOut << stream;
// 	log.fileOut << stream;
// 	return log;
// }

extern Logger Log;
// extern Log Debug;

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
// class Logger
//{
//	private:
//	ImGuiTextBuffer Buf;
//	ImGuiTextFilter Filter;
//	ImVector<int> LineOffsets; // Index to lines offset
//	bool ScrollToBottom;
//
//	public:
//	void Clear ();
//
//	// void AddLog(const char* fmt, ...);
//
//	void Draw (const char* title, bool* p_open = NULL);
//};

//} // namespace Log
