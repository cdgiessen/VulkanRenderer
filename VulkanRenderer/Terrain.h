#pragma once

#include <vulkan\vulkan.h>
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"
#include "VulkanBuffer.hpp"

#include "VulkanInitializers.hpp"
#include "VulkanTools.h"

#include "Mesh.h"
#include "Texture.h"
#include <glm\common.hpp>

class Terrain {
public:
	int numCells; //
	glm::vec3 pos; //position of corner
	glm::vec3 size; //width and length
	Mesh* mesh;

	VulkanDevice *device;

	VkPipeline pipeline;
	VkPipeline wireframe;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	Mesh* terrainMesh;
	VulkanModel terrainModel;

	Texture* terrainSplatMap;
	VulkanTexture2D terrainVulkanSplatMap;

	std::vector<Texture> terrainTextureArray;
	VulkanTexture2DArray terrainVulkanTextureArray;

	ModelBufferObject modelUniformObject;
	VulkanBuffer modelUniformBuffer;

	std::vector<VkCommandBuffer> commandBuffers;

	Terrain(int numCells, float posX, float posY, int sizeX, int sizeY);
	~Terrain();

	void InitTerrain(VulkanDevice* device, VkRenderPass renderPass, float viewPortWidth, float viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting);
	void ReinitTerrain(VulkanDevice* device, VkRenderPass renderPass, float viewPortWidth, float viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting);
	void CleanUp();
	void UpdateUniformBuffer(float time);

	void LoadTexture(std::string filename);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();

	void SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting);
	void SetupPipeline(VkRenderPass renderPass, float viewPortWidth, float viewPortHeight);

	void BuildCommandBuffer(VulkanSwapChain* swapChain, VkRenderPass* renderPass);
	void RebuildCommandBuffer(VulkanSwapChain* swapChain, VkRenderPass* renderPass);
};