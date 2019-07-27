#pragma once

#include <vector>

#include "cml/cml.h"

#include <vulkan/vulkan.h>

#include "rendering/Model.h"
#include "rendering/Renderer.h"
#include "rendering/Texture.h"

#include "resources/Mesh.h"
#include "resources/Texture.h"

#include "util/DoubleBuffer.h"

class InstancedSceneObject
{
	public:
	int instanceMemberSize = 8;

	// Per-instance data block
	struct InstanceData
	{
		cml::vec3f pos;
		cml::vec3f rot;
		float scale = 1;
		int texIndex = 0;
		// float dummy2;
		InstanceData () = default;
		InstanceData (cml::vec3f pos, cml::vec3f rot, float scale, int texIndex)
		: pos (pos), rot (rot), scale (scale), texIndex (texIndex)
		{
		}

		bool operator== (const InstanceData& rhs)
		{
			return this->pos == rhs.pos && this->rot == rhs.rot && this->scale == rhs.scale &&
			       this->texIndex == rhs.texIndex;
		}
	};

	InstancedSceneObject (VulkanRenderer& renderer, int maxInstances = 16384);
	~InstancedSceneObject ();

	void InitInstancedSceneObject ();
	void UploadData ();

	void LoadTexture (Resource::Texture::TexID);
	void LoadModel (std::string fileName);
	void LoadModel (std::shared_ptr<MeshData> mesh);

	void SetFragmentShaderToUse (std::string frag);
	void SetCullMode (VkCullModeFlagBits cullMode);
	void SetBlendMode (VkBool32 blendEnable);

	void SetupUniformBuffer ();
	void SetupImage ();
	void SetupModel ();
	void SetupPipeline ();

	void SetupDescriptor ();

	void AddInstance (InstanceData data);
	void RemoveInstance (InstanceData data);

	void AddInstances (std::vector<InstanceData>& instances);
	void RemoveInstances (std::vector<InstanceData>& instances);

	void RemoveAllInstances ();

	// Resets all current instances and puts new ones in its place
	void ReplaceAllInstances (std::vector<InstanceData>& instances);

	void UploadInstances ();

	void ImGuiShowInstances ();

	void WriteToCommandBuffer (VkCommandBuffer commandBuffer, bool wireframe);

	private:
	VulkanRenderer& renderer;

	std::unique_ptr<Pipeline> normal;
	std::unique_ptr<Pipeline> wireframe;

	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::shared_ptr<MeshData> mesh;
	std::shared_ptr<VulkanModel> vulkanModel;

	Resource::Texture::TexID texture;
	std::shared_ptr<VulkanTexture> vulkanTexture;

	std::shared_ptr<VulkanBufferUniform> uniformBuffer;

	std::mutex instanceDataLock;
	int instanceCount = 0;
	int maxInstanceCount = 16384;
	std::vector<InstanceData> instancesData;
	std::shared_ptr<VulkanBufferInstancePersistant> instanceBuffer;

	bool isDirty = false;

	std::string fragShaderPath;
	// VkShaderModule fragShaderModule;
	VkCullModeFlagBits cullModeFlagBits = VK_CULL_MODE_BACK_BIT;
	VkBool32 enableBlending = VK_FALSE;

	Signal isFinishedTransfer;
};
