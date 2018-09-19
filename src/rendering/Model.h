#pragma once

#include <vulkan/vulkan.h>

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

#include "Buffer.h"
#include "Device.h"
#include "RenderStructs.h"
#include "RenderTools.h"

#include "../core/CoreTools.h"
#include "../resources/Mesh.h"

class VulkanRenderer;

class VertexLayout
{
	public:
	VertexLayout (VertexDescription desc)
	: bindingDesc (getBindingDescription (desc)), attribDesc (getAttributeDescriptions (desc))
	{
	}

	std::vector<VkVertexInputBindingDescription> bindingDesc;
	std::vector<VkVertexInputAttributeDescription> attribDesc;

	private:
	std::vector<VkVertexInputBindingDescription> getBindingDescription (VertexDescription vertDesc);
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions (VertexDescription vertDesc);
};

class VulkanModel
{
	public:
	VulkanModel (VulkanRenderer& renderer, std::shared_ptr<Mesh> mesh);
	VulkanModel (VulkanRenderer& renderer, std::shared_ptr<MeshData> meshData);

	// VulkanBuffer vertices;
	// VulkanBuffer indices;
	uint32_t vertexCount = 0;
	uint32_t vertexElementCount = 0;
	uint32_t indexCount = 0;

	std::unique_ptr<VulkanBufferVertex> vmaVertices;
	std::unique_ptr<VulkanBufferIndex> vmaIndicies;

	Signal readyToUse;

	struct ModelPart
	{
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};
	std::vector<ModelPart> parts;

	// bool loadFromMesh(std::shared_ptr<Mesh> mesh, VulkanRenderer& renderer);

	void BindModel (VkCommandBuffer cmdBuf);

	private:
	VulkanRenderer& renderer;
	VertexLayout vertLayout;
};