#include "Buffer.h"

#include <cstring>
#include <functional>

#include "Device.h"
#include "RenderTools.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

const uint32_t VERTEX_BUFFER_BIND_ID = 0;
const uint32_t INSTANCE_BUFFER_BIND_ID = 1;

VulkanBuffer::VulkanBuffer (VulkanDevice& device, BufCreateDetails details, void const* memToCopy)
: device (&device), m_size (details.bufferSize), resource (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER), details (details)
{
	switch (details.type)
	{
		case (BufferType::uniform):
			resource = DescriptorResource (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			break;
		case (BufferType::uniform_dynamic):
			resource = DescriptorResource (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
			break;
		case (BufferType::storage):
			resource = DescriptorResource (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			break;
		case (BufferType::storage_dynamic):
			resource = DescriptorResource (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
			break;
		case (BufferType::vertex):
		case (BufferType::index):
		case (BufferType::instance):
		case (BufferType::staging):
		default:
			resource = DescriptorResource ((VkDescriptorType) (0));
			break;
	}


	if (details.dynamicAlignment == true)
	{
		size_t minUboAlignment =
		    device.physical_device.physical_device_properties.limits.minUniformBufferOffsetAlignment;
		alignment = m_size;
		if (minUboAlignment > 0)
		{
			alignment = (alignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		m_size = details.elem_count * alignment;
	}

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_size;
	bufferInfo.usage = details.bufferUsage;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = details.allocUsage;
	allocInfo.flags = details.allocFlags;

	buffer.allocator = device.GetGeneralAllocator ();

	VK_CHECK_RESULT (vmaCreateBuffer (
	    buffer.allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));

	if (memToCopy != nullptr)
	{
		memcpy (buffer.allocationInfo.pMappedData, memToCopy, m_size);
	}

	if (details.persistentlyMapped == true)
	{
		this->persistentlyMapped = true;
		Map (&mapped);
	}

	resource.FillResource (buffer.buffer, 0, m_size);

	// Log::Debug << "Allocated buffer Memory\n";
}

VulkanBuffer::~VulkanBuffer ()
{
	if (persistentlyMapped)
	{
		Unmap ();
	}

	vmaDestroyBuffer (buffer.allocator, buffer.buffer, buffer.allocation);

	// Log::Debug << "Freed buffer Memory\n";
}

void VulkanBuffer::Map (void** pData) { vmaMapMemory (buffer.allocator, buffer.allocation, pData); }
void VulkanBuffer::Unmap () { vmaUnmapMemory (buffer.allocator, buffer.allocation); }

void VulkanBuffer::Flush ()
{
	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties (buffer.allocator, buffer.allocationInfo.memoryType, &memFlags);
	if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
		VkMappedMemoryRange memRange{};
		memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memRange.memory = buffer.allocationInfo.deviceMemory;
		memRange.offset = buffer.allocationInfo.offset;
		memRange.size = buffer.allocationInfo.size;
		vkFlushMappedMemoryRanges (device->device, 1, &memRange);
	}
}

VkDeviceSize VulkanBuffer::Size () const { return m_size; }

void AlignedMemcpy (uint8_t bytes, VkDeviceSize destMemAlignment, void* src, void* dst)
{
	int src_offset = 0;
	int dest_offset = 0;
	for (int i = 0; i < bytes; i++)
	{
		memcpy ((char*)dst + dest_offset, (char*)src + src_offset, sizeof (bytes));
		src_offset += 1;
		dest_offset += (int)destMemAlignment;
	}
}

void VulkanBuffer::CopyToBuffer (void* pData, VkDeviceSize size)
{
	if (persistentlyMapped)
	{
		assert (mapped != nullptr);
		memcpy (mapped, pData, (size_t)size);
	}
	else
	{
		this->Map (&mapped);
		assert (mapped != nullptr);
		memcpy (mapped, pData, (size_t)size);

		// VkDeviceSize bufAlignment = device->physical_device_properties.limits.minUniformBufferOffsetAlignment;

		// AlignedMemcpy((size_t)size, bufAlignment, pData, mapped);

		this->Unmap ();
	}
}

void VulkanBuffer::BindVertexBuffer (VkCommandBuffer cmdBuf)
{
	assert (details.type == BufferType::vertex);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers (cmdBuf, VERTEX_BUFFER_BIND_ID, 1, &buffer.buffer, offsets);
}

void VulkanBuffer::BindIndexBuffer (VkCommandBuffer cmdBuf)
{
	assert (details.type == BufferType::index);
	vkCmdBindIndexBuffer (cmdBuf, buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void VulkanBuffer::BindInstanceBuffer (VkCommandBuffer cmdBuf)
{
	assert (details.type == BufferType::instance);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers (cmdBuf, INSTANCE_BUFFER_BIND_ID, 1, &buffer.buffer, offsets);
}
