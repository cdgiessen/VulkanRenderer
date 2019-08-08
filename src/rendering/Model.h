#pragma once

#include <vulkan/vulkan.h>

//#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include "Buffer.h"

#include "core/CoreTools.h"
#include "resources/Mesh.h"

class VulkanRenderer;
using Signal = std::shared_ptr<bool>;

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
	VulkanModel (VulkanRenderer& renderer, std::unique_ptr<MeshData> meshData);

	uint32_t vertexCount = 0;
	uint32_t vertexElementCount = 0;
	uint32_t indexCount = 0;

	std::unique_ptr<VulkanBuffer> vmaVertices;
	std::unique_ptr<VulkanBuffer> vmaIndicies;

	Signal readyToUse;

	struct ModelPart
	{
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};
	std::vector<ModelPart> parts;

	void BindModel (VkCommandBuffer cmdBuf);

	VertexLayout GetVertexLayout ();

	private:
	VulkanRenderer& renderer;
	VertexLayout vertLayout;
};