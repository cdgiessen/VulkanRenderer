#pragma once

#include "../../third-party/ImGui/imgui.h"
// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");

#include <fstream>

namespace DebugLog {
	
	class CusLog {
	public:
		CusLog();
		CusLog(std::string logName);
		~CusLog();

		std::ofstream& GetLog();
		std::ostream& GetOldStream();

		void CaptureStream(std::ostream& buf);
		std::ostream* ReleaseStream();

	private:
		std::ofstream log;
		std::ostream oldStream;
	};

	extern CusLog log;
	
	void SetupCoutCapture();
	void ReleaseCoutCapture();

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
