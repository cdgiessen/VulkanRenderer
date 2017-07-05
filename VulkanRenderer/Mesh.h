#pragma once

#include <vector>

#include <glm\common.hpp>
#include <vulkan\vulkan.hpp>

#include "VulkanBuffer.hpp"

#include "VulkanInitializers.hpp"

#include <noise/noise.h>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		return initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
	Vertex() {
		
	}

	Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 texCoord) : pos(pos), color(color), texCoord(texCoord) {

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
		{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } }

		
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
		{ { -0.5f, -0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } },

		{ { -0.5f, -0.5f, -1.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, -1.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
		{ { 0.5f, 0.5f, -1.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -1.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } }
	},
		//indices
	{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	});
}

static Mesh* createFlatPlane() {
	std::vector<Vertex> verts;
	std::vector<uint16_t> indices;
	
	int numCells = 10;

	noise::module::Perlin myModule = noise::module::Perlin();

	verts.resize((numCells + 1) * (numCells + 1));
	indices.resize(numCells * numCells * 6);

	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			verts[i*numCells + j] = Vertex(glm::vec3(i, myModule.GetValue(i,0,j), j), glm::vec3(0, 1, 0), glm::vec2(i, j));
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
		{{ 0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 1.0f	}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 1.0f	}},
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 0.5f	}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 0.5f	}},
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 0.5f	}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 1.0f	}},
		
		//Right face			
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.0f	}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.5f	}},
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.5f	}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.0f	}},
		{{ -0.5f, 0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.0f	}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.5f	}},
		
		//Back face			
		{{ -0.5f, -0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f	}},
		{{ -0.5f, -0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 0.5f}},
		{{ -0.5f, 0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 0.5f}},
		{{ -0.5f, 0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 0.5f  }},
		{{ -0.5f, -0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f  }},
		
		 //Front face			
		{{ 0.5f, 0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.0f	}},
		{{ 0.5f, 0.5f, 0.5f		},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f	}},
		{{ 0.5f, -0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.5f	}},
		{{ 0.5f, -0.5f, 0.5f	},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.5f	}},
		{{ 0.5f, -0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.5f	}},
		{{ 0.5f, 0.5f, 0.5f		},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f	}},
		
		 //Bottom face		
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 0.5f	}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 1.0f		}},
		{{ 0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 1.0f	}},
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 0.5f	}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 0.5f		}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 1.0f		}},
		
		 //Top face			
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.5f	}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.5f	}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.0f	}},
		{{ -0.5f, 0.5f, 0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.0f	}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.0f	}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.5f	}}
	},

	//indices
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
	});
			
}

