#pragma once

#include <string>
#include <iostream>

#include <vulkan\vulkan.h>

#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Texture.h"
#include "VulkanTexture.hpp"
#include "VulkanModel.hpp"
#include "VulkanDevice.hpp"


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


	void InitSkybox(VulkanDevice* device, std::string filename, std::string fileExt, VkRenderPass renderPass, float viewPortWidth, float viewPortHeight);
	void ReinitSkybox(VulkanDevice* device, VkRenderPass renderPass, float viewPortWidth, float viewPortHeight); //rebuilds stuff when screen size changes
	
	void CleanUp();

	void UpdateUniform(glm::mat4 proj, glm::mat4 view);

	void LoadSkyboxData(std::string filename, std::string fileExt);

	void SetupUniformBuffer();
	void SetupCubeMapImage();
	void SetupDescriptor();

	void SetupPipeline(VkRenderPass renderPass, float viewPortWidth, float viewPortHeight);

private:
	Texture skyboxImageFront;
	Texture skyboxImageBack;
	Texture skyboxImageLeft;
	Texture skyboxImageRight;
	Texture skyboxImageTop;
	Texture skyboxImageBottom;
};

