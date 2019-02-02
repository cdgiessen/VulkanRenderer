#pragma once

#include <atomic>
#include <vector>
#include <memory>
#include <string>

#include "Asset.h"

#include "Material.h"
#include "Texture.h"
#include "Mesh.h"

namespace Resource {

class Asset {
public:
	Asset(AssetID id, std::string const& name): id(id), name(name) {}

	private:
	AssetID id;
	std::string name;

};

class AssetManager
{
public:
	AssetManager();
	~AssetManager();

	AssetID create_asset_handle(AssetType const type, std::string const& name);



	Texture::Manager texManager;
	MeshManager meshManager;
	MaterialManager matManager;

private:
	std::atomic_int counter;

};


}