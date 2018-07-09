#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "Mesh.h"

class MeshManager
{
public:
	MeshManager();
	~MeshManager();


private:
	std::unordered_map<int, std::unique_ptr<Mesh>> handles;

};

