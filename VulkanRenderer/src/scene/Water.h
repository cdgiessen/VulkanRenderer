#pragma once


#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\vulkan\VulkanRenderer.hpp"
#include "..\vulkan\VulkanModel.hpp"
#include "..\vulkan\VulkanTexture.hpp"

#include "..\core\Mesh.h"
#include "..\core\Texture.h"
#include <glm\common.hpp>

class Water {
public:
	int numCells; //
	glm::vec3 pos; //position of corner
	glm::vec3 size; //width and length
	std::shared_ptr<Mesh> mesh;

	std::shared_ptr<VulkanRenderer> renderer;

	VkPipeline pipeline;
	VkPipeline seascapePipeline;
	VkPipeline wireframe;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	std::shared_ptr<Mesh> WaterMesh;
	VulkanModel WaterModel;

	std::shared_ptr<Texture> WaterTexture;
	VulkanTexture2D WaterVulkanTexture;

	ModelBufferObject modelUniformObject;
	VulkanBuffer modelUniformBuffer;

	std::vector<VkCommandBuffer> commandBuffers;

	Water(int numCells, float posX, float posY, float sizeX, float sizeY);
	~Water();

	void InitWater(std::shared_ptr<VulkanRenderer> renderer, VulkanBuffer &global, VulkanBuffer &lighting);
	void ReinitWater(std::shared_ptr<VulkanRenderer> renderer);
	void CleanUp();
	void UpdateUniformBuffer(float time, glm::mat4 view);

	void LoadTexture(std::string filename);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();

	void SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting);
	void SetupPipeline();

	void BuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass);
	void RebuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass);
};