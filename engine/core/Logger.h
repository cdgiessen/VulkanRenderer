#pragma once

#include <cstdio>
#include <fmt/format.h>
#include <string_view>

class Logger
{
	public:
	Logger ();
	void debug (std::string_view str_v);
	void error (std::string_view str_v);

	private:
	struct OutputFileHandle
	{
		OutputFileHandle (std::string file_name);
		~OutputFileHandle ();
		FILE* fp = nullptr;
	};

	OutputFileHandle fp_debug;
	OutputFileHandle fp_error;
};

extern Logger Log;