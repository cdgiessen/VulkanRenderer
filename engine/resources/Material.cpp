#include "Material.h"

#include <nlohmann/json.hpp>
namespace Resource::Material
{

int GetDataMemeberSize (DataMemberType type)
{
	switch (type)
	{
		case DataMemberType::Bool:
			return 1;
		case DataMemberType::Int:
			return 1;
		case DataMemberType::Float:
			return 1;
		case DataMemberType::Vec2f:
			return 2;
		case DataMemberType::Vec3f:
			return 3;
		case DataMemberType::Vec4f:
			return 4;
		case DataMemberType::Vec2i:
			return 2;
		case DataMemberType::Vec3i:
			return 3;
		case DataMemberType::Vec4i:
			return 4;
		case DataMemberType::Mat3:
			return 9;
		case DataMemberType::Mat4:
			return 16;
	}
	return 0;
}


Manager::Manager () {}

Manager::~Manager () {}

} // namespace Resource::Material