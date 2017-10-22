#include "VulkanLargeBuffer.h"


VulkanLargeBuffer::VulkanLargeBuffer(VulkanDevice* device, VkBufferUsageFlagBits usageFlags, VkDeviceSize size)
{

	this->device = device;

	buffer.device = device->device;

	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo(usageFlags, size);
	VK_CHECK_RESULT(vkCreateBuffer(device->device, &bufferCreateInfo, nullptr, &buffer.buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device->device, buffer.buffer, &memReqs);

	VkMemoryAllocateInfo memAlloc = initializers::memoryAllocateInfo();
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAlloc, nullptr, &buffer.bufferMemory));

	buffer.alignment = memReqs.alignment;
	buffer.size = memAlloc.allocationSize;
	buffer.usageFlags = usageFlags;
	buffer.memoryPropertyFlags = memoryPropertyFlags;


}


VulkanLargeBuffer::~VulkanLargeBuffer()
{
}

VkDeviceMemory* VulkanLargeBuffer::StageResource() {

}

bool VulkanLargeBuffer::TransferBuffers() {

}
