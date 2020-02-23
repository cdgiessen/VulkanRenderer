#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

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

// Correct usage for single element allocation
// bufferSize = whole_size, elem_count = 1
// for multi element allocations
// bufferSize = element_size, elem_count = number of elements in allocation
struct BufCreateDetails
{
	BufferType type = BufferType::uniform;
	VkDeviceSize bufferSize = 0;
	VkBufferUsageFlags bufferUsage = 0;
	VmaMemoryUsage allocUsage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	VmaAllocationCreateFlags allocFlags = (VmaAllocationCreateFlagBits) (0);
	uint32_t elem_count = 1; // cause 1 element is still an array
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
inline BufCreateDetails uniform_array_details (uint32_t elem_count, VkDeviceSize size_of_element)
{
	return BufCreateDetails{ BufferType::uniform,
		elem_count * size_of_element,
		(VkBufferUsageFlags) (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		(VmaMemoryUsage) (VMA_MEMORY_USAGE_CPU_TO_GPU),
		elem_count };
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

namespace details
{
struct BufData
{
	VulkanDevice* device;
	VkDeviceSize m_size;
	uint32_t element_count = 1;

	size_t alignment = -1;
	BufferType type;
	bool persistentlyMapped = false;

	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo;
	VmaAllocator allocator = nullptr;

	void* mapped;
};
} // namespace details

class VulkanBuffer
{
	public:
	explicit VulkanBuffer (VulkanDevice& device, BufCreateDetails details);
	~VulkanBuffer ();

	VulkanBuffer (const VulkanBuffer& buf) = delete;
	VulkanBuffer& operator= (const VulkanBuffer& buf) = delete;

	VulkanBuffer (VulkanBuffer&& buf) noexcept;
	VulkanBuffer& operator= (VulkanBuffer&& buf) noexcept;


	void Map (void** pData);
	void Unmap ();

	void Flush ();

	template <typename T> void CopyToBuffer (std::vector<T> const& data)
	{
		CopyToBuffer (static_cast<void const*> (data.data ()), sizeof (T) * data.size ());
	}

	template <typename T> void CopyToBuffer (T const& data)
	{
		CopyToBuffer (static_cast<void const*> (&data), sizeof (T));
	}


	VkDeviceSize Size () const;

	void BindVertexBuffer (VkCommandBuffer cmdBuf);
	void BindIndexBuffer (VkCommandBuffer cmdBuf);
	void BindInstanceBuffer (VkCommandBuffer cmdBuf);

	VkDescriptorType GetDescriptorType ();
	VkDescriptorBufferInfo GetDescriptorInfo ();
	VkDescriptorBufferInfo GetDescriptorInfo (VkDeviceSize offset, VkDeviceSize range);
	VkDescriptorBufferInfo GetDescriptorInfo (int element_index);

	VkBuffer Get () const;

	private:
	VkBuffer buffer = VK_NULL_HANDLE;

	details::BufData data;

	void CopyToBuffer (void const* pData, size_t size);
};

class DoubleBuffer
{
	public:
	DoubleBuffer (VulkanDevice& device, BufCreateDetails const& create_details);

	VulkanBuffer const& Read ();
	VulkanBuffer& Write ();

	void Advance ();

	private:
	int cur_write = 0;
	int cur_read = 1;
	std::array<VulkanBuffer, 2> buffers;
};