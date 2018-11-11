#pragma once

#include <vector>
#include <memory>

#include "Material.h"
#include "Texture.h"
#include "Mesh.h"

namespace Resource {


class AssetManager
{
public:
	AssetManager();
	~AssetManager();

	Texture::Manager texManager;
	//MeshManager meshManager;
	MaterialManager matManager;

private:

};


}