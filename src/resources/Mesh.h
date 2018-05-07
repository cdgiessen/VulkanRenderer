#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <variant>

#include <glm/common.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

#include "../rendering/Initializers.h"

struct Vertex_PosNorm {
	glm::vec3 pos;
	glm::vec3 normal;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
		std::vector<VkVertexInputBindingDescription> bindingDescription;
		bindingDescription.push_back(initializers::vertexInputBindingDescription(0, sizeof(Vertex_PosNorm), VK_VERTEX_INPUT_RATE_VERTEX));
		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attrib = {};
		
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex_PosNorm, pos)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex_PosNorm, normal)));

		return attrib;
	}
	Vertex_PosNorm() {
		
	}

	Vertex_PosNorm(glm::vec3 pos, glm::vec3 normal) : pos(pos), normal(normal) {

	}

	bool operator==(const Vertex_PosNorm& other) const {
		return pos == other.pos && normal == other.normal;
	}
};

struct Vertex_PosNormTex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
		std::vector<VkVertexInputBindingDescription> bindingDescription;
		bindingDescription.push_back(initializers::vertexInputBindingDescription(0, sizeof(Vertex_PosNormTex), VK_VERTEX_INPUT_RATE_VERTEX));
		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attrib = {};
		
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex_PosNormTex, pos)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex_PosNormTex, normal)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex_PosNormTex, texCoord)));		

		return attrib;
	}
	Vertex_PosNormTex() {
		
	}

	Vertex_PosNormTex(glm::vec3 pos, glm::vec3 normal, glm::vec2 texCoord) 
		: pos(pos), normal(normal), texCoord(texCoord) {

	}

	bool operator==(const Vertex_PosNormTex& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}
};

struct Vertex_PosNormTexColor {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
		std::vector<VkVertexInputBindingDescription> bindingDescription;
		bindingDescription.push_back(initializers::vertexInputBindingDescription(0, sizeof(Vertex_PosNormTexColor), VK_VERTEX_INPUT_RATE_VERTEX));
		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attrib = {};
		
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex_PosNormTexColor, pos)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex_PosNormTexColor, normal)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT,offsetof(Vertex_PosNormTexColor, texCoord)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(Vertex_PosNormTexColor, color)));
		

		return attrib;
	}
	Vertex_PosNormTexColor() {
		
	}

	Vertex_PosNormTexColor(glm::vec3 pos, glm::vec3 normal, glm::vec2 texCoord, glm::vec4 color) 
		: pos(pos), normal(normal), texCoord(texCoord), color(color) {

	}

	bool operator==(const Vertex_PosNormTexColor& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord && color == other.color;
	}
};

//Hash function for indicy uniqueness in model loading.
namespace std {
	template<> struct hash<Vertex_PosNormTexColor> {
		size_t operator()(Vertex_PosNormTexColor const& vertex) const {
			return (  (hash<glm::vec3>()(vertex.pos) 
					^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) 
					^ (hash<glm::vec2>()(vertex.texCoord) << 1)
					^ (hash<glm::vec4>()(vertex.color) << 1);
		}
	};

	template<> struct hash<Vertex_PosNormTex> {
		size_t operator()(Vertex_PosNormTex const& vertex) const {
			return (  (hash<glm::vec3>()(vertex.pos) 
					^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) 
					^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};

	template<> struct hash<Vertex_PosNorm> {
		size_t operator()(Vertex_PosNorm const& vertex) const {
			return (  (hash<glm::vec3>()(vertex.pos) 
					^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1);
		}
	};
}

using Vertices_PosNorm = std::vector<Vertex_PosNorm>;
using Vertices_PosNormTex = std::vector<Vertex_PosNormTex>;
using Vertices_PosNormTexColor = std::vector<Vertex_PosNormTexColor>;

class Mesh
{
public:
	Mesh(Vertices_PosNorm vertices, std::vector<uint16_t> indices);
	Mesh(Vertices_PosNormTex vertices, std::vector<uint16_t> indices);
	Mesh(Vertices_PosNormTexColor vertices, std::vector<uint16_t> indices);

	Mesh();
	~Mesh();
	
	void importFromFile(const std::string filename);

	std::variant<std::vector<Vertex_PosNorm>, std::vector<Vertex_PosNormTex>, std::vector<Vertex_PosNormTexColor>> vertices;
	std::vector<uint16_t> indices;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	uint32_t vertexElementCount = 0;

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
	return std::make_shared<Mesh>(Vertices_PosNormTexColor({
		//vertices

		{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }

		//indices
	}), std::vector<uint16_t>({
		0, 1, 2, 2, 3, 0	
	}));
};

static std::shared_ptr<Mesh> createDoublePlane() {
	return std::make_shared<Mesh>(Vertices_PosNormTexColor({
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
	}), std::vector<uint16_t>({
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	}) );
};


static std::shared_ptr<Mesh> createFlatPlane(int dim, glm::vec3 size) {
	Vertices_PosNormTexColor verts;
	std::vector<uint16_t> indices;

	verts.resize((dim + 1) * (dim + 1));
	indices.resize((dim) * (dim)* 6);

	for (int i = 0; i <= dim; i++)
	{
		for (int j = 0; j <= dim; j++)
		{
			verts[(i)*(dim + 1) + j] = Vertex_PosNormTexColor(
				glm::vec3((double)i *(size.x) / (float)dim, 0, 
							(double)j *(size.z) / (float)dim), 
				glm::vec3(0,1,0), glm::vec2(i, j), glm::vec4(1));
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

	return std::make_shared<Mesh>(verts, indices);
}

static std::shared_ptr<Mesh> createCube() {
	return std::make_shared<Mesh>(Vertices_PosNormTexColor({

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
	}), std::vector<uint16_t>({
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
	}));
			
}


extern std::shared_ptr<Mesh> createSphere(int dim = 10);