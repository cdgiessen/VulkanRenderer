#pragma once

#include <atomic>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

namespace Resource
{

enum class AssetType : uint8_t
{
	unspecified = 0,
	texture,
	mesh,
	material,
	sound,
	text,
	animation,
	json,
	gltf
};

struct AssetID
{
	AssetID (uint32_t const& id) : id (id) {}

	bool operator== (AssetID const& right) { return id == right.id; }
	bool operator!= (AssetID const& right) { return !(*this == right); }

	uint32_t const id;
};

class Asset
{
	public:
	Asset (AssetID id, std::string const& name) : id (id), name (name) {}

	private:
	AssetID id;
	std::string name;
};

class AssetManager
{
	public:
	AssetManager ();
	~AssetManager ();


	Mesh::Manager mesh_manager;
	Texture::Manager texture_manager;
	MaterialManager matManager;

	private:
	std::atomic_int counter;
};


} // namespace Resource