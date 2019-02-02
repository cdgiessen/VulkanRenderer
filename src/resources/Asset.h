#pragma once

#include <stdint.h>

namespace Resource {

enum class AssetType : uint8_t{
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

struct AssetID {
	AssetID(uint32_t const& id): id(id){}

	bool operator==(AssetID const& right){
		return id == right.id;
	}
	bool operator!=(AssetID const& right){
		return !(*this == right);
	}

	uint32_t const id;
};



}