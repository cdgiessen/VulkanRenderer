#pragma once

#include <memory>
#include <vector>

#include <glm/fwd.hpp>

class VertexDescription
{
	public:
	VertexDescription (std::vector<int> layout) : layout (layout){};
	int ElementCount () const;

	std::vector<int> layout; // each element in the array is a different attribute, its value (1-4) is it's size
};



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



extern std::shared_ptr<MeshData> createSinglePlane ();

extern std::shared_ptr<MeshData> createDoublePlane ();

extern std::shared_ptr<MeshData> createFlatPlane (int dim, glm::vec3 size);

extern std::shared_ptr<MeshData> createCube ();


extern std::shared_ptr<MeshData> createSphere (int dim = 10);