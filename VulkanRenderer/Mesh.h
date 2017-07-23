#pragma once

#include <vector>
#include <iostream>
#include <string>

#include <glm\common.hpp>
#include <vulkan\vulkan.hpp>

#include "VulkanInitializers.hpp"

#include <noise/noise.h>
#include <noiseutils.h>


struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;

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
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
	Vertex() {
		
	}

	Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 texCoord, glm::vec4 color) : pos(pos), normal(normal), texCoord(texCoord), color(color) {

	}
};

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

static Mesh* createSinglePlane() {
	return new Mesh(
		//vertices
	{
		{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }

		
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
		{ { -0.5f, -0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },

		{ { -0.5f, -0.5f, -1.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, -1.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, -1.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -1.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
	},
		//indices
	{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	});
}

static glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t) {
	return a + (b - a)*t;
}

static Mesh* generateTerrainMesh(int numCells, float xLoc, float yLoc, float xSize, float ySize) {
	std::vector<Vertex> verts;
	std::vector<uint16_t> indices;

	noise::module::Perlin myModule;
	myModule.SetOctaveCount(6);
	myModule.SetFrequency(0.05);
	myModule.SetPersistence(0.5);

	noise::utils::NoiseMap heightMap;
	utils::NoiseMapBuilderPlane heightMapBuilder;
	heightMapBuilder.SetSourceModule(myModule);
	heightMapBuilder.SetDestNoiseMap(heightMap);
	heightMapBuilder.SetDestSize(numCells + 1, numCells + 1);
	heightMapBuilder.SetBounds(xLoc, xLoc + xSize + (double)xSize/numCells, yLoc, yLoc + ySize + (double)ySize/numCells);
	heightMapBuilder.Build();

	utils::RendererImage renderer;
	utils::Image image;
	renderer.ClearGradient();
	//old gradient for vertex colors, new one for tex splat map
	////renderer.AddGradientPoint(-1.0000, utils::Color(0, 0, 128, 255)); // deeps
	////renderer.AddGradientPoint(0.000, utils::Color(0, 0, 255, 255)); // shallow
	////renderer.AddGradientPoint(0.1000, utils::Color(0, 128, 255, 255)); // shore
	//renderer.AddGradientPoint(-1.0000, utils::Color(76, 36, 16, 255)); // Ocean bottom
	//renderer.AddGradientPoint(-0.1000, utils::Color(107, 71, 53, 255)); // shallow ocean
	//renderer.AddGradientPoint(0.1000, utils::Color(228, 163, 55, 255)); // shore
	//renderer.AddGradientPoint(0.1625, utils::Color(240, 240, 64, 255)); // sand
	//renderer.AddGradientPoint(0.2250, utils::Color(32, 160, 0, 255)); // grass
	//renderer.AddGradientPoint(0.4750, utils::Color(127, 91, 48, 255)); // dirt
	//renderer.AddGradientPoint(0.7500, utils::Color(128, 128, 128, 255)); // rock
	//renderer.AddGradientPoint(1.0000, utils::Color(255, 255, 255, 255)); // snow

	/*
	"DeadSpruceTreeTrunk.png",
	"Sand.png",
	"SpruceTreeLeaf.png",
	"Rock.png",
	"Snow.png",
	*/

	renderer.AddGradientPoint(-1.000, utils::Color(255, 0, 0, 32)); 
	renderer.AddGradientPoint(0.0000, utils::Color(255, 0, 0, 255));
	renderer.AddGradientPoint(0.1000, utils::Color(0, 255, 0, 255));
	renderer.AddGradientPoint(0.4000, utils::Color(0, 255, 0, 255));
	renderer.AddGradientPoint(0.5000, utils::Color(0, 0, 255, 255));
	renderer.AddGradientPoint(0.8000, utils::Color(0, 0, 255, 255));
	renderer.AddGradientPoint(1.0000, utils::Color(0, 0, 0, 255));

	renderer.SetSourceNoiseMap(heightMap);
	renderer.SetDestImage(image);
	renderer.Render();


	verts.resize((numCells + 1) * (numCells + 1));
	indices.resize((numCells) * (numCells) * 6);

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			double hL = myModule.GetValue((double)(i + 1) *(xSize) / numCells + (xLoc), 0, (double)j *(ySize) / numCells + yLoc);
			double hR = myModule.GetValue((double)(i - 1) *(xSize) / numCells + (xLoc), 0, (double)j *(ySize) / numCells + yLoc);
			double hD = myModule.GetValue((double)i *(xSize) / numCells + (xLoc), 0, (double)(j + 1)*(ySize) / numCells + yLoc);
			double hU = myModule.GetValue((double)i *(xSize) / numCells + (xLoc), 0, (double)(j - 1)*(ySize) / numCells + yLoc);
			glm::vec3 normal(hR - hL, 1, hU - hD);

			double value = (myModule.GetValue((double)i *(xSize) / numCells + (xLoc), 0.0, (double)j *(ySize) / numCells + (yLoc)));

			verts[(i)*(numCells + 1) + j] = Vertex(glm::vec3((double)i *(xSize) / numCells, value * 5, (double)j * (ySize) / numCells), normal,
				glm::vec2(i, j),
				glm::vec4((float)image.GetValue(i, j).red / 256, (float)image.GetValue(i, j).green / 256, (float)image.GetValue(i, j).blue / 256, (float)image.GetValue(i,j).alpha/256));
			//std::cout << value << std::endl;
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

	return new Mesh(verts, indices);
}

static Mesh* createFlatPlane(int numCells) {
	std::vector<Vertex> verts;
	std::vector<uint16_t> indices;

	verts.resize((numCells + 1) * (numCells + 1));
	indices.resize((numCells) * (numCells)* 6);

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			verts[(i)*(numCells + 1) + j] = Vertex(glm::vec3(i,0,j), glm::vec3(0,1,0), glm::vec2(i, j), glm::vec4(1));
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

	return new Mesh(verts, indices);
}

static Mesh* createCube() {
	return new Mesh(
	{
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
	},

	//indices
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
	});
			
}

