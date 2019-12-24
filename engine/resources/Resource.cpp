#include "Resource.h"

#include <filesystem>

#include "core/JobSystem.h"
#include "core/Logger.h"
namespace Resource
{

ResourceManager::ResourceManager (job::TaskManager& task_manager)
: task_manager (task_manager), mesh_manager (task_manager), texture_manager (task_manager), shader_manager (task_manager)
{
	Log.Debug ("Creating asset directories");

	std::vector<std::string> dirs_to_create = {
		"assets", "assets/meshes", "assets/materials", "assets/textures", "assets/mesh", "assets/sounds", "assets/gltf", "assets/materials", ".cache"

	};
	for (auto& dir_name : dirs_to_create)
	{
		if (!std::filesystem::exists (dir_name))
		{
			std::filesystem::path asset_path (dir_name);
			if (std::filesystem::create_directory (asset_path))
			{
				Log.Error (fmt::format ("Failed to create directory {}", dir_name));
			}
		}
	}
}


ResourceManager::~ResourceManager () {}

} // namespace Resource