#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../rendering/Renderer.hpp"
#include "../rendering/Model.hpp"
#include "../rendering/Texture.hpp"

#include "../resources/Mesh.h"
#include "../resources/Texture.h"


class InstancedSceneObject
{
public:

	// Per-instance data block
	struct InstanceData {
		glm::vec3 pos;
		glm::vec3 rot;
		float scale;
		float dummy1;//Alignment fix
		float dummy2;//Alignment fix
	};
	// Contains the instanced data
	//struct InstanceBuffer {
	//	VkBuffer buffer = VK_NULL_HANDLE;
	//	VkDeviceMemory memory = VK_NULL_HANDLE;
	//	size_t size = 0;
	//	VkDescriptorBufferInfo descriptor;
	//} instanceBuffer;



	InstancedSceneObject(std::shared_ptr<VulkanRenderer> renderer, int maxInstances = 256);
	~InstancedSceneObject();

	void InitInstancedSceneObject(std::shared_ptr<VulkanRenderer> renderer);
	void CleanUp();
	void UpdateUniformBuffer();

	void LoadTexture(std::shared_ptr<Texture> tex);
	void LoadModel(std::string fileName);
	void LoadModel(std::shared_ptr<Mesh> mesh);

	void SetFragmentShaderToUse(VkShaderModule shaderModule);
	void SetCullMode(VkCullModeFlagBits cullMode);
	void SetBlendMode(VkBool32 blendEnable);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();
	void SetupPipeline();

	void SetupDescriptor();

	void AddInstance(InstanceData data);

	void AddInstances(std::vector<glm::vec3> positions);
	//void RemoveInstance(std::vector<glm::vec3> positions);

	void UploadInstances();

	void WriteToCommandBuffer(VkCommandBuffer commandBuffer, bool wireframe);
private:

	std::shared_ptr<VulkanRenderer> renderer;

	std::shared_ptr<ManagedVulkanPipeline> mvp;

	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	//VkDescriptorSetLayout descriptorSetLayout;
	//VkDescriptorPool descriptorPool;
	//VkDescriptorSet descriptorSet;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<VulkanModel> vulkanModel;

	std::shared_ptr<Texture> texture;
	VulkanTexture2D vulkanTexture;

	std::vector<InstanceData> instancesData;
	std::vector<glm::vec3> modelPositions;
	std::vector<ModelBufferObject> modelUniforms;

	std::shared_ptr<VulkanBufferUniform> uniformBuffer;
	std::shared_ptr<VulkanBufferVertex> instanceBuffer;

	int instanceCount = 0;
	int maxInstanceCount = 256;

	VkShaderModule fragShaderModule;
	VkCullModeFlagBits cullModeFlagBits = VK_CULL_MODE_BACK_BIT;
	VkBool32 enableBlending = VK_FALSE;
};
