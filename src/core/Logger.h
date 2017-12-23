#pragma once

#include "../../third-party/ImGui/imgui.h"
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

	void AddLog(const char* fmt, ...) IM_PRINTFARGS(2);

	void Draw(const char* title, bool* p_open = NULL);
};

