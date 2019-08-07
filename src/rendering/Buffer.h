#pragma once

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

enum class BufferType
{
	uniform,
	uniform_dynamic,
	storage,
	storage_dynamic,
	vertex,
	index,
	instance,
	staging
};

struct BufCreateDetails
{
	BufferType type = BufferType::uniform;
	VkDeviceSize bufferSize = 0;
	VkBufferUsageFlags bufferUsage = 0;
	VmaMemoryUsage allocUsage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	VmaAllocationCreateFlags allocFlags = (VmaAllocationCreateFlagBits) (0);
	int elem_count = 1; // cause 1 element is still an array
	bool persistentlyMapped = false;
	bool dynamicAlignment = false;
};

inline BufCreateDetails uniform_details (VkDeviceSize size)
{
	return BufCreateDetails{ BufferType::uniform,
		size,
		(VkBufferUsageFlags) (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		(VmaMemoryUsage) (VMA_MEMORY_USAGE_CPU_TO_GPU) };
}
inline BufCreateDetails uniform_dynamic_details (VkDeviceSize size)
{
	return BufCreateDetails{
		BufferType::uniform_dynamic,
		size,
		(VkBufferUsageFlags) (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		(VmaMemoryUsage) (VMA_MEMORY_USAGE_CPU_TO_GPU),
		VMA_ALLOCATION_CREATE_MAPPED_BIT,
	};
}
inline BufCreateDetails vertex_details (uint32_t elements, uint32_t element_size)
{
	return BufCreateDetails{ BufferType::vertex,
		elements * element_size * sizeof (float),
		(VmaMemoryUsage) (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_GPU_ONLY };
}
inline BufCreateDetails index_details (uint32_t count)
{
	return { BufferType::index,
		sizeof (uint32_t) * count,
		(VkBufferUsageFlags) (VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_GPU_ONLY };
}
inline BufCreateDetails instance_details (uint32_t elements, uint32_t element_size)
{
	return BufCreateDetails{ BufferType::instance,
		elements * element_size * sizeof (float),
		(VkBufferUsageFlags) (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_GPU_ONLY };
}
inline BufCreateDetails staging_details (BufferType type, VkDeviceSize size)
{
	return BufCreateDetails{ type,
		size,
		(VmaMemoryUsage) (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		VMA_ALLOCATION_CREATE_MAPPED_BIT };
}


class VulkanBuffer
{
	public:
	// Common Setup configs

	explicit VulkanBuffer (VulkanDevice& device, BufCreateDetails details, void const* memToCopy = nullptr);

	VulkanBuffer (const VulkanBuffer& buf) = delete;
	VulkanBuffer& operator= (const VulkanBuffer& buf) = delete;

	~VulkanBuffer ();

	void Map (void** pData);
	void Unmap ();

	void Flush ();

	void CopyToBuffer (void* pData, VkDeviceSize size);

	VkDeviceSize Size () const;

	bool IsCreated () const;

	void BindVertexBuffer (VkCommandBuffer cmdBuf);
	void BindIndexBuffer (VkCommandBuffer cmdBuf);
	void BindInstanceBuffer (VkCommandBuffer cmdBuf);

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

	bool persistentlyMapped = false;

	size_t alignment = -1;

	BufCreateDetails details;
};

// class VulkanBufferUniform : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferUniform (VulkanDevice& device, VkDeviceSize size);
// };

// class VulkanBufferUniformPersistant : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferUniformPersistant (VulkanDevice& device, VkDeviceSize size);
// };

// class VulkanBufferUniformStaging : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferUniformStaging (VulkanDevice& device, VkDeviceSize size, void* pData);
// };

// class VulkanBufferUniformDynamic : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferUniformDynamic (VulkanDevice& device, VkDeviceSize size, uint32_t count);

// 	void BindDynamicBufferInstance (
// 	    VkCommandBuffer cmdBuf, uint32_t instance, VkPipelineLayout pipelineLayout, VkDescriptorSet* descriptorSet);
// };

// class VulkanBufferStagingResource : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferStagingResource (VulkanDevice& device, VkDeviceSize size, void* pData);
// };

// class VulkanBufferData : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferData (VulkanDevice& device, VkDeviceSize size);
// };

// class VulkanBufferVertex : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferVertex (VulkanDevice& device, uint32_t count, uint32_t vertexElementCount);
// 	VulkanBufferVertex (VulkanDevice& device, uint32_t float_count);

// 	void BindVertexBuffer (VkCommandBuffer cmdBuf);
// };

// class VulkanBufferStagingVertex : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferStagingVertex (VulkanDevice& device, uint32_t count, uint32_t vertexElementCount, void* pData);
// 	VulkanBufferStagingVertex (VulkanDevice& device, uint32_t float_count, void* pData);
// };

// class VulkanBufferIndex : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferIndex (VulkanDevice& device, uint32_t count);
// 	void BindIndexBuffer (VkCommandBuffer cmdBuf);
// };

// class VulkanBufferStagingIndex : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferStagingIndex (VulkanDevice& device, uint32_t count, void* pData);
// };

// class VulkanBufferInstance : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferInstance (VulkanDevice& device, uint32_t count, uint32_t indexElementCount);
// 	void BindInstanceBuffer (VkCommandBuffer cmdBuf);
// };

// class VulkanBufferStagingInstance : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferStagingInstance (VulkanDevice& device, uint32_t count, uint32_t indexElementCount, void* pData);
// };

// class VulkanBufferInstancePersistant : public VulkanBuffer
// {
// 	public:
// 	VulkanBufferInstancePersistant (VulkanDevice& device, uint32_t count, uint32_t
// indexElementCount); 	void BindInstanceBuffer (VkCommandBuffer cmdBuf);
// };