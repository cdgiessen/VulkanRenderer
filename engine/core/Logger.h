#pragma once

#include <cstdio>
#include <fmt/format.h>
#include <string_view>


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
		OutputFileHandle (std::string file_name);
		~OutputFileHandle ();
		FILE* fp = nullptr;
	};

	OutputFileHandle fp_debug;
	OutputFileHandle fp_error;
};

extern Logger Log;