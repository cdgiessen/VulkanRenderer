#pragma once

#include <glm/fwd.hpp>
#include <memory>
#include <vector>

#include "Asset.h"

enum VertexType
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
	int ElementCount () const;

	std::vector<VertexType> layout; // each element in the array is a different attribute, its value (1-4) is it's size
};

static VertexDescription Vert_PosNorm = VertexDescription ({ Vert3, Vert3 });
static VertexDescription Vert_PosNormUv = VertexDescription ({ Vert3, Vert3, Vert2 });
static VertexDescription Vert_PosNormUvCol = VertexDescription ({ Vert3, Vert3, Vert2, Vert4 });

class MeshData
{
	public:
	MeshData (VertexDescription desc, std::vector<float> vertexData, std::vector<uint16_t> indexData)
	: desc (desc), vertexData (vertexData), indexData (indexData)
	{
	}

	// MeshData (VertexDescription desc, std::vector<float>&& vertexData, std::vector<uint16_t>&&
	// indexData) : desc (desc), vertexData (std::move (vertexData)), indexData (indexData)
	//{
	//}

	const VertexDescription desc;
	std::vector<float> vertexData;
	std::vector<uint16_t> indexData;

	private:
};

struct NonInterleavedMeshData
{
	struct FloatBufferData
	{
		FloatBufferData (VertexType type) : type (type) {}
		VertexType type;
		std::vector<float> data;
	};

	NonInterleavedMeshData (std::vector<VertexType> bufferTypes)
	{
		for (auto& t : bufferTypes)
		{
			buffers.push_back (FloatBufferData (t));
		}
	}


	std::vector<FloatBufferData> buffers;
};

extern std::shared_ptr<MeshData> createSinglePlane ();

extern std::shared_ptr<MeshData> createDoublePlane ();

extern std::shared_ptr<MeshData> createFlatPlane (int dim, glm::vec3 size);

extern std::shared_ptr<MeshData> createCube (int dim = 1);

extern std::shared_ptr<MeshData> createSphere (int dim = 10);

extern std::shared_ptr<MeshData> create_water_plane_subdiv (int levels = 3, int subdivs = 3);
namespace Resource
{

struct MeshAssetID
{
	AssetID id;
};

struct Mesh
{
};


class MeshManager
{
	public:
	// MeshManager();
	// ~MeshManager();

	MeshAssetID CreateNewMeshAsset (MeshData& data);

	MeshAssetID CreatePlane (int dim = 1);
	MeshAssetID createCube ();
	MeshAssetID createSphere ();
};

} // namespace Resource