#pragma once

#include "../../third-party/ImGui/imgui.h"

#include <iosfwd>
#include <fstream>
#include <ostream> 

namespace Log {
	
	class Log {
	public:
		Log(std::string fileOut, std::streambuf* consoleOut);

		std::ofstream fileOut;
		std::ostream consoleOut;
	};

	template<typename T>
	Log& operator << (Log& log, T const& stream) {
		log.consoleOut << stream;
		log.fileOut << stream;
		return log;
	}

	extern Log Error;
	extern Log Debug;

	// Usage:
	//  static ExampleAppLog my_log;
	//  my_log.AddLog("Hello %d world\n", 123);
	//  my_log.Draw("title");
	class Logger
	{
	private:
		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets;        // Index to lines offset
		bool                ScrollToBottom;
	public:
		void Clear();
	
		//void AddLog(const char* fmt, ...);
	
		void Draw(const char* title, bool* p_open = NULL);
	};

}
