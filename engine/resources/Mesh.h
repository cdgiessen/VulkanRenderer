#pragma once

#include <unordered_map>
#include <vector>

#include "cml/cml.h"


namespace job
{
class ThreadPool;
}
namespace Resource::Mesh
{

enum class VertexType
{
	Vert1 = 1,
	Vert2 = 2,
	Vert3 = 3,
	Vert4 = 4,
};

class VertexDescription
{
	public:
	VertexDescription (std::vector<VertexType> layout) : layout (layout){};
	int element_count () const;

	std::vector<VertexType> layout; // each element in the array is a different attribute, its value (1-4) is it's size
};

struct MeshData
{
	MeshData (VertexDescription desc, std::vector<float> vertexData, std::vector<uint32_t> indexData)
	: desc (desc), vertexData (vertexData), indexData (indexData)
	{
	}

	const VertexDescription desc;
	std::vector<float> vertexData;
	std::vector<uint32_t> indexData;
};

struct DeInterleavedMeshData
{
	struct FloatBufferData
	{
		FloatBufferData (VertexType type) : type (type) {}
		VertexType type;
		std::vector<float> data;
	};

	DeInterleavedMeshData (std::vector<VertexType> bufferTypes)
	{
		for (auto& t : bufferTypes)
		{
			buffers.push_back (FloatBufferData (t));
		}
	}

	std::vector<FloatBufferData> buffers;
};

MeshData create_flat_plane (int dim, cml::vec3f size);

MeshData create_cube (int dim = 1);

MeshData create_sphere (int dim = 10);

MeshData create_water_plane_sub_div (int levels = 3, int subdivs = 3);

using MeshID = uint32_t;
class Meshes
{
	public:
	Meshes (job::ThreadPool& thread_pool);

	MeshID LoadMesh ();

	private:
	job::ThreadPool& thread_pool;
	std::unordered_map<MeshID, MeshData> meshes;
};



} // namespace Resource::Mesh