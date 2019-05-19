#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Descriptor.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

/*
Buffer types:



Vertex, index, instance
Staging: Vertex, index, instance

Uniform, Dynamic
Staging: Uniform

Persistant: Uniform, Instance
Data

*/

class VulkanDevice;

class VulkanBuffer
{
	public:
	enum class PersistentlyMapped
	{ // can't muck up types this way
		F,
		T
	};

	enum class DynamicallyAligned
	{
		F,
		T
	};

	// VulkanBuffer(VulkanDevice& device);
	explicit VulkanBuffer (VulkanDevice& device,
	    VkDescriptorType type,
	    VkDeviceSize bufferSize,
	    VkBufferUsageFlags bufferUsage,
	    VmaMemoryUsage allocUsage,
	    VmaAllocationCreateFlags allocFlags = (VmaAllocationCreateFlagBits) (0),
	    void* memToCopy = nullptr,
	    PersistentlyMapped persistentlyMapped = PersistentlyMapped::F,
	    DynamicallyAligned dynamicAlignment = DynamicallyAligned::F,
	    int count = 0);

	VulkanBuffer (const VulkanBuffer& buf) = delete;
	VulkanBuffer& operator= (const VulkanBuffer& buf) = delete;

	VulkanBuffer (VulkanBuffer&& buf);
	VulkanBuffer& operator= (VulkanBuffer&& buf);


	virtual ~VulkanBuffer ();

	void Map (void** pData);
	void Unmap ();

	void Flush ();

	void CopyToBuffer (void* pData, VkDeviceSize size);

	VkDeviceSize Size () const;

	bool IsCreated () const;

	struct VmaBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo;
		VmaAllocator allocator = nullptr;
	} buffer;
	DescriptorResource resource;

	protected:
	VulkanDevice* device;
	VkDeviceSize m_size;
	void* mapped;

	bool created = false;
	bool movedFrom = false;

	bool persistentlyMapped = false;

	size_t alignment = -1;
};

class VulkanBufferUniform : public VulkanBuffer
{
	public:
	VulkanBufferUniform (VulkanDevice& device, VkDeviceSize size);
};

class VulkanBufferUniformPersistant : public VulkanBuffer
{
	public:
	VulkanBufferUniformPersistant (VulkanDevice& device, VkDeviceSize size);
};

template <typename T> class VulkanBufferUniformArrayPersistant : public VulkanBuffer
{
	public:
	VulkanBufferUniformArrayPersistant (VulkanDevice& device, int count)
	: VulkanBuffer (device,
	      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	      sizeof (T) * count,
	      (VkBufferUsageFlags) (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
	      (VmaMemoryUsage)VMA_MEMORY_USAGE_CPU_TO_GPU,
	      VMA_ALLOCATION_CREATE_MAPPED_BIT,
	      nullptr,
	      PersistentlyMapped::T)
	{
	}

	void CopyArrayToBuffer (std::vector<T>& data)
	{
		if (data.size () > 0)
		{
			CopyToBuffer (data.data (), data.size () * sizeof (T));
		}
	}
};

class VulkanBufferUniformStaging : public VulkanBuffer
{
	public:
	VulkanBufferUniformStaging (VulkanDevice& device, VkDeviceSize size, void* pData);
};

class VulkanBufferUniformDynamic : public VulkanBuffer
{
	public:
	VulkanBufferUniformDynamic (VulkanDevice& device, VkDeviceSize size, uint32_t count);

	void BindDynamicBufferInstance (
	    VkCommandBuffer cmdBuf, uint32_t instance, VkPipelineLayout pipelineLayout, VkDescriptorSet* descriptorSet);
};

class VulkanBufferStagingResource : public VulkanBuffer
{
	public:
	VulkanBufferStagingResource (VulkanDevice& device, VkDeviceSize size, void* pData);
};

class VulkanBufferData : public VulkanBuffer
{
	public:
	VulkanBufferData (VulkanDevice& device, VkDeviceSize size);
};

class VulkanBufferVertex : public VulkanBuffer
{
	public:
	VulkanBufferVertex (VulkanDevice& device, uint32_t count, uint32_t vertexElementCount);
	VulkanBufferVertex (VulkanDevice& device, uint32_t float_count);

	void BindVertexBuffer (VkCommandBuffer cmdBuf);
};

class VulkanBufferStagingVertex : public VulkanBuffer
{
	public:
	VulkanBufferStagingVertex (VulkanDevice& device, uint32_t count, uint32_t vertexElementCount, void* pData);
	VulkanBufferStagingVertex (VulkanDevice& device, uint32_t float_count, void* pData);
};

class VulkanBufferIndex : public VulkanBuffer
{
	public:
	VulkanBufferIndex (VulkanDevice& device, uint32_t count);
	void BindIndexBuffer (VkCommandBuffer cmdBuf);
};

class VulkanBufferStagingIndex : public VulkanBuffer
{
	public:
	VulkanBufferStagingIndex (VulkanDevice& device, uint32_t count, void* pData);
};

class VulkanBufferInstance : public VulkanBuffer
{
	public:
	VulkanBufferInstance (VulkanDevice& device, uint32_t count, uint32_t indexElementCount);
	void BindInstanceBuffer (VkCommandBuffer cmdBuf);
};

class VulkanBufferStagingInstance : public VulkanBuffer
{
	public:
	VulkanBufferStagingInstance (VulkanDevice& device, uint32_t count, uint32_t indexElementCount, void* pData);
};

class VulkanBufferInstancePersistant : public VulkanBuffer
{
	public:
	VulkanBufferInstancePersistant (VulkanDevice& device, uint32_t count, uint32_t indexElementCount);
	void BindInstanceBuffer (VkCommandBuffer cmdBuf);
};