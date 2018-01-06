#include "CoreTools.h"

std::string executableFilePath;

std::string GetFilePathFromFullPath(std::string file) {
	std::string path = file;
	#ifdef _WIN32
	auto pos = path.rfind("\\");
	#else
	auto pos = path.rfind("/");
	#endif // _WIN32
	path.erase(pos, path.size() - pos);

	return path;
}

void SetExecutableFilePath(std::string file) {
	executableFilePath = GetFilePathFromFullPath(file);
}

std::string GetExecutableFilePath() {
	return executableFilePath;
}
