#pragma once


  
  

#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"
#include "VulkanDescriptor.hpp"




/**
* @brief Encapsulates access to a Vulkan buffer backed up by device memory
* @note To be filled by an external source like the VulkanDevice
*/

class VulkanBuffer {
public:
	VulkanBuffer();

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



class VulkanBadBuffer {
public:
	VkDevice device;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferMemory = VK_NULL_HANDLE;

	//DescriptorResource resource;
	VkDescriptorBufferInfo descriptor;
	VkDeviceSize size = 0;
	VkDeviceSize alignment = 0;
	void* mapped = nullptr;

	/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
	VkBufferUsageFlags usageFlags;
	/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
	VkMemoryPropertyFlags memoryPropertyFlags;

	void cleanBuffer() {
		if (buffer)
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}
		if (bufferMemory)
		{
			vkFreeMemory(device, bufferMemory, nullptr);
		}
	}

	/**
	* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	*
	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the buffer mapping call
	*/
	VkResult map(VkDevice device, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		return vkMapMemory(device, bufferMemory, offset, size, 0, &mapped);
	}

	/**
	* Unmap a mapped memory range
	*
	* @note Does not return a result as vkUnmapMemory can't fail
	*/
	void unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, bufferMemory);
			mapped = nullptr;
		}
	}

	/**
	* Attach the allocated memory block to the buffer
	*
	* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
	*
	* @return VkResult of the bindBufferMemory call
	*/
	VkResult bind(VkDeviceSize offset = 0)
	{
		return vkBindBufferMemory(device, buffer, bufferMemory, offset);
	}

	/**
	* Setup the default descriptor for this buffer
	*
	* @param size (Optional) Size of the memory range of the descriptor
	* @param offset (Optional) Byte offset from beginning
	*
	*/
	void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		descriptor.offset = offset;
		descriptor.buffer = buffer;
		descriptor.range = size;
	}

	/**
	* Copies the specified data to the mapped buffer
	*
	* @param data Pointer to the data to copy
	* @param size Size of the data to copy in machine units
	*
	*/
	void copyTo(void* data, VkDeviceSize size)
	{
		assert(mapped);
		memcpy(mapped, data, (size_t)size);
	}

	/**
	* Flush a memory range of the buffer to make it visible to the device
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the flush call
	*/
	VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = bufferMemory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}


	/**
	* Invalidate a memory range of the buffer to make it visible to the host
	*
	* @note Only required for non-coherent memory
	*
	* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	* @param offset (Optional) Byte offset from beginning
	*
	* @return VkResult of the invalidate call
	*/
	VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = bufferMemory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

	VkBuffer getBuffer() {
		return buffer;
	}

	VkDeviceMemory getBufferMemory() {
		return bufferMemory;
	}
};