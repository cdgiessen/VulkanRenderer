#pragma once

#include <vector>

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


class InstancedSceneObject
{
public:

	// Per-instance data block
	struct InstanceData {
		glm::vec3 pos;
		glm::vec3 rot;
		float scale;
	};
	// Contains the instanced data
	struct InstanceBuffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		size_t size = 0;
		VkDescriptorBufferInfo descriptor;
	} instanceBuffer;



	InstancedSceneObject();
	~InstancedSceneObject();

	void InitInstancedSceneObject(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting);
	void ReinitInstancedSceneObject(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
	void CleanUp();
	void UpdateUniformBuffer();

	void LoadModel(std::string fileName);
	void LoadModel(Mesh* mesh);
	void LoadTexture(std::string filename);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();

	void SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting);
	void SetupPipeline(VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);

	void InstancedSceneObject::AddInstances(std::vector<glm::vec3> positions);
	//void InstancedSceneObject::RemoveInstance(std::vector<glm::vec3> positions);

	void prepareInstanceData();

	void WriteToCommandBuffer(VkCommandBuffer commandBuffer, bool wireframe);

	VulkanDevice *device;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkPipeline wireframe;
	VkPipeline debugNormals;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	Mesh* mesh;
	VulkanModel vulkanModel;

	Texture* texture;
	VulkanTexture2D vulkanTexture;

	std::vector<glm::vec3> modelPositions;
	std::vector<ModelBufferObject> modelUniforms;
	VulkanBuffer uniformBuffer;
};

