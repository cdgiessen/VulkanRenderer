#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "Descriptor.h"

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

namespace details
{
struct BufData
{
	VulkanDevice* device;
	VkDeviceSize m_size;

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

	VulkanBuffer (VulkanBuffer&& buf);
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


	VkBuffer buffer = VK_NULL_HANDLE;

	DescriptorResource resource;

	private:
	details::BufData data;

	void CopyToBuffer (void const* pData, size_t size);
};

using BufferID = int;

class BufferManager
{
	public:
	BufferManager (VulkanDevice& device);

	BufferID CreateBuffer (BufCreateDetails details);
	void FreeBuffer (BufferID id);

	VulkanBuffer& GetBuffer (BufferID);

	private:
	VulkanDevice& device;

	BufferID buf_index = 0;
	std::mutex map_lock;
	std::unordered_map<BufferID, VulkanBuffer> buffer_map;
};