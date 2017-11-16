#pragma once

#include <string>
#include <iostream>

#include <vulkan\vulkan.h>

#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\core\Texture.h"
#include "..\vulkan\VulkanTexture.hpp"
#include "..\vulkan\VulkanModel.hpp"
#include "..\vulkan\VulkanRenderer.hpp"


struct SkyboxUniformBuffer {
	glm::mat4 proj;
	glm::mat4 view;
};

class Skybox {
public:
	Skybox();
	~Skybox();

	std::shared_ptr<VulkanRenderer> renderer;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	VulkanModel model;

	std::shared_ptr<CubeMap> skyboxCubeMap;
	VulkanCubeMap vulkanCubeMap;
	
	SkyboxUniformBuffer skyboxUniform;
	VulkanBuffer skyboxUniformBuffer;


	void InitSkybox(std::shared_ptr<VulkanRenderer> device, std::string filename, std::string fileExt);
	void ReinitSkybox(std::shared_ptr<VulkanRenderer> device); //rebuilds stuff when screen size changes
	
	void CleanUp();

	void UpdateUniform(glm::mat4 proj, glm::mat4 view);

	void LoadSkyboxData(std::string filename, std::string fileExt);

	void SetupUniformBuffer();
	void SetupCubeMapImage();
	void SetupDescriptor();

	void SetupPipeline();

	//Builds a secondary command buffer for the skybox and returns the buffer
	VkCommandBuffer BuildSecondaryCommandBuffer(VkCommandBuffer secondaryCommandBuffer, VkCommandBufferInheritanceInfo inheritanceInfo);

private:

};

