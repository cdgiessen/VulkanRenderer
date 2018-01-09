#pragma once

#include <vector>
#include <memory>


#include "TextureManager.h"
#include "MeshManager.h"

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	TextureManager texManager;
	MeshManager meshManager;

private:

};

