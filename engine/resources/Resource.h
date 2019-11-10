#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include "core/JobSystem.h"

#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

namespace Resource
{

enum class AssetType : uint16_t
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
	AssetID (uint16_t const& id, AssetType const type) : id (id), type (type) {}

	bool operator== (AssetID const& right) { return id == right.id && type == right.type; }
	bool operator!= (AssetID const& right) { return !(*this == right); }

	uint16_t const id;
	AssetType const type;
};
class AssetDescription
{
	public:
	AssetDescription (AssetID id, std::string const& name) : id (id), name (name) {}

	private:
	AssetID id;
	std::string name;
};

class AssetManager
{
	public:
	AssetManager (job::TaskManager& task_manager);
	~AssetManager ();



	Mesh::Manager mesh_manager;
	Texture::Manager texture_manager;
	Material::Manager material_manager;

	private:
	job::TaskManager& task_manager;
	std::atomic_int counter;
};


} // namespace Resource

namespace std
{

template <> struct hash<Resource::AssetID>
{
	std::size_t operator() (Resource::AssetID const& k) const
	{
		using std::size_t;
		return ((std::hash<uint16_t> () (k.id) ^ (std::hash<uint16_t> () (k.id) << 1)) >>
		        (std::hash<Resource::AssetType> () (k.type) ^ (std::hash<Resource::AssetType> () (k.type) << 1)));
	}
};

} // namespace std