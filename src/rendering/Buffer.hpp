#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Device.hpp"
#include "Descriptor.hpp"




/**
* @brief Encapsulates access to a Vulkan buffer backed up by device memory
* @note To be filled by an external source like the VulkanDevice
*/

class VulkanBuffer {
public:
	VulkanBuffer();
	VulkanBuffer(VkDescriptorType type);

	void CleanBuffer(VulkanDevice& device);

	void Map(VulkanDevice& device, void** pData);
	void Unmap(VulkanDevice& device);

	void CopyToBuffer(VulkanDevice& device, void* pData, VkDeviceSize size);

	VmaBuffer buffer;
	DescriptorResource resource;

protected:
	void SetupResource();

	VkDeviceSize m_size;
};

class VulkanBufferUniform : public VulkanBuffer {
public:
	VulkanBufferUniform();
	void CreateUniformBuffer(VulkanDevice& device, VkDeviceSize size);
	void CreateStagingUniformBuffer(VulkanDevice& device, void* pData, VkDeviceSize size);


};
class VulkanBufferUniformDynamic : public VulkanBuffer {
public:
	VulkanBufferUniformDynamic();
	void CreateDynamicUniformBuffer(VulkanDevice& device, uint32_t count, VkDeviceSize size);

};

class VulkanBufferVertex : public VulkanBuffer {
public:
	VulkanBufferVertex();
	void CreateVertexBuffer(VulkanDevice& device, uint32_t count);
	void CreateStagingVertexBuffer(VulkanDevice& device, void* pData, uint32_t count);
	
	void BindVertexBuffer(VkCommandBuffer cmdBuf);
};

class VulkanBufferIndex : public VulkanBuffer {
public:
	VulkanBufferIndex();
	void CreateIndexBuffer(VulkanDevice& device, uint32_t count);
	void CreateStagingIndexBuffer(VulkanDevice& device, void* pData, uint32_t count);

	void BindIndexBuffer(VkCommandBuffer cmdBuf);

};
