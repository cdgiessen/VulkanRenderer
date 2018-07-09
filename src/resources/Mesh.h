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

		attrib.push_back(initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosNorm, pos)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosNorm, normal)));

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

		attrib.push_back(initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosNormTex, pos)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosNormTex, normal)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex_PosNormTex, texCoord)));

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

		attrib.push_back(initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosNormTexColor, pos)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosNormTexColor, normal)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex_PosNormTexColor, texCoord)));
		attrib.push_back(initializers::vertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex_PosNormTexColor, color)));


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
			return ((hash<glm::vec3>()(vertex.pos)
				^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
				^ (hash<glm::vec2>()(vertex.texCoord) << 1)
				^ (hash<glm::vec4>()(vertex.color) << 1);
		}
	};

	template<> struct hash<Vertex_PosNormTex> {
		size_t operator()(Vertex_PosNormTex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos)
				^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
				^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};

	template<> struct hash<Vertex_PosNorm> {
		size_t operator()(Vertex_PosNorm const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos)
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

	struct Dimension
	{
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size = glm::vec3(1.0f);
	} dimensions;

private:


};

extern std::shared_ptr<Mesh> createSinglePlane();

extern std::shared_ptr<Mesh> createDoublePlane();


extern std::shared_ptr<Mesh> createFlatPlane(int dim, glm::vec3 size);

extern std::shared_ptr<Mesh> createCube();


extern std::shared_ptr<Mesh> createSphere(int dim = 10);