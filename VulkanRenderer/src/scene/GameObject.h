#pragma once

#include <vulkan\vulkan.h>
#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\vulkan\vulkanDevice.hpp"
#include "..\vulkan\VulkanModel.hpp"
#include "..\vulkan\VulkanPipeline.hpp"
#include "..\vulkan\VulkanTexture.hpp"
#include "..\vulkan\VulkanInitializers.hpp"
#include "..\core\Mesh.h"
#include "..\core\Texture.h"

class GameObject
{
public:
	GameObject();
	~GameObject();

	std::shared_ptr<VulkanDevice> device;

	VkPipeline pipeline;
	VkPipeline wireframe;
	VkPipeline debugNormals;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	std::shared_ptr<Mesh> gameObjectMesh;
	VulkanModel gameObjectModel;

	std::shared_ptr<Texture> gameObjectTexture;
	VulkanTexture2D gameObjectVulkanTexture;

	ModelBufferObject modelUniformObject;
	VulkanBuffer modelUniformBuffer;

	void InitGameObject(std::shared_ptr<VulkanDevice> device, std::shared_ptr<VulkanPipeline> pipelineManager, VkRenderPass renderPass,
		uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting);
	void ReinitGameObject(std::shared_ptr<VulkanDevice> device, std::shared_ptr<VulkanPipeline> pipelineManager, VkRenderPass renderPass,
		uint32_t viewPortWidth, uint32_t viewPortHeight);
	void CleanUp();
	void UpdateUniformBuffer(float time);

	void LoadModel(std::string fileName);
	void LoadModel(std::shared_ptr<Mesh> mesh);
	void LoadTexture(std::string filename);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();

	void SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting);
	void SetupPipeline(std::shared_ptr<VulkanPipeline> pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
};

