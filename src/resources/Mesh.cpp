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
    std::vector<uint32_t>& indices,
    int dim,
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

	size_t offset = verts.size () / 8;
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
			indices.push_back (offset + i * (dim + 1) + j);
			indices.push_back (offset + i * (dim + 1) + j + 1);
			indices.push_back (offset + (i + 1) * (dim + 1) + j);
			indices.push_back (offset + i * (dim + 1) + j + 1);
			indices.push_back (offset + (i + 1) * (dim + 1) + j + 1);
			indices.push_back (offset + (i + 1) * (dim + 1) + j);
		}
	}
}


std::shared_ptr<MeshData> createSinglePlane ()
{
	std::vector<float> verts;
	std::vector<uint32_t> indices;

	int dim = 1;

	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	AddPlane (
	    verts, indices, dim, glm::vec3 (-1, 0, -1), glm::vec3 (-1, 0, 1), glm::vec3 (1, 0, -1), glm::vec3 (1, 0, 1));

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
};

std::shared_ptr<MeshData> createDoublePlane ()
{
	std::vector<float> verts;
	std::vector<uint32_t> indices;

	int dim = 1;

	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	AddPlane (
	    verts, indices, dim, glm::vec3 (-1, 0, -1), glm::vec3 (-1, 0, 1), glm::vec3 (1, 0, -1), glm::vec3 (-1, 0, 1));
	AddPlane (
	    verts, indices, dim, glm::vec3 (-1, 1, -1), glm::vec3 (-1, 1, 1), glm::vec3 (1, 1, -1), glm::vec3 (1, 1, 1));

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
};


std::shared_ptr<MeshData> createFlatPlane (int dim, glm::vec3 size)
{
	std::vector<float> verts;
	std::vector<uint32_t> indices;

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



std::shared_ptr<MeshData> createCube (int dim)
{
	std::vector<float> verts;
	std::vector<uint32_t> indices;
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


	AddPlane (verts, indices, dim, urf, urb, ulf, ulb);
	AddPlane (verts, indices, dim, dlf, dlb, drf, drb);
	AddPlane (verts, indices, dim, drf, drb, urf, urb);
	AddPlane (verts, indices, dim, ulf, ulb, dlf, dlb);
	AddPlane (verts, indices, dim, ulf, dlf, urf, drf);
	AddPlane (verts, indices, dim, urb, drb, ulb, dlb);

	return std::make_shared<MeshData> (Vert_PosNormUv, verts, indices);
}

std::shared_ptr<MeshData> createSphere (int dim)
{
	auto cube = createCube (dim);

	// normalize it so all vertexes are equidistant from the center
	for (int i = 0; i < cube->vertexData.size (); i += 8)
	{
		glm::vec3 pos = glm::normalize (glm::vec3 (
		    cube->vertexData.at (i + 0), cube->vertexData.at (i + 1), cube->vertexData.at (i + 2)));
		cube->vertexData.at (i + 0) = pos.x;
		cube->vertexData.at (i + 1) = pos.y;
		cube->vertexData.at (i + 2) = pos.z;
		cube->vertexData.at (i + 3) = pos.x;
		cube->vertexData.at (i + 4) = pos.y;
		cube->vertexData.at (i + 5) = pos.z;
	}

	return cube;
};

void Add_subdiv_triangle_no_seam_fix (
    std::vector<float>& verts, std::vector<uint32_t>& indices, glm::vec3 top, glm::vec3 left, glm::vec3 down, int dim)
{
	uint32_t offset = verts.size () / 3;
	for (float i = 0; i < dim; i++)
	{
		for (float j = 0; j < dim + 1 - i; j++)
		{
			glm::vec3 pos = glm::mix (glm::mix (top, left, j / (dim - i)), down, i / (dim));

			verts.push_back (pos.x);
			verts.push_back (pos.y);
			verts.push_back (pos.z);
		}
	}

	// i/dim is a NaN, better to just duplicate it once below for the last vertex
	glm::vec3 pos = down;

	verts.push_back (pos.x);
	verts.push_back (pos.y);
	verts.push_back (pos.z);


	int base = 0;
	int next_base = dim + 1;
	for (int i = 0; i < dim; i++)
	{
		int across = dim - i;

		indices.push_back (offset + base);
		indices.push_back (offset + base + 1);
		indices.push_back (offset + next_base);

		for (int j = 1; j < dim - i; j++)
		{
			indices.push_back (offset + base + j);
			indices.push_back (offset + next_base + j);
			indices.push_back (offset + next_base + j - 1);

			indices.push_back (offset + base + j);
			indices.push_back (offset + base + j + 1);
			indices.push_back (offset + next_base + j);
		}
		base = next_base;
		next_base += across;
	}
}


void Add_subdiv_triangle (
    std::vector<float>& verts, std::vector<uint32_t>& indices, glm::vec3 top, glm::vec3 left, glm::vec3 down, int dim)
{
	uint32_t offset = verts.size () / 3;
	for (float i = 0; i < dim; i++)
	{
		for (float j = 0; j < dim + 1 - i; j++)
		{
			glm::vec3 pos = glm::mix (glm::mix (top, left, j / (dim - i)), down, i / (dim));

			verts.push_back (pos.x);
			verts.push_back (pos.y);
			verts.push_back (pos.z);
		}

		// seam fix vertex
		glm::vec3 pos = glm::mix (glm::mix (top, left, (dim - i) / (dim - i)), down, (i + 0.5) / (dim));

		verts.push_back (pos.x);
		verts.push_back (pos.y);
		verts.push_back (pos.z);
	}

	// i/dim is a NaN, better to just duplicate it once below for the last vertex
	glm::vec3 pos = down;

	verts.push_back (pos.x);
	verts.push_back (pos.y);
	verts.push_back (pos.z);

	int base = 0;
	int next_base = dim + 2;
	for (int i = 0; i < dim; i++)
	{
		int across = dim - i + 1;
		for (int j = 0; j < dim - i - 1; j++)
		{
			indices.push_back (offset + base + j);
			indices.push_back (offset + base + j + 1);
			indices.push_back (offset + next_base + j + 1);

			indices.push_back (offset + base + j);
			indices.push_back (offset + next_base + j + 1);
			indices.push_back (offset + next_base + j);
		}

		indices.push_back (offset + base + across - 2);
		indices.push_back (offset + base + across - 1);
		indices.push_back (offset + base + across);

		indices.push_back (offset + base + across - 2);
		indices.push_back (offset + base + across);
		indices.push_back (offset + next_base + across - 2);

		base = next_base;
		next_base += across;
	}
}


// subdivide triangle into 4 segments, and recursively subdivide the center one
void subdiv_triangle (std::vector<float>& verts,
    std::vector<uint32_t>& indices,
    glm::vec3 top,
    glm::vec3 bottom_left,
    glm::vec3 bottom_right,
    int levels,
    int subdivs)
{
	Add_subdiv_triangle (
	    verts, indices, top, glm::mix (top, bottom_left, 0.5), glm::mix (top, bottom_right, 0.5), subdivs);
	Add_subdiv_triangle (
	    verts, indices, bottom_left, glm::mix (bottom_left, bottom_right, 0.5), glm::mix (top, bottom_left, 0.5), subdivs);
	Add_subdiv_triangle (
	    verts, indices, bottom_right, glm::mix (top, bottom_right, 0.5), glm::mix (bottom_left, bottom_right, 0.5), subdivs);

	if (levels == 0)
	{
		Add_subdiv_triangle_no_seam_fix (verts,
		    indices,
		    glm::mix (top, bottom_left, 0.5),
		    glm::mix (bottom_left, bottom_right, 0.5),
		    glm::mix (top, bottom_right, 0.5),
		    subdivs * 2);
	}
	else
	{
		subdiv_triangle (verts,
		    indices,
		    glm::mix (top, bottom_left, 0.5),
		    glm::mix (bottom_left, bottom_right, 0.5),
		    glm::mix (top, bottom_right, 0.5),
		    levels - 1,
		    subdivs);
	}
}

std::shared_ptr<MeshData> create_water_plane_subdiv (int levels, int subdivs)
{

	std::vector<float> verts;
	std::vector<uint32_t> indices;

	float max_float = 5000000.0f;

	float neg_vert_offset = (max_float / 2.0f) * glm::tan (glm::radians (30.0f));
	float pos_vert_offset = -(max_float / 2.0f) / glm::cos (glm::radians (30.0f));

	glm::vec3 top{ 0, 0, pos_vert_offset };
	glm::vec3 bottom_left{ -(max_float / 2.0f), 0, neg_vert_offset };
	glm::vec3 bottom_right{ (max_float / 2.0f), 0, neg_vert_offset };

	subdiv_triangle (verts, indices, top, bottom_left, bottom_right, levels, subdivs);

	return std::make_shared<MeshData> (Vert_Pos, verts, indices);
}