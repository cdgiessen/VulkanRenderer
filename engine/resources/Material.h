#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "cml/cml.h"

#include "Texture.h"

namespace Resource::Material
{
enum class DataMemberType
{
	Bool,
	Int,
	Float,
	Vec2f,
	Vec3f,
	Vec4f,
	Vec2i,
	Vec3i,
	Vec4i,
	Mat3,
	Mat4
};
struct DataMemberOutline
{
	DataMemberType type;
	std::string name;
};

int GetDataMemeberSize (DataMemberType type);

struct TextureMemberOutline
{
	Texture::TextureType type;
	std::string name;
};

using MatOutlineID = int;
using MatInstanceID = int;

struct MaterialOutline
{
	MatOutlineID const id;
	std::string name;
	std::vector<DataMemberOutline> texture_members;
	std::vector<TextureMemberOutline> data_members;
};

struct DataMember
{
	using DataVariant =
	    std::variant<bool, int32_t, float, cml::vec2f, cml::vec3f, cml::vec4f, cml::vec2i, cml::vec3i, cml::vec4i>;
	DataVariant data;
};


struct MaterialInstance
{
	MatInstanceID const id;
	MatOutlineID const outline_id;
	std::string const name;
	std::vector<DataMember> data_members;
	std::vector<Texture::TexID> tex_members;
};
class Materials
{
	public:
	Materials ();
	~Materials ();

	MatOutlineID create_material_outline ();


	private:
	MatOutlineID outline_counter = 0;
	std::unordered_map<MatOutlineID, MaterialOutline> outlines;
};
} // namespace Resource::Material