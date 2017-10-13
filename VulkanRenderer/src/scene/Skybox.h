#pragma once

#include <string>
#include <iostream>

#include <vulkan\vulkan.h>

#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "..\core\Texture.h"
#include "..\vulkan\VulkanTexture.hpp"
#include "..\vulkan\VulkanModel.hpp"
#include "..\vulkan\VulkanDevice.hpp"
#include "..\vulkan\VulkanPipeline.hpp"


struct SkyboxUniformBuffer {
	glm::mat4 proj;
	glm::mat4 view;
};

class Skybox {
public:
	Skybox();
	~Skybox();

	VulkanDevice *device;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	VulkanModel model;

	CubeMap skyboxCubeMap;
	VulkanCubeMap vulkanCubeMap;
	
	SkyboxUniformBuffer skyboxUniform;
	VulkanBuffer skyboxUniformBuffer;


	void InitSkybox(VulkanDevice* device, std::string filename, std::string fileExt, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
	void ReinitSkybox(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight); //rebuilds stuff when screen size changes
	
	void CleanUp();

	void UpdateUniform(glm::mat4 proj, glm::mat4 view);

	void LoadSkyboxData(std::string filename, std::string fileExt);

	void SetupUniformBuffer();
	void SetupCubeMapImage();
	void SetupDescriptor();

	void SetupPipeline(VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);

	//Builds a secondary command buffer for the skybox and returns the buffer
	VkCommandBuffer BuildSecondaryCommandBuffer(VkCommandBuffer secondaryCommandBuffer, VkCommandBufferInheritanceInfo inheritanceInfo);

private:
	Texture skyboxImageFront;
	Texture skyboxImageBack;
	Texture skyboxImageLeft;
	Texture skyboxImageRight;
	Texture skyboxImageTop;
	Texture skyboxImageBottom;
};

