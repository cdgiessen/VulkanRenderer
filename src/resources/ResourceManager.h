#pragma once

#include <vector>
#include <memory>

#include "Material.h"
#include "Texture.h"
#include "Mesh.h"

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	TextureManager texManager;
	MeshManager meshManager;
	MaterialManager matManager;

private:

};

