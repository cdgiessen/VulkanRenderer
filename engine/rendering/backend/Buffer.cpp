#include "Buffer.h"

#include <cassert>
#include <cstring>

#include "Device.h"
#include "RenderTools.h"
#include "rendering/Initializers.h"

const uint32_t VERTEX_BUFFER_BIND_ID = 0;
const uint32_t INSTANCE_BUFFER_BIND_ID = 1;

VulkanBuffer::VulkanBuffer (VulkanDevice& device, BufCreateDetails details)
{
	data.device = &device;
	data.m_size = details.bufferSize * details.elem_count;
	data.type = details.type;
	data.element_count = details.elem_count;

	if (details.dynamicAlignment == true)
	{
		size_t minUboAlignment = device.phys_device.properties.limits.minUniformBufferOffsetAlignment;
		data.alignment = data.m_size;
		if (minUboAlignment > 0)
		{
			data.alignment = (data.alignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		data.m_size = details.elem_count * data.alignment;
	}

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = data.m_size;
	bufferInfo.usage = details.bufferUsage;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = details.allocUsage;
	allocInfo.flags = details.allocFlags;

	data.allocator = device.get_general_allocator ();

	VK_CHECK_RESULT (vmaCreateBuffer (
	    data.allocator, &bufferInfo, &allocInfo, &buffer, &data.allocation, &data.allocationInfo));

	if (details.persistentlyMapped == true)
	{
		data.persistentlyMapped = true;
		map (&data.mapped);
	}
}

VulkanBuffer::~VulkanBuffer ()
{
	if (data.allocator != nullptr && buffer != VK_NULL_HANDLE && data.allocation != VK_NULL_HANDLE)
	{
		if (data.persistentlyMapped)
		{
			unmap ();
		}
		vmaDestroyBuffer (data.allocator, buffer, data.allocation);
	}
}

VulkanBuffer::VulkanBuffer (VulkanBuffer&& other) noexcept
: buffer (other.buffer), data (other.data)
{
	other.buffer = VK_NULL_HANDLE;
	other.data.allocation = VK_NULL_HANDLE;
	other.data.allocator = nullptr;
}
VulkanBuffer& VulkanBuffer::operator= (VulkanBuffer&& other) noexcept
{
	if (this != &other)
	{
		buffer = other.buffer;
		data = other.data;

		other.buffer = VK_NULL_HANDLE;
		other.data.allocation = VK_NULL_HANDLE;
		other.data.allocator = nullptr;
	}

	return *this;
}

void VulkanBuffer::map (void** pData) { vmaMapMemory (data.allocator, data.allocation, pData); }
void VulkanBuffer::unmap () { vmaUnmapMemory (data.allocator, data.allocation); }

void VulkanBuffer::flush ()
{
	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties (data.allocator, data.allocationInfo.memoryType, &memFlags);
	if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
		VkMappedMemoryRange memRange{};
		memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memRange.memory = data.allocationInfo.deviceMemory;
		memRange.offset = data.allocationInfo.offset;
		memRange.size = data.allocationInfo.size;
		vkFlushMappedMemoryRanges (data.device->device, 1, &memRange);
	}
}

VkDeviceSize VulkanBuffer::size () const { return data.m_size; }

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

void VulkanBuffer::copy_to_buffer (void const* pData, size_t size)
{
	if (data.persistentlyMapped)
	{
		assert (data.mapped != nullptr);
		memcpy (data.mapped, pData, size);
	}
	else
	{
		this->map (&data.mapped);
		assert (data.mapped != nullptr);
		memcpy (data.mapped, pData, size);
		this->unmap ();
	}
}

void VulkanBuffer::bind_vertex_buffer (VkCommandBuffer cmdBuf)
{
	assert (data.type == BufferType::vertex);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers (cmdBuf, VERTEX_BUFFER_BIND_ID, 1, &buffer, offsets);
}

void VulkanBuffer::bind_index_buffer (VkCommandBuffer cmdBuf)
{
	assert (data.type == BufferType::index);
	vkCmdBindIndexBuffer (cmdBuf, buffer, 0, VK_INDEX_TYPE_UINT32);
}

void VulkanBuffer::bind_instance_buffer (VkCommandBuffer cmdBuf)
{
	assert (data.type == BufferType::instance);
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers (cmdBuf, INSTANCE_BUFFER_BIND_ID, 1, &buffer, offsets);
}

VkDescriptorType VulkanBuffer::get_descriptor_type ()
{
	VkDescriptorType descriptor_type;
	switch (data.type)
	{
		case (BufferType::uniform): descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
		case (BufferType::uniform_dynamic):
			descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			break;
		case (BufferType::storage): descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
		case (BufferType::storage_dynamic):
			descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			break;
		case (BufferType::vertex):
		case (BufferType::index):
		case (BufferType::instance):
		case (BufferType::staging):
		default: descriptor_type = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM; break;
	}
	return descriptor_type;
}

VkDescriptorBufferInfo VulkanBuffer::get_descriptor_info ()
{
	return initializers::descriptor_buffer_create_info (buffer, 0, data.m_size);
}

VkDescriptorBufferInfo VulkanBuffer::get_descriptor_info (int element_index)
{
	return initializers::descriptor_buffer_create_info (buffer, element_index * data.m_size, data.m_size);
}

VkDescriptorBufferInfo VulkanBuffer::get_descriptor_info (VkDeviceSize offset, VkDeviceSize range)
{
	return initializers::descriptor_buffer_create_info (buffer, offset, range);
}

VkBuffer VulkanBuffer::get () const { return buffer; }

//// DOUBLE BUFFER ////

DoubleBuffer::DoubleBuffer (VulkanDevice& device, BufCreateDetails const& create_details)
: buffers ({ VulkanBuffer{ device, create_details }, VulkanBuffer{ device, create_details } })
{
}

VulkanBuffer const& DoubleBuffer::read () { return buffers[cur_read]; }
VulkanBuffer& DoubleBuffer::Write () { return buffers[cur_write]; }

void DoubleBuffer::advance ()
{
	cur_read = cur_write;
	cur_write = (cur_write + 1) % 2;
}

VkDescriptorType DoubleBuffer::get_descriptor_type ()
{
	return buffers.at (0).get_descriptor_type ();
}

VkDescriptorBufferInfo DoubleBuffer::get_descriptor_info (int which)
{
	return buffers.at (which).get_descriptor_info ();
}
VkDescriptorBufferInfo DoubleBuffer::get_descriptor_info (int which, VkDeviceSize offset, VkDeviceSize range)
{
	return buffers.at (which).get_descriptor_info (offset, range);
}

VkDescriptorBufferInfo DoubleBuffer::get_descriptor_info (int which, int element_index)
{
	return buffers.at (which).get_descriptor_info (element_index);
}
