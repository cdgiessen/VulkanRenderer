#pragma once

#include <vector>
#include <unordered_map>
#include <iostream>
#include <string>


#include <glm\common.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan\vulkan.hpp>

#include "..\vulkan\VulkanInitializers.hpp"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
		std::vector<VkVertexInputBindingDescription> bindingDescription;
		bindingDescription.push_back(initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX));
		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
		attributeDescriptions.resize(4);

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
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
	Vertex() {
		
	}

	Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 texCoord, glm::vec4 color) : pos(pos), normal(normal), texCoord(texCoord), color(color) {

	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord && color == other.color;
	}
};

//Hash function for indicy uniqueness in model loading.
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return (  (hash<glm::vec3>()(vertex.pos) 
					^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) 
					^ (hash<glm::vec2>()(vertex.texCoord) << 1)
					^ (hash<glm::vec4>()(vertex.color) << 1);
		}
	};
}



class Mesh
{
public:
	Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices);
	Mesh();
	~Mesh();
	
	void importFromFile(const std::string filename);

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;

	/** @brief Stores vertex and index base and counts for each part of a model */
	struct ModelPart {
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};

	std::vector<ModelPart> parts;

	struct Dimension
	{
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size;
	} dimensions;

private:
};

static std::shared_ptr<Mesh> createSinglePlane() {
	return std::make_shared<Mesh>(std::initializer_list<Vertex>{
		//vertices

		{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }

		//indices
	}, std::initializer_list<uint16_t>{
		0, 1, 2, 2, 3, 0	
	});
}

static std::shared_ptr<Mesh> createDoublePlane() {
	return std::make_shared<Mesh>(std::initializer_list<Vertex>{
		//vertices
	
		{ { -0.5f, -0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },

		{ { -0.5f, -0.5f, -1.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, -1.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, -1.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -1.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
	
		//indices
	}, std::initializer_list<uint16_t>{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	} );
}


static std::shared_ptr<Mesh> createFlatPlane(int numCells, glm::vec3 size) {
	std::vector<Vertex> verts;
	std::vector<uint16_t> indices;

	verts.resize((numCells + 1) * (numCells + 1));
	indices.resize((numCells) * (numCells)* 6);

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			verts[(i)*(numCells + 1) + j] = Vertex(glm::vec3((double)i *(size.x) / (float)numCells, 0, (double)j *(size.z) / (float)numCells), glm::vec3(0,1,0), glm::vec2(i, j), glm::vec4(1));
		}
	}

	int counter = 0;
	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			indices[counter++] = i * (numCells + 1) + j;
			indices[counter++] = i * (numCells + 1) + j + 1;
			indices[counter++] = (i + 1) * (numCells + 1) + j;
			indices[counter++] = i * (numCells + 1) + j + 1;
			indices[counter++] = (i + 1) * (numCells + 1) + j + 1;
			indices[counter++] = (i + 1) * (numCells + 1) + j;
		}
	}

	return std::make_shared<Mesh>(verts, indices);
}

static std::shared_ptr<Mesh> createCube() {
	return std::make_shared<Mesh>(std::initializer_list<Vertex>{

		//Left face
		{{ 0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 1.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 1.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, 0.0f, -1.0f},{ 0.667f, 1.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		
		//Right face			
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 0.0f, 1.0f},{ 0.667f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, 0.0f, 1.0f},{ 0.333f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		
		//Back face			
		{{ -0.5f, -0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 1.0f}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 0.5f}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.333f, 0.5f}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 0.5f  }, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{-1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f  }, {1.0f,1.0f,1.0f, 1.0f}},
																					  
		 //Front face																  
		{{ 0.5f, 0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, -0.5f, 0.5f	},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f	},{1.0f, 0.0f, 0.0f},{ 0.334f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{1.0f, 0.0f, 0.0f},{ 0.0f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		
		 //Bottom face		
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 1.0f		}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 1.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, 0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 0.5f		}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, -0.5f, -0.5f	},{0.0f, -1.0f, 0.0f},{ 1.0f, 1.0f		}, {1.0f,1.0f,1.0f, 1.0f}},
		
		 //Top face			
		{{ 0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, 0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ 0.5f, 0.5f, 0.5f		},{0.0f, 1.0f, 0.0f},{ 1.0f, 0.0f	}, {1.0f,1.0f,1.0f, 1.0f}},
		{{ -0.5f, 0.5f, -0.5f	},{0.0f, 1.0f, 0.0f},{ 0.667f, 0.5f	}, {1.0f,1.0f,1.0f, 1.0f}}
	

	//indices
	}, std::initializer_list<uint16_t>{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
	});
			
}

