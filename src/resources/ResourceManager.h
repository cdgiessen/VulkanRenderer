#pragma once

#include <vector>
#include <memory>

#include "Material.h"
#include "Texture.h"
#include "Mesh.h"

namespace Resource {


class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	Texture::Manager texManager;
	MeshManager meshManager;
	MaterialManager matManager;

private:

};


}