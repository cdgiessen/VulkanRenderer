#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Descriptor.h"
#include "RenderTools.h"

class VulkanDevice;

class VulkanBuffer {
public:
	VulkanBuffer(VulkanDevice& device);
	VulkanBuffer(VulkanDevice& device, VkDescriptorType type);

	void CleanBuffer();

	void Map(void** pData);
	void Unmap();

	void CopyToBuffer(void* pData, VkDeviceSize size);

	VkDeviceSize Size() const;

	VmaBuffer buffer;
	DescriptorResource resource;

protected:
	VulkanDevice & device;

	void SetupResource();

	VkDeviceSize m_size;
	bool persistantlyMapped = false;
	void* mapped;
};

class VulkanBufferUniform : public VulkanBuffer {
public:
	VulkanBufferUniform(VulkanDevice& device);

	void CreateUniformBuffer(VkDeviceSize size);
	void CreateUniformBufferPersitantlyMapped(VkDeviceSize size);
	void CreateStagingUniformBuffer(void* pData, VkDeviceSize size);
};

class VulkanBufferUniformDynamic : public VulkanBuffer {
public:
	VulkanBufferUniformDynamic(VulkanDevice& device);

	void CreateDynamicUniformBuffer(uint32_t count, VkDeviceSize size);

};

class VulkanBufferStagingResource : public VulkanBuffer {
public:
	VulkanBufferStagingResource(VulkanDevice& device, void* pData, VkDeviceSize size);

};

class VulkanBufferVertex : public VulkanBuffer {
public:
	VulkanBufferVertex(VulkanDevice& device);

	void CreateVertexBuffer(uint32_t count, uint32_t vertexElementCount);
	void CreateStagingVertexBuffer(void* pData, uint32_t count, uint32_t vertexElementCount);

	void BindVertexBuffer(VkCommandBuffer cmdBuf);
};

class VulkanBufferIndex : public VulkanBuffer {
public:
	VulkanBufferIndex(VulkanDevice& device);
	void CreateIndexBuffer(uint32_t count);
	void CreateStagingIndexBuffer(void* pData, uint32_t count);

	void BindIndexBuffer(VkCommandBuffer cmdBuf);

};

class VulkanBufferInstance : public VulkanBuffer {
public:
	VulkanBufferInstance(VulkanDevice& device);

	void CreateInstanceBuffer(uint32_t count, uint32_t indexElementCount);
	void CreateStagingInstanceBuffer(void* pData, uint32_t count, uint32_t indexElementCount);

	void BindInstanceBuffer(VkCommandBuffer cmdBuf);


};
