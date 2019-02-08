#include "Mesh.h"

#include <glm/glm.hpp>
#include <numeric>

#include <fx/gltf.h>

#include "core/CoreTools.h"
#include "core/Logger.h"

int VertexDescription::ElementCount () const
{
	return std::accumulate (std::begin (layout), std::end (layout), 0);
}

void AddPlane (std::vector<float>& verts,
    std::vector<uint16_t>& indices,
    int dim,
    int faceNum,
    glm::vec3 topLeft,
    glm::vec3 topRight,
    glm::vec3 bottomLeft,
    glm::vec3 bottomRight)
{

	glm::vec3 p1 = topLeft;
	glm::vec3 p2 = topRight;
	glm::vec3 p3 = bottomLeft;

	glm::vec3 t1 = p2 - p1;
	glm::vec3 t2 = p3 - p1;

	glm::vec3 normal = glm::cross (t1, t2);

	for (float i = 0; i <= dim; i++)
	{
		for (float j = 0; j <= dim; j++)
		{
			glm::vec3 pos = glm::mix (
			    glm::mix (topLeft, topRight, j / dim), glm::mix (bottomLeft, bottomRight, j / dim), i / dim);

			verts.push_back (pos.x);
			verts.push_back (pos.y);
			verts.push_back (pos.z);
			verts.push_back (normal.x);
			verts.push_back (normal.y);
			verts.push_back (normal.z);
			verts.push_back (i);
			verts.push_back (j);
		}
	}

	for (int i = 0; i < dim; i++)
	{
		for (int j = 0; j < dim; j++)
		{
			indices.push_back ((dim + 1) * (dim + 1) * faceNum + i * (dim + 1) + j);
			indices.push_back ((dim + 1) * (dim + 1) * faceNum + i * (dim + 1) + j + 1);
			indices.push_back ((dim + 1) * (dim + 1) * faceNum + (i + 1) * (dim + 1) + j);
			indices.push_back ((dim + 1) * (dim + 1) * faceNum + i * (dim + 1) + j + 1);
			indices.push_back ((dim + 1) * (dim + 1) * faceNum + (i + 1) * (dim + 1) + j + 1);
			indices.push_back ((dim + 1) * (dim + 1) * faceNum + (i + 1) * (dim + 1) + j);
		}
	}
}


std::shared_ptr<MeshData> createSinglePlane ()
{
	std::vector<float> verts;
	std::vector<uint16_t> indices;

	int dim = 1;

	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	AddPlane (
	    verts, indices, dim, 0, glm::vec3 (-1, 0, -1), glm::vec3 (-1, 0, 1), glm::vec3 (1, 0, -1), glm::vec3 (1, 0, 1));

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
};

std::shared_ptr<MeshData> createDoublePlane ()
{
	std::vector<float> verts;
	std::vector<uint16_t> indices;

	int dim = 1;

	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	AddPlane (
	    verts, indices, dim, 0, glm::vec3 (-1, 0, -1), glm::vec3 (-1, 0, 1), glm::vec3 (1, 0, -1), glm::vec3 (-1, 0, 1));
	AddPlane (
	    verts, indices, dim, 0, glm::vec3 (-1, 1, -1), glm::vec3 (-1, 1, 1), glm::vec3 (1, 1, -1), glm::vec3 (1, 1, 1));

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
};


std::shared_ptr<MeshData> createFlatPlane (int dim, glm::vec3 size)
{
	std::vector<float> verts;
	std::vector<uint16_t> indices;

	verts.resize ((dim + 1) * (dim + 1) * 8);
	indices.resize ((dim) * (dim)*6);

	for (int i = 0; i <= dim; i++)
	{
		for (int j = 0; j <= dim; j++)
		{
			verts[8 * ((i) * (dim + 1) + j) + 0] = (double)i * (size.x) / (float)dim;
			verts[8 * ((i) * (dim + 1) + j) + 1] = 0;
			verts[8 * ((i) * (dim + 1) + j) + 2] = (double)j * (size.z) / (float)dim;
			verts[8 * ((i) * (dim + 1) + j) + 3] = 0;
			verts[8 * ((i) * (dim + 1) + j) + 4] = 1;
			verts[8 * ((i) * (dim + 1) + j) + 5] = 0;
			verts[8 * ((i) * (dim + 1) + j) + 6] = i;
			verts[8 * ((i) * (dim + 1) + j) + 7] = j;
		}
	}

	int counter = 0;
	for (int i = 0; i < dim; i++)
	{
		for (int j = 0; j < dim; j++)
		{
			indices[counter++] = i * (dim + 1) + j;
			indices[counter++] = i * (dim + 1) + j + 1;
			indices[counter++] = (i + 1) * (dim + 1) + j;
			indices[counter++] = i * (dim + 1) + j + 1;
			indices[counter++] = (i + 1) * (dim + 1) + j + 1;
			indices[counter++] = (i + 1) * (dim + 1) + j;
		}
	}

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
}



std::shared_ptr<MeshData> createCube ()
{
	std::vector<float> verts;
	std::vector<uint16_t> indices;
	int dim = 1;
	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	glm::vec3 dlb{ -1, -1, -1 };
	glm::vec3 dlf{ -1, -1, 1 };
	glm::vec3 drb{ 1, -1, -1 };
	glm::vec3 drf{ 1, -1, 1 };
	glm::vec3 ulb{ -1, 1, -1 };
	glm::vec3 ulf{ -1, 1, 1 };
	glm::vec3 urb{ 1, 1, -1 };
	glm::vec3 urf{ 1, 1, 1 };


	AddPlane (verts, indices, dim, 0, urf, urb, ulf, ulb);
	AddPlane (verts, indices, dim, 1, dlf, dlb, drf, drb);
	AddPlane (verts, indices, dim, 2, drf, drb, urf, urb);
	AddPlane (verts, indices, dim, 3, ulf, ulb, dlf, dlb);
	AddPlane (verts, indices, dim, 4, ulf, dlf, urf, drf);
	AddPlane (verts, indices, dim, 5, urb, drb, ulb, dlb);

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
}

std::shared_ptr<MeshData> createSphere (int dim)
{

	std::vector<float> verts;
	std::vector<uint16_t> indices;

	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	glm::vec3 dlb{ -1, -1, -1 };
	glm::vec3 dlf{ -1, -1, 1 };
	glm::vec3 drb{ 1, -1, -1 };
	glm::vec3 drf{ 1, -1, 1 };
	glm::vec3 ulb{ -1, 1, -1 };
	glm::vec3 ulf{ -1, 1, 1 };
	glm::vec3 urb{ 1, 1, -1 };
	glm::vec3 urf{ 1, 1, 1 };


	AddPlane (verts, indices, dim, 0, urf, urb, ulf, ulb);
	AddPlane (verts, indices, dim, 1, dlf, dlb, drf, drb);
	AddPlane (verts, indices, dim, 2, drf, drb, urf, urb);
	AddPlane (verts, indices, dim, 3, ulf, ulb, dlf, dlb);
	AddPlane (verts, indices, dim, 4, ulf, dlf, urf, drf);
	AddPlane (verts, indices, dim, 5, urb, drb, ulb, dlb);

	for (int i = 0; i < verts.size () / 8; i++)
	{
		glm::vec3 pos = glm::normalize (glm::vec3 (verts[i + 0], verts[i + 1], verts[i + 2]));
		verts[i + 0] = pos.x;
		verts[i + 1] = pos.y;
		verts[i + 2] = pos.z;
		verts[i + 3] = pos.x;
		verts[i + 4] = pos.y;
		verts[i + 5] = pos.z;
	}

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
};