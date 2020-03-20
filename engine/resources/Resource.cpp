#include "Resource.h"

#include <filesystem>

#include "core/JobSystem.h"
#include "core/Logger.h"
namespace Resource
{

Resources::Resources (job::ThreadPool& thread_pool)
: thread_pool (thread_pool), meshes (thread_pool), textures (thread_pool), shaders (thread_pool)
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


Resources::~Resources () {}

} // namespace Resource