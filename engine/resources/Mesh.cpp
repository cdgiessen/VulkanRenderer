#include "Mesh.h"

#include "cml/cml.h"
#include <numeric>

#include "core/Logger.h"

namespace Resource::Mesh
{

int VertexDescription::ElementCount () const
{
	return std::accumulate (std::begin (layout), std::end (layout), 0);
}

void AddPlane (std::vector<float>& verts,
    std::vector<uint32_t>& indices,
    int dim,
    cml::vec3f topLeft,
    cml::vec3f topRight,
    cml::vec3f bottomLeft,
    cml::vec3f bottomRight)
{

	cml::vec3f p1 = topLeft;
	cml::vec3f p2 = topRight;
	cml::vec3f p3 = bottomLeft;

	cml::vec3f t1 = p2 - p1;
	cml::vec3f t2 = p3 - p1;

	cml::vec3f normal = cml::cross (t1, t2);

	size_t offset = verts.size () / 8;
	for (float i = 0; i <= dim; i++)
	{
		for (float j = 0; j <= dim; j++)
		{
			cml::vec3f pos = cml::lerp (
			    cml::lerp (topLeft, topRight, j / dim), cml::lerp (bottomLeft, bottomRight, j / dim), i / dim);

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

MeshData CreateFlatPlane (int dim, cml::vec3f size)
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

	return MeshData (Vert_PosNormUv, verts, indices);
}



MeshData CreateCube (int dim)
{
	std::vector<float> verts;
	std::vector<uint32_t> indices;
	verts.reserve ((dim + 1) * (dim + 1) * 6);
	indices.reserve ((dim) * (dim)*6 * 6);

	cml::vec3f dlb{ -1, -1, -1 };
	cml::vec3f dlf{ -1, -1, 1 };
	cml::vec3f drb{ 1, -1, -1 };
	cml::vec3f drf{ 1, -1, 1 };
	cml::vec3f ulb{ -1, 1, -1 };
	cml::vec3f ulf{ -1, 1, 1 };
	cml::vec3f urb{ 1, 1, -1 };
	cml::vec3f urf{ 1, 1, 1 };


	AddPlane (verts, indices, dim, urf, urb, ulf, ulb);
	AddPlane (verts, indices, dim, dlf, dlb, drf, drb);
	AddPlane (verts, indices, dim, drf, drb, urf, urb);
	AddPlane (verts, indices, dim, ulf, ulb, dlf, dlb);
	AddPlane (verts, indices, dim, ulf, dlf, urf, drf);
	AddPlane (verts, indices, dim, urb, drb, ulb, dlb);

	return MeshData (Vert_PosNormUv, verts, indices);
}

MeshData CreateSphere (int dim)
{
	auto cube = CreateCube (dim);

	// normalize it so all vertexes are equidistant from the center
	for (int i = 0; i < cube.vertexData.size (); i += 8)
	{
		cml::vec3f pos = cml::normalize (cml::vec3f (
		    cube.vertexData.at (i + 0), cube.vertexData.at (i + 1), cube.vertexData.at (i + 2)));
		cube.vertexData.at (i + 1) = pos.y;
		cube.vertexData.at (i + 0) = pos.x;
		cube.vertexData.at (i + 2) = pos.z;
		cube.vertexData.at (i + 3) = pos.x;
		cube.vertexData.at (i + 4) = pos.y;
		cube.vertexData.at (i + 5) = pos.z;
	}

	return cube;
};

void Add_subdiv_triangle_no_seam_fix (
    std::vector<float>& verts, std::vector<uint32_t>& indices, cml::vec3f top, cml::vec3f left, cml::vec3f down, int dim)
{
	uint32_t offset = verts.size () / 3;
	for (float i = 0; i < dim; i++)
	{
		for (float j = 0; j < dim + 1 - i; j++)
		{
			cml::vec3f pos = cml::lerp (cml::lerp (top, left, j / (dim - i)), down, i / (dim));

			verts.push_back (pos.x);
			verts.push_back (pos.y);
			verts.push_back (pos.z);
		}
	}

	// i/dim is a NaN, better to just duplicate it once below for the last vertex
	cml::vec3f pos = down;

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
    std::vector<float>& verts, std::vector<uint32_t>& indices, cml::vec3f top, cml::vec3f left, cml::vec3f down, int dim)
{
	uint32_t offset = verts.size () / 3;
	for (float i = 0; i < dim; i++)
	{
		for (float j = 0; j < dim + 1 - i; j++)
		{
			cml::vec3f pos = cml::lerp (cml::lerp (top, left, j / (dim - i)), down, i / (dim));

			verts.push_back (pos.x);
			verts.push_back (pos.y);
			verts.push_back (pos.z);
		}

		// seam fix vertex
		cml::vec3f pos =
		    cml::lerp (cml::lerp (top, left, (dim - i) / (dim - i)), down, (float)(i + 0.5f) / (dim));

		verts.push_back (pos.x);
		verts.push_back (pos.y);
		verts.push_back (pos.z);
	}

	// i/dim is a NaN, better to just duplicate it once below for the last vertex
	cml::vec3f pos = down;

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
    cml::vec3f top,
    cml::vec3f bottom_left,
    cml::vec3f bottom_right,
    int levels,
    int subdivs)
{
	Add_subdiv_triangle (
	    verts, indices, top, cml::lerp (top, bottom_left, 0.5f), cml::lerp (top, bottom_right, 0.5f), subdivs);
	Add_subdiv_triangle (
	    verts, indices, bottom_left, cml::lerp (bottom_left, bottom_right, 0.5f), cml::lerp (top, bottom_left, 0.5f), subdivs);
	Add_subdiv_triangle (
	    verts, indices, bottom_right, cml::lerp (top, bottom_right, 0.5f), cml::lerp (bottom_left, bottom_right, 0.5f), subdivs);

	if (levels == 0)
	{
		Add_subdiv_triangle_no_seam_fix (verts,
		    indices,
		    cml::lerp (top, bottom_left, 0.5f),
		    cml::lerp (bottom_left, bottom_right, 0.5f),
		    cml::lerp (top, bottom_right, 0.5f),
		    subdivs * 2);
	}
	else
	{
		subdiv_triangle (verts,
		    indices,
		    cml::lerp (top, bottom_left, 0.5f),
		    cml::lerp (bottom_left, bottom_right, 0.5f),
		    cml::lerp (top, bottom_right, 0.5f),
		    levels - 1,
		    subdivs);
	}
}

MeshData CreateWaterPlaneSubdiv (int levels, int subdivs)
{

	std::vector<float> verts;
	std::vector<uint32_t> indices;

	float max_float = 5000000.0f;

	float neg_vert_offset = (max_float / 2.0f) * cml::tan (cml::radians (30.0f));
	float pos_vert_offset = -(max_float / 2.0f) / cml::cos (cml::radians (30.0f));

	cml::vec3f top{ 0, 0, pos_vert_offset };
	cml::vec3f bottom_left{ -(max_float / 2.0f), 0, neg_vert_offset };
	cml::vec3f bottom_right{ (max_float / 2.0f), 0, neg_vert_offset };

	subdiv_triangle (verts, indices, top, bottom_left, bottom_right, levels, subdivs);

	return MeshData (Vert_Pos, verts, indices);
}

Manager::Manager (job::TaskManager& task_manager) : task_manager (task_manager) {}

} // namespace Resource::Mesh