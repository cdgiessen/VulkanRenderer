#pragma once

#include <vulkan\vulkan.h>
#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanDevice.hpp"
#include "VulkanTools.h"
#include "VulkanInitializers.hpp"
#include "VulkanTexture.hpp"
#include "VulkanModel.hpp"
#include "VulkanPipeline.h"
#include "Mesh.h"
#include "Texture.h"

class GameObject
{
public:
	GameObject();
	~GameObject();

	VulkanDevice *device;

	VkPipeline pipeline;
	VkPipeline wireframe;
	VkPipeline debugNormals;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	Mesh* gameObjectMesh;
	VulkanModel gameObjectModel;

	Texture* gameObjectTexture;
	VulkanTexture2D gameObjectVulkanTexture;

	ModelBufferObject modelUniformObject;
	VulkanBuffer modelUniformBuffer;

	void InitGameObject(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting);
	void ReinitGameObject(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
	void CleanUp();
	void UpdateUniformBuffer(float time);

	void LoadModel(std::string fileName);
	void LoadModel(Mesh* mesh);
	void LoadTexture(std::string filename);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();

	void SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting);
	void SetupPipeline(VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
};

