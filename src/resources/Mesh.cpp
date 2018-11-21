#include "Mesh.h"

#include <glm/glm.hpp>
#include <numeric>

#include "core/CoreTools.h"
#include "core/Logger.h"

int VertexDescription::ElementCount () const
{
	return std::accumulate (std::begin (layout), std::end (layout), 0);
}


std::shared_ptr<MeshData> createSinglePlane ()
{


	return std::make_shared<MeshData> (VertexDescription ({ 3, 3, 2 }),
	    std::vector<float> ({
	        // vertices

	        -0.5f,
	        -0.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        0.0f,
	        0.5f,
	        0.5f,
	        0.0f,
	        0.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.0f,
	        1.0f

	        // indices
	    }),
	    std::vector<uint16_t> ({ 0, 1, 2, 2, 3, 0 }));
};

std::shared_ptr<MeshData> createDoublePlane ()
{
	return std::make_shared<MeshData> (VertexDescription ({ 3, 3, 2 }),
	    std::vector<float> ({
	        // vertices

	        -0.5f,
	        -0.5f,
	        1.0f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.5f,
	        -0.5f,
	        1.0f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        0.0f,
	        0.5f,
	        0.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.0f,
	        1.0f,

	        -0.5f,
	        -0.5f,
	        -1.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.5f,
	        -0.5f,
	        -1.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        0.0f,
	        0.5f,
	        0.5f,
	        -1.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        -1.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.0f,
	        1.0f

	        // indices
	    }),
	    std::vector<uint16_t> ({ 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 }));
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

	return std::make_shared<MeshData> (VertexDescription ({ 3, 3, 2 }), verts, indices);
}

std::shared_ptr<MeshData> createCube ()
{
	return std::make_shared<MeshData> (VertexDescription ({ 3, 3, 2, 4 }),
	    std::vector<float> ({

	        // Left face
	        0.5f,
	        -0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        -1.0f,
	        0.333f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        -1.0f,
	        0.667f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        -1.0f,
	        0.333f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        -1.0f,
	        0.667f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        -1.0f,
	        0.333f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        -0.5f,
	        0.0f,
	        0.0f,
	        -1.0f,
	        0.667f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,

	        // Right face
	        0.5f,
	        0.5f,
	        0.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        0.667f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        0.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        0.333f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        -0.5f,
	        0.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        0.667f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        0.5f,
	        0.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        0.667f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        0.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        0.333f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        0.5f,
	        0.0f,
	        0.0f,
	        1.0f,
	        0.333f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,

	        // Back face
	        -0.5f,
	        -0.5f,
	        -0.5f,
	        -1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        0.5f,
	        -1.0f,
	        0.0f,
	        0.0f,
	        0.333f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        0.5f,
	        -1.0f,
	        0.0f,
	        0.0f,
	        0.333f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        0.5f,
	        -1.0f,
	        0.0f,
	        0.0f,
	        0.333f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        -0.5f,
	        -1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        -0.5f,
	        -1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,

	        // Front face
	        0.5f,
	        0.5f,
	        -0.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.334f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        0.5f,
	        0.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        -0.5f,
	        -0.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.334f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        -0.5f,
	        0.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        -0.5f,
	        -0.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.334f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        0.5f,
	        0.5f,
	        1.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,

	        // Bottom face
	        0.5f,
	        -0.5f,
	        0.5f,
	        0.0f,
	        -1.0f,
	        0.0f,
	        0.667f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        -0.5f,
	        0.0f,
	        -1.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        -0.5f,
	        -0.5f,
	        0.0f,
	        -1.0f,
	        0.0f,
	        0.667f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        -0.5f,
	        0.5f,
	        0.0f,
	        -1.0f,
	        0.0f,
	        0.667f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        0.5f,
	        0.0f,
	        -1.0f,
	        0.0f,
	        1.0f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        -0.5f,
	        -0.5f,
	        0.0f,
	        -1.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,

	        // Top face
	        0.5f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        0.667f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        0.5f,
	        0.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        0.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        0.667f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        0.5f,
	        0.5f,
	        0.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        0.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f,
	        -0.5f,
	        0.5f,
	        -0.5f,
	        0.0f,
	        1.0f,
	        0.0f,
	        0.667f,
	        0.5f,
	        1.0f,
	        1.0f,
	        1.0f,
	        1.0f


	        // indices
	    }),
	    std::vector<uint16_t> (
	        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 }));
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

std::shared_ptr<MeshData> createSphere (int dim)
{

	std::vector<float> verts;
	std::vector<uint16_t> indices;

	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	AddPlane (verts,
	    indices,
	    dim,
	    0,
	    glm::vec3 (0.5, 0.5, 0.5),
	    glm::vec3 (0.5, 0.5, -0.5),
	    glm::vec3 (-0.5, 0.5, 0.5),
	    glm::vec3 (-0.5, 0.5, -0.5));
	AddPlane (verts,
	    indices,
	    dim,
	    1,
	    glm::vec3 (-0.5, -0.5, 0.5),
	    glm::vec3 (-0.5, -0.5, -0.5),
	    glm::vec3 (0.5, -0.5, 0.5),
	    glm::vec3 (0.5, -0.5, -0.5));
	AddPlane (verts,
	    indices,
	    dim,
	    2,
	    glm::vec3 (0.5, -0.5, 0.5),
	    glm::vec3 (0.5, -0.5, -0.5),
	    glm::vec3 (0.5, 0.5, 0.5),
	    glm::vec3 (0.5, 0.5, -0.5));
	AddPlane (verts,
	    indices,
	    dim,
	    3,
	    glm::vec3 (-0.5, 0.5, 0.5),
	    glm::vec3 (-0.5, 0.5, -0.5),
	    glm::vec3 (-0.5, -0.5, 0.5),
	    glm::vec3 (-0.5, -0.5, -0.5));
	AddPlane (verts,
	    indices,
	    dim,
	    4,
	    glm::vec3 (-0.5, 0.5, 0.5),
	    glm::vec3 (-0.5, -0.5, 0.5),
	    glm::vec3 (0.5, 0.5, 0.5),
	    glm::vec3 (0.5, -0.5, 0.5));
	AddPlane (verts,
	    indices,
	    dim,
	    5,
	    glm::vec3 (0.5, 0.5, -0.5),
	    glm::vec3 (0.5, -0.5, -0.5),
	    glm::vec3 (-0.5, 0.5, -0.5),
	    glm::vec3 (-0.5, -0.5, -0.5));

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

	return std::make_shared<MeshData> (VertexDescription ({ 3, 3, 2 }), verts, indices);
};