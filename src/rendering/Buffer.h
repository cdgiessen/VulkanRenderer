#pragma once

#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "Descriptor.h"
#include "RenderTools.h"

class VulkanDevice;

class VulkanBuffer {
public:
	VulkanBuffer(VulkanDevice& device);
	VulkanBuffer(VulkanDevice& device, VkDescriptorType type);

	VulkanBuffer(const VulkanBuffer& buf) = default;
	VulkanBuffer& operator=(const VulkanBuffer& buf) = default;

	virtual ~VulkanBuffer();

	void CleanBuffer();

	void Map(void** pData);
	void Unmap();

	void Flush();

	void CopyToBuffer(void* pData, VkDeviceSize size);

	VkDeviceSize Size() const;

	bool IsCreated() const;

	VmaBuffer buffer;
	DescriptorResource resource;

	//Give the desired prperties (usage specifiers), and optionally the allocation flags
	//if memToCopy is not a nullprt, then it will attempt to copy the data at memToCopy into the
	//newly created buffer 
	void SetupBuffer(VkDeviceSize bufferSize, 
		VkBufferUsageFlags bufferUsage, VmaMemoryUsage allocUsage, 
		VmaAllocationCreateFlags allocFlags = (VmaAllocationCreateFlagBits)(0), 
		void* memToCopy = nullptr);

protected:
	bool created = false;

	VulkanDevice * device;

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
	VulkanBufferStagingResource(VulkanDevice& device);

	void CreateStagingResourceBuffer(void* pData, VkDeviceSize size);
};

class VulkanBufferData : public VulkanBuffer {
public:
	VulkanBufferData(VulkanDevice& device);

	void CreateDataBuffer(VkDeviceSize size);
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

	void CreatePersistantInstanceBuffer(uint32_t count, uint32_t indexElementCount);

	void BindInstanceBuffer(VkCommandBuffer cmdBuf);


};

