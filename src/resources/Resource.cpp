#include "Resource.h"

#include <filesystem>

namespace Resource
{


AssetManager::AssetManager () { std::filesystem::path ("assets"); }


AssetManager::~AssetManager () {}

} // namespace Resource