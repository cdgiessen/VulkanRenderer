#include "Resource.h"

#include <filesystem>

namespace Resource
{


AssetManager::AssetManager (job::TaskManager& task_manager)
: task_manager (task_manager), texture_manager (task_manager)
{
	std::filesystem::path ("assets");
}


AssetManager::~AssetManager () {}

} // namespace Resource