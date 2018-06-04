#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../rendering/Renderer.h"
#include "../rendering/Model.h"
#include "../rendering/Texture.h"

#include "../resources/Mesh.h"
#include "../resources/Texture.h"


class InstancedSceneObject
{
public:

	int instanceMemberSize = 8;

	// Per-instance data block
	struct InstanceData {
		glm::vec3 pos;
		glm::vec3 rot;
		float scale = 1;
		int texIndex = 0;
		//float dummy2;
		InstanceData() = default;
		InstanceData(glm::vec3 pos, glm::vec3 rot, float scale, int texIndex) :
			pos(pos), rot(rot), scale(scale), texIndex(texIndex)
		{}

		bool operator==(const InstanceData& rhs) {
			return this->pos == rhs.pos
				&& this->rot == rhs.rot
				&& this->scale == rhs.scale
				&& this->texIndex == rhs.texIndex;
		}
	};

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
	void RemoveInstance(InstanceData data);

	void AddInstances(std::vector<InstanceData>& instances);
	void RemoveInstances(std::vector<InstanceData>& instances);

	void RemoveAllInstances();

	//Resets all current instances and puts new ones in its place
	void ReplaceAllInstances(std::vector<InstanceData>& instances);

	void UploadInstances();

	void ImGuiShowInstances();

	void WriteToCommandBuffer(VkCommandBuffer commandBuffer, bool wireframe);
private:

	std::shared_ptr<VulkanRenderer> renderer;

	std::shared_ptr<ManagedVulkanPipeline> mvp;

	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<VulkanModel> vulkanModel;

	std::shared_ptr<Texture> texture;
	std::shared_ptr<VulkanTexture2D> vulkanTexture;

	std::shared_ptr<VulkanBufferUniform> uniformBuffer;

	int instanceCount = 0;
	int maxInstanceCount = 16384;
	std::vector<InstanceData> instancesData;
	std::shared_ptr<VulkanBufferInstance> instanceBuffer;


	VkShaderModule fragShaderModule;
	VkCullModeFlagBits cullModeFlagBits = VK_CULL_MODE_BACK_BIT;
	VkBool32 enableBlending = VK_FALSE;

	Signal isFinishedTransfer;
};
