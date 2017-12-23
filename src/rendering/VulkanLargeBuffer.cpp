#include "VulkanLargeBuffer.h"


VulkanLargeBuffer::VulkanLargeBuffer(std::shared_ptr<VulkanDevice> device, VkBufferUsageFlagBits usageFlags, VkDeviceSize size)
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
	memAlloc.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, deviceMemoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAlloc, nullptr, &buffer.bufferMemory));

	buffer.alignment = memReqs.alignment;
	buffer.size = memAlloc.allocationSize;
	buffer.usageFlags = usageFlags;
	buffer.memoryPropertyFlags = deviceMemoryPropertyFlags;



	//host memory buffer creation
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		hostMemoryPropertyFlags,
		&stagingBuffer,
		size));
}


VulkanLargeBuffer::~VulkanLargeBuffer()
{
}

VkDeviceMemory VulkanLargeBuffer::StageResource() {
	return 0;
}

bool VulkanLargeBuffer::TransferBuffers() {
	return false;
}
