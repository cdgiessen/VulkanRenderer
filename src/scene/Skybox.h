#pragma once

#include <string>
#include <iostream>

#include <vulkan\vulkan.h>

#include <glm\common.hpp>
#include <glm\gtc\matrix_transform.hpp>


  
  

#include "..\resources\Texture.h"

#include "..\rendering\VulkanTexture.hpp"
#include "..\rendering\VulkanModel.hpp"
#include "..\rendering\VulkanRenderer.hpp"


struct SkyboxUniformBuffer {
	glm::mat4 proj;
	glm::mat4 view;
};

class Skybox {
public:
	Skybox();
	~Skybox();

	std::shared_ptr<VulkanRenderer> renderer;

	std::shared_ptr<ManagedVulkanPipeline> mvp;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	VulkanModel model;

	std::shared_ptr<CubeMap> skyboxCubeMap;
	VulkanCubeMap vulkanCubeMap;
	
	SkyboxUniformBuffer skyboxUniform;
	VulkanBuffer skyboxUniformBuffer;


	void InitSkybox(std::shared_ptr<VulkanRenderer> renderer);
	
	void CleanUp();

	void UpdateUniform(glm::mat4 proj, glm::mat4 view);

	void SetupUniformBuffer();
	void SetupCubeMapImage();
	void SetupDescriptor();
	void SetupPipeline();

	//Builds a secondary command buffer for the skybox and returns the buffer
	VkCommandBuffer BuildSecondaryCommandBuffer(VkCommandBuffer secondaryCommandBuffer, VkCommandBufferInheritanceInfo inheritanceInfo);

private:

};
