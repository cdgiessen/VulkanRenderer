#include "Device.hpp"

#include "Initializers.hpp"
#include "RendererStructs.h"
#include "SwapChain.hpp"
#include "Buffer.hpp"

#include "../core/Logger.h"


TransferQueue::TransferQueue(VulkanDevice& device, VkCommandPool transfer_queue_command_pool, VkQueue transfer_queue, uint32_t transferFamily):
	device(device), transfer_queue_command_pool(transfer_queue_command_pool), transfer_queue(transfer_queue){

	//vkGetDeviceQueue(device, transferFamily, 0, &transfer_queue);
}

TransferQueue::~TransferQueue() {
	//vkDestroyCommandPool(device, transfer_queue_command_pool, nullptr);
}

VkCommandBuffer TransferQueue::GetTransferCommandBuffer() {
	VkCommandBuffer buf;
	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(transfer_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	transferLock.lock();
	vkAllocateCommandBuffers(device.device, &allocInfo, &buf);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;


	vkBeginCommandBuffer(buf, &beginInfo);
	transferLock.unlock();
	return buf;
}

void WaitForSubmissionFinish(VkDevice device, VkCommandPool transfer_queue_command_pool, 
	VkCommandBuffer buf, VkFence fence, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean) {

	vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
	if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
		for (auto& sig : readySignal)
			*sig = true;		
	}
	else if (vkGetFenceStatus(device, fence) == VK_NOT_READY) {
		Log::Error << "Transfer exeeded maximum fence timeout! Is too much stuff happening?\n";
		vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
		if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
			for (auto& sig : readySignal)
				*sig = true;
		}
	}
	else if (vkGetFenceStatus(device, fence) == VK_ERROR_DEVICE_LOST){
		Log::Error << "AAAAAAAAAAAHHHHHHHHHHHHH EVERYTHING IS ONE FIRE\n";
		throw std::runtime_error("Fence lost device!\n");
	}


	vkDestroyFence(device, fence, nullptr);
	//transferLock.lock(); // need to pass a lock in? should have a better synchronization structure
	vkFreeCommandBuffers(device, transfer_queue_command_pool, 1, &buf);
	//transferLock.unlock();
	for (auto& buffer : bufsToClean) {
		buffer.CleanBuffer();
	}
}

void TransferQueue::SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean) {
	vkEndCommandBuffer(buf);

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buf;

	VkFence fence;
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

	transferLock.lock();
	vkQueueSubmit(transfer_queue, 1, &submitInfo, fence);
	transferLock.unlock();

	std::thread submissionCompletion = std::thread(WaitForSubmissionFinish, device.device, transfer_queue_command_pool, buf, fence, readySignal, bufsToClean);
	submissionCompletion.detach();
}

void TransferQueue::SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal) {
	SubmitTransferCommandBuffer(buf, readySignal, {});
}

void TransferQueue::SubmitTransferCommandBufferAndWait(VkCommandBuffer buf) {
	vkEndCommandBuffer(buf);

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buf;

	VkFence fence;
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

	transferLock.lock();
	vkQueueSubmit(transfer_queue, 1, &submitInfo, fence);
	transferLock.unlock();

	vkWaitForFences(device.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
	vkDestroyFence(device.device, fence, nullptr);
	transferLock.lock();
	vkFreeCommandBuffers(device.device, transfer_queue_command_pool, 1, &buf);
	transferLock.unlock();
}






VulkanDevice::VulkanDevice(bool validationLayers) : enableValidationLayers(validationLayers)
{

}

VulkanDevice::~VulkanDevice()
{
	Log::Debug << "device deleted\n";
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
	Log::Debug << "validation layer: " << msg << "\n";// << "\n";

	return VK_FALSE;
}

void VulkanDevice::InitVulkanDevice(VkSurfaceKHR &surface)
{
	createInstance("My Vulkan App");
	setupDebugCallback();
	createSurface(surface);
	pickPhysicalDevice(surface);
	FindQueueFamilies(surface);

	createLogicalDevice();
	createCommandPools();
	CreateVulkanAllocator();

	//when the hardware doesn't have a separate transfer queue ( can't do asynchronous submissions)
	if (familyIndices.graphicsFamily != familyIndices.transferFamily) {
		transferQueue = std::make_unique<TransferQueue>(*this, transfer_queue_command_pool, transfer_queue, familyIndices.transferFamily);
		separateTransferQueue = true;
		Log::Debug << "Using a Separate transfer queue\n";
	}
	else
		separateTransferQueue = false;
	
}

void VulkanDevice::Cleanup(VkSurfaceKHR &surface) {
	vmaDestroyAllocator(allocator);

	vkDestroyCommandPool(device, graphics_queue_command_pool, nullptr);
	vkDestroyCommandPool(device, compute_queue_command_pool, nullptr);
	vkDestroyCommandPool(device, transfer_queue_command_pool, nullptr);

	vkDestroyDevice(device, nullptr);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {

	QueueFamilyIndices indices = FindQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = VulkanSwapChain::querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
	}

	vkGetPhysicalDeviceFeatures(device, &physical_device_features);

	return indices.isComplete() && extensionsSupported && physical_device_features.samplerAnisotropy;
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool VulkanDevice::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : VALIDATION_LAYERS) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void VulkanDevice::CreateVulkanAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physical_device;
	allocatorInfo.device = device;
	
	VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &allocator));

}

void VulkanDevice::VmaMapMemory(VmaBuffer& buffer, void** pData) {
	vmaMapMemory(allocator, buffer.allocation, pData);

}

void VulkanDevice::VmaUnmapMemory(VmaBuffer& buffer) {
	vmaUnmapMemory(allocator, buffer.allocation);
}

void VulkanDevice::CreateUniformBufferMapped(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	
	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
}

void VulkanDevice::CreateUniformBuffer(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	
	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
}

void VulkanDevice::CreateStagingUniformBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo();
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));

	memcpy(buffer.allocationInfo.pMappedData, data, bufferSize);
}

void VulkanDevice::CreateDynamicUniformBuffer		(VmaBuffer& buffer, uint32_t count, VkDeviceSize sizeOfData) {
	size_t minUboAlignment = physical_device_properties.limits.minUniformBufferOffsetAlignment;
	size_t dynamicAlignment = sizeof(sizeOfData);
	if (minUboAlignment > 0) {
		dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = count * dynamicAlignment;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));


}

void VulkanDevice::CreateMeshBufferVertex(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
}

void VulkanDevice::CreateMeshBufferIndex(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
}

void VulkanDevice::CreateMeshStagingBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo();
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));

	memcpy(buffer.allocationInfo.pMappedData, data, bufferSize);
}

void VulkanDevice::DestroyVmaAllocatedBuffer(VkBuffer* buffer, VmaAllocation* allocation) {
	vmaDestroyBuffer(allocator, *buffer, *allocation);
}

void VulkanDevice::DestroyVmaAllocatedBuffer(VmaBuffer& buffer) {
	vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

void VulkanDevice::CreateImage2D(VkImageCreateInfo imageInfo, VmaImage& image) {

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageInfo, &imageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));
}

void VulkanDevice::CreateDepthImage(VkImageCreateInfo imageInfo, VmaImage& image) {
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageInfo, &imageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));
}

void VulkanDevice::CreateStagingImage2D(VkImageCreateInfo imageInfo, VmaImage& image) {

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo stagingImageAllocCreateInfo = {};
	stagingImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	stagingImageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageInfo, &stagingImageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));

}

void VulkanDevice::DestroyVmaAllocatedImage(VmaImage& image) {
	vmaDestroyImage(allocator, image.image, image.allocation);
}

/**
* Create a buffer on the device
*
* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
* @param size Size of the buffer in byes
* @param buffer Pointer to the buffer handle acquired by the function
* @param memory Pointer to the memory handle acquired by the function
* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
*
* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
*/
//VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data)
//{
//	// Create the buffer handle
//	VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo(usageFlags, size);
//	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//	VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer));
//
//	// Create the memory backing up the buffer handle
//	VkMemoryRequirements memReqs;
//	VkMemoryAllocateInfo memAlloc = initializers::memoryAllocateInfo();
//	vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
//	memAlloc.allocationSize = memReqs.size;
//	// Find a memory type index that fits the properties of the buffer
//	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
//	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, memory));
//
//	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
//	if (data != nullptr)
//	{
//		void *mapped;
//		VK_CHECK_RESULT(vkMapMemory(device, *memory, 0, size, 0, &mapped));
//		memcpy(mapped, data, size);
//		// If host coherency hasn't been requested, do a manual flush to make writes visible
//		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
//		{
//			VkMappedMemoryRange mappedRange = initializers::mappedMemoryRange();
//			mappedRange.memory = *memory;
//			mappedRange.offset = 0;
//			mappedRange.size = size;
//			vkFlushMappedMemoryRanges(device, 1, &mappedRange);
//		}
//		vkUnmapMemory(device, *memory);
//	}
//
//	// Attach the memory to the buffer object
//	VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *memory, 0));
//
//	return VK_SUCCESS;
//}

/**
* Create a buffer on the device
*
* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
* @param buffer Pointer to a vk::Vulkan buffer object
* @param size Size of the buffer in bytes
* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
*
* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
*/
//VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer *buffer, VkDeviceSize size, void *data)
//{
//	buffer->device = device;
//
//	// Create the buffer handle
//	VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo(usageFlags, size);
//	VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer->buffer));
//
//	// Create the memory backing up the buffer handle
//	VkMemoryRequirements memReqs;
//	vkGetBufferMemoryRequirements(device, buffer->buffer, &memReqs);
//
//	VkMemoryAllocateInfo memAlloc = initializers::memoryAllocateInfo();
//	memAlloc.allocationSize = memReqs.size;
//	// Find a memory type index that fits the properties of the buffer
//	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
//	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &buffer->bufferMemory));
//
//	buffer->alignment = memReqs.alignment;
//	buffer->size = memAlloc.allocationSize;
//	buffer->usageFlags = usageFlags;
//	buffer->memoryPropertyFlags = memoryPropertyFlags;
//
//	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
//	if (data != nullptr)
//	{
//		VK_CHECK_RESULT(buffer->map(device));
//		memcpy(buffer->mapped, data, size);
//		buffer->unmap();
//	}
//
//	// Initialize a default descriptor that covers the whole buffer size
//	buffer->setupDescriptor();
//
//	// Attach the memory to the buffer object
//	return buffer->bind();
//}

/**
* Get the index of a memory type that has all the requested property bits set
*
* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
* @param properties Bitmask of properties for the memory type to request
* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
*
* @return Index of the requested memory type
*
* @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
*/
uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound)
{

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memTypeFound)
				{
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}


	if (memTypeFound)
	{
		*memTypeFound = false;
		return 0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}

}


/**
* Allocate a command buffer from the command pool
*
* @param level Level of the new command buffer (primary or secondary)
* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
*
* @return A handle to the allocated command buffer
*/
VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandPool commandPool, VkCommandBufferLevel level, bool begin)
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = initializers::commandBufferAllocateInfo(commandPool, level, 1);

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

	// If requested, also start recording for the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	}

	return cmdBuffer;
}

/**
* Finish command buffer recording and submit it to a queue
*
* @param commandBuffer Command buffer to flush
* @param queue Queue to submit the command buffer to
* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
*
* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
* @note Uses a fence to ensure command buffer has finished executing
*/
void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}


	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(device, fence, nullptr);

	if (free)
	{
		vkFreeCommandBuffers(device, graphics_queue_command_pool, 1, &commandBuffer);
	}
}

void VulkanDevice::flushCommandBuffer(VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}


	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(device, fence, nullptr);

	if (free)
	{
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}
}

VkCommandBuffer VulkanDevice::GetTransferCommandBuffer() {
	if (separateTransferQueue) {
		return transferQueue->GetTransferCommandBuffer();
	} else {
		VkCommandBuffer buf;
		VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(transfer_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

		vkAllocateCommandBuffers(device, &allocInfo, &buf);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(buf, &beginInfo);
		return buf;
		
	}
}

void VulkanDevice::SubmitTransferCommandBufferAndWait(VkCommandBuffer buf) {
	if (separateTransferQueue) {
		transferQueue->SubmitTransferCommandBufferAndWait(buf);
	} else {
		vkEndCommandBuffer(buf);

		VkSubmitInfo submitInfo = initializers::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buf;

		vkQueueSubmit(transfer_queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(transfer_queue);

		vkFreeCommandBuffers(device, transfer_queue_command_pool, 1, &buf);
		isDmaCmdBufWritable = false;
	}
}

void VulkanDevice::SubmitTransferCommandBuffer(VkCommandBuffer buf, 
	std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean) {
	if (separateTransferQueue) {
		transferQueue->SubmitTransferCommandBuffer(buf, readySignal, bufsToClean);
	}
	else {
		vkEndCommandBuffer(buf);

		VkSubmitInfo submitInfo = initializers::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buf;

		vkQueueSubmit(transfer_queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(transfer_queue);

		for (auto& buffer : bufsToClean) {
			buffer.CleanBuffer();
		}
		vkFreeCommandBuffers(device, transfer_queue_command_pool, 1, &buf);
	}
}

void VulkanDevice::SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal) {
	SubmitTransferCommandBuffer(buf, readySignal, {});
}

void VulkanDevice::createInstance(std::string appName) {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		Log::Debug << "validation layers requested, but not available! " << "\n";
		enableValidationLayers = false;
		//throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	createInfo.enabledLayerCount = 0;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}



void VulkanDevice::setupDebugCallback() {
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}



void VulkanDevice::createSurface(VkSurfaceKHR &surface) {
	//VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
	VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (res != VK_SUCCESS) {
		Log::Error << errorString(res) << "\n";
		throw std::runtime_error("failed to create window surface!");
	}
}


void VulkanDevice::pickPhysicalDevice(VkSurfaceKHR &surface) {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (isDeviceSuitable(device, surface)) {
			physical_device = device;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

//Put all device specific features here
VkPhysicalDeviceFeatures VulkanDevice::QueryDeviceFeatures() {
	VkPhysicalDeviceFeatures deviceFeatures = {};
	if (physical_device_features.samplerAnisotropy)
		deviceFeatures.samplerAnisotropy = VK_TRUE;

	if (physical_device_features.fillModeNonSolid)
		deviceFeatures.fillModeNonSolid = VK_TRUE;

	if (physical_device_features.geometryShader)
		deviceFeatures.geometryShader = VK_TRUE;

	if (physical_device_features.tessellationShader)
		deviceFeatures.tessellationShader = VK_TRUE;

	return deviceFeatures;
}

void VulkanDevice::FindQueueFamilies(VkSurfaceKHR windowSurface) {
	familyIndices = FindQueueFamilies(physical_device, windowSurface);
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice physDevice, VkSurfaceKHR windowSurface) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

	//finds a transfer only queue
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0
			&& (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			&& ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
			&& ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {

			indices.transferFamily = i;
		}
		i++;
	}

	//finds graphics, present, and optionally a compute queue
	i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, windowSurface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily = i;
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			indices.computeFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	//make sure that they are set since they might get used
	if (indices.transferFamily == -1) {
		indices.transferFamily = 0;
	}
	if (indices.computeFamily == -1) {
		indices.computeFamily = 0;
	}

	return indices;
}

const QueueFamilyIndices VulkanDevice::GetFamilyIndices() const {
	return familyIndices;
}

void VulkanDevice::createLogicalDevice() {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = 
	{ familyIndices.graphicsFamily, familyIndices.presentFamily, familyIndices.computeFamily, familyIndices.transferFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = QueryDeviceFeatures();

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, familyIndices.graphicsFamily, 0, &graphics_queue);
	vkGetDeviceQueue(device, familyIndices.presentFamily, 0, &present_queue);
	vkGetDeviceQueue(device, familyIndices.computeFamily, 0, &compute_queue);
	vkGetDeviceQueue(device, familyIndices.transferFamily, 0, &transfer_queue);

	vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memoryProperties);
}

void VulkanDevice::createCommandPools() {

	// graphics_queue_command_pool
	{
		VkCommandPoolCreateInfo pool_info = initializers::commandPoolCreateInfo();
		pool_info.queueFamilyIndex = familyIndices.graphicsFamily;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional
							 // hint the command pool will rerecord buffers by VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
							 // allow buffers to be rerecorded individually by VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		if (vkCreateCommandPool(device, &pool_info, nullptr, &graphics_queue_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}

	// compute_queue_command_pool
	{
		VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo();
		cmd_pool_info.queueFamilyIndex = familyIndices.computeFamily;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &cmd_pool_info, nullptr, &compute_queue_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}

	// transfer_queue_command_pool
	{
		VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo();
		cmd_pool_info.queueFamilyIndex = familyIndices.transferFamily;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		if (vkCreateCommandPool(device, &cmd_pool_info, nullptr, &transfer_queue_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}
}

VkResult VulkanDevice::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanDevice::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}
