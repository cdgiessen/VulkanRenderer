#pragma once

#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "Descriptor.h"
#include "RenderTools.h"

class VulkanDevice;

class VulkanBuffer {
public:
	enum class PersistantlyMapped {//can't muck up types this way
		F,
		T
	};

	enum class DynamicallyAligned {
		F,
		T
	};

	//VulkanBuffer(VulkanDevice& device);
	VulkanBuffer(VulkanDevice& device, VkDescriptorType type, VkDeviceSize bufferSize,
		VkBufferUsageFlags bufferUsage, VmaMemoryUsage allocUsage,
		VmaAllocationCreateFlags allocFlags = (VmaAllocationCreateFlagBits)(0),
		void* memToCopy = nullptr,
		PersistantlyMapped persistantlyMapped = PersistantlyMapped::F,
		DynamicallyAligned dynamicAlignment = DynamicallyAligned::F, int count = 0);

	VulkanBuffer(const VulkanBuffer& buf) = delete;
	VulkanBuffer& operator=(const VulkanBuffer& buf) = delete;

	VulkanBuffer(VulkanBuffer&& buf);
	VulkanBuffer& operator=(VulkanBuffer&& buf);


	virtual ~VulkanBuffer();

	void CleanBuffer();

	void Map(void** pData);
	void Unmap();

	void Flush();

	void CopyToBuffer(void* pData, VkDeviceSize size);

	VkDeviceSize Size() const;

	bool IsCreated() const;

	struct VmaBuffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo;
		VmaAllocator allocator = nullptr;
	} buffer;
	DescriptorResource resource;

	//Give the desired prperties (usage specifiers), and optionally the allocation flags
	//if memToCopy is not a nullprt, then it will attempt to copy the data at memToCopy into the
	//newly created buffer 
	/*void SetupBuffer(VkDeviceSize bufferSize,
		VkBufferUsageFlags bufferUsage, VmaMemoryUsage allocUsage,
		VmaAllocationCreateFlags allocFlags = (VmaAllocationCreateFlagBits)(0),
		void* memToCopy = nullptr);*/

protected:
	VulkanDevice * device;
	VkDeviceSize m_size;
	void* mapped;

	bool created = false;
	bool movedFrom = false;

	bool persistantlyMapped = false;


	void SetupResource();


};

class VulkanBufferUniform : public VulkanBuffer {
public:
	VulkanBufferUniform(VulkanDevice& device, VkDeviceSize size);
};

class VulkanBufferUniformPersistant : public VulkanBuffer {
public:
	VulkanBufferUniformPersistant(VulkanDevice& device, VkDeviceSize size);
};

class VulkanBufferUniformStaging : public VulkanBuffer {
public:
	VulkanBufferUniformStaging(VulkanDevice& device, VkDeviceSize size, void* pData);
};

//class VulkanBufferUniform : public VulkanBuffer {
//public:
//	VulkanBufferUniform(VulkanDevice& device, VkDeviceSize size, bool persistantMapped, );
//
//	void CreateUniformBuffer(VkDeviceSize size);
//	void CreateUniformBufferPersitantlyMapped(VkDeviceSize size);
//	void CreateStagingUniformBuffer(void* pData, VkDeviceSize size);
//};

class VulkanBufferUniformDynamic : public VulkanBuffer {
public:
	VulkanBufferUniformDynamic(VulkanDevice& device, VkDeviceSize size, uint32_t count);

	//void CreateDynamicUniformBuffer(uint32_t count, VkDeviceSize size);
};

class VulkanBufferStagingResource : public VulkanBuffer {
public:
	VulkanBufferStagingResource(VulkanDevice& device, VkDeviceSize size, void* pData);

	//void CreateStagingResourceBuffer(void* pData, VkDeviceSize size);
};

class VulkanBufferData : public VulkanBuffer {
public:
	VulkanBufferData(VulkanDevice& device, VkDeviceSize size);

	//void CreateDataBuffer(VkDeviceSize size);
};

class VulkanBufferVertex : public VulkanBuffer {
public:
	VulkanBufferVertex(VulkanDevice& device, uint32_t count, uint32_t vertexElementCount);

	void BindVertexBuffer(VkCommandBuffer cmdBuf);

	//void CreateVertexBuffer(uint32_t count, uint32_t vertexElementCount);
	//void CreateStagingVertexBuffer(void* pData, uint32_t count, uint32_t vertexElementCount);
};

class VulkanBufferStagingVertex : public VulkanBuffer {
public:
	VulkanBufferStagingVertex(VulkanDevice& device, uint32_t count, uint32_t vertexElementCount, void* pData);

};

class VulkanBufferIndex : public VulkanBuffer {
public:
	VulkanBufferIndex(VulkanDevice& device, uint32_t count);
	void BindIndexBuffer(VkCommandBuffer cmdBuf);

	//void CreateIndexBuffer(uint32_t count);
	//void CreateStagingIndexBuffer(void* pData, uint32_t count);
};

class VulkanBufferStagingIndex : public VulkanBuffer {
public:
	VulkanBufferStagingIndex(VulkanDevice& device, uint32_t count, void* pData);

};

class VulkanBufferInstance : public VulkanBuffer {
public:
	VulkanBufferInstance(VulkanDevice& device, uint32_t count, uint32_t indexElementCount);
	void BindInstanceBuffer(VkCommandBuffer cmdBuf);

	//void CreateInstanceBuffer(uint32_t count, uint32_t indexElementCount);
	//void CreateStagingInstanceBuffer(void* pData, uint32_t count, uint32_t indexElementCount);

	//void CreatePersistantInstanceBuffer(uint32_t count, uint32_t indexElementCount);

};

class VulkanBufferStagingInstance : public VulkanBuffer {
public:
	VulkanBufferStagingInstance(VulkanDevice& device, uint32_t count, uint32_t indexElementCount, void* pData);
};

class VulkanBufferInstancePersistant : public VulkanBuffer {
public:
	VulkanBufferInstancePersistant(VulkanDevice& device, uint32_t count, uint32_t indexElementCount);
};