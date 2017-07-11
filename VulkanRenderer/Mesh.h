#pragma once

#include <vector>
#include <iostream>

#include <glm\common.hpp>
#include <vulkan\vulkan.hpp>

#include "VulkanBuffer.hpp"

#include "VulkanInitializers.hpp"

#include <noise/noise.h>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		return initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
	Vertex() {
		
	}

	Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 texCoord, glm::vec3 color) : pos(pos), normal(normal), texCoord(texCoord), color(color) {

	}
};

class Mesh
{
public:
	Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices);
	~Mesh();

	void createMeshBuffers(VkDevice device, VkPhysicalDevice physicalDevice);
	void cleanup(VkDevice device);

	void createVertexBuffer();
	void createIndexBuffer();

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkDevice device;
	
private:
};

static Mesh* createSinglePlane() {
	return new Mesh(
		//vertices
	{
		{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } }

		
	},
		//indices
	{
		0, 1, 2, 2, 3, 0
		
	});
}

static Mesh* createDoublePlane() {
	return new Mesh(
		//vertices
	{
		{ { -0.5f, -0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { -0.5f, 0.5f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },

		{ { -0.5f, -0.5f, -1.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, -1.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, -1.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
		{ { -0.5f, 0.5f, -1.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } }
	},
		//indices
	{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	});
}

static glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t) {
	

	
	return (a) + glm::vec3((b - (a)).x * t, (b - (a)).y * t, (b - (a)).z * t);
}

static glm::vec3 color0 = { 0, 0.1f, 0.8f };
static glm::vec3 color1 = { 0, 0.65f, 0.33f };
static glm::vec3 color2 = { 0.47f, 0.28f, 0.0f };
static glm::vec3 color3 = { 0.5f, 0.5f, 0.5f };
static glm::vec3 color4 = { 1.0f, 1.0f, 1.0f };

static glm::vec3 CalcColorAtElevation(float elevation) {
	if (elevation < 0.01f)
		return lerp(color0, color1, elevation);
	else if (elevation < 0.35)
		return lerp(color1, color2, elevation);
	else if (elevation < 0.6)
		return lerp(color2, color3, elevation);
	else if (elevation < 0.8)
		return lerp(color3, color4, elevation);
	return glm::vec3(1.0f);
}

static Mesh* createFlatPlane() {
	std::vector<Vertex> verts;
	std::vector<uint16_t> indices;
	
	int numCells = 100;

	noise::module::Perlin myModule;
	myModule.SetOctaveCount(6);
	myModule.SetFrequency(0.05);
	myModule.SetPersistence(0.5);

	verts.resize((numCells) * (numCells));
	indices.resize((numCells - 1) * (numCells -1)* 6);

	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			float hL = myModule.GetValue((double)i + 1, 0.25, (double)j + 0.25) + 0.5f;
			float hR = myModule.GetValue((double)i - 1, 0.25, (double)j + 0.25) + 0.5f;
			float hD = myModule.GetValue((double)i, 0.25, (double)j + 1 + 0.25) + 0.5f;
			float hU = myModule.GetValue((double)i, 0.25, (double)j - 1 + 0.25) + 0.5f;
			glm::vec3 normal(hR - hL, 1, hU - hD);

			double value = (myModule.GetValue((double)i , 0.25, (double)j + 0.25) + 1.0f)/2.0f;

			verts[i*numCells + j] = Vertex(glm::vec3(i, value * 10, j), normal, glm::vec2(i, j), CalcColorAtElevation(value));
			//std::cout << value << std::endl;
		}
	}

	int counter = 0;
	for (int i = 0; i < numCells - 1; i++)
	{
		for (int j = 0; j < numCells - 1; j++)
		{
			indices[counter++] = i * numCells + j;
			indices[counter++] = i * numCells + j + 1;
			indices[counter++] = (i + 1) * numCells + j;
			indices[counter++] = i * numCells + j + 1;
			indices[counter++] = (i + 1) * numCells + j + 1;
			indices[counter++] = (i + 1) * numCells + j;
		}
	}

	return new Mesh(verts, indices);
}

static Mesh* createCube() {
	return new Mesh(
	{
		//Left face
		{{ 0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 1.0f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 1.0f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 1.0f	}, {1.0f,1.0f,1.0f}},
		
		//Right face			
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f}},
		
		//Back face			
		{{ -0.5f, -0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 1.0f}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 0.5f}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 0.5f}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 0.5f  }, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f  }, {1.0f,1.0f,1.0f}},
		
		 //Front face			
		{{ 0.5f, 0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, -0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, -0.5f, 0.5f	},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, -0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f	}, {1.0f,1.0f,1.0f}},
		
		 //Bottom face		
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 1.0f		}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 1.0f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 0.5f		}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 1.0f		}, {1.0f,1.0f,1.0f}},
		
		 //Top face			
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.0f	}, {1.0f,1.0f,1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f}}
	},

	//indices
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
	});
			
}

