#pragma once

#include "VulkanDevice.hpp"
#include "VulkanInitializers.hpp"

VulkanDevice::VulkanDevice()
{

}

VulkanDevice::~VulkanDevice()
{

}

void VulkanDevice::initVulkanDevice(VkSurfaceKHR &surface)
{
	createInstance("My Vulkan App");
	setupDebugCallback();
	createSurface(surface);
	pickPhysicalDevice(surface);
	//findQueueFamilies();
	createLogicalDevice(surface);
	createCommandPool(surface);
}

void VulkanDevice::cleanup(VkSurfaceKHR &surface) {
	vkDestroyCommandPool(device, graphics_queue_command_pool, nullptr);
	vkDestroyCommandPool(device, compute_queue_command_pool, nullptr);
	vkDestroyCommandPool(device, transfer_queue_command_pool, nullptr);


	vkDestroyDevice(device, nullptr);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
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
VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data)
{
	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo(usageFlags, size);
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = initializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, memory));

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		void *mapped;
		VK_CHECK_RESULT(vkMapMemory(device, *memory, 0, size, 0, &mapped));
		memcpy(mapped, data, size);
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange mappedRange = initializers::mappedMemoryRange();
			mappedRange.memory = *memory;
			mappedRange.offset = 0;
			mappedRange.size = size;
			vkFlushMappedMemoryRanges(device, 1, &mappedRange);
		}
		vkUnmapMemory(device, *memory);
	}

	// Attach the memory to the buffer object
	VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *memory, 0));

	return VK_SUCCESS;
}

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
VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer *buffer, VkDeviceSize size, void *data)
{
	buffer->device = device;

	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo(usageFlags, size);
	VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer->buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, buffer->buffer, &memReqs);

	VkMemoryAllocateInfo memAlloc = initializers::memoryAllocateInfo();
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &buffer->bufferMemory));

	buffer->alignment = memReqs.alignment;
	buffer->size = memAlloc.allocationSize;
	buffer->usageFlags = usageFlags;
	buffer->memoryPropertyFlags = memoryPropertyFlags;

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		VK_CHECK_RESULT(buffer->map(device));
		memcpy(buffer->mapped, data, size);
		buffer->unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	buffer->setupDescriptor();

	// Attach the memory to the buffer object
	return buffer->bind();
}

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

void VulkanDevice::createInstance(std::string appName) {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
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

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
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
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
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

void VulkanDevice::createLogicalDevice(VkSurfaceKHR &surface) {
	QueueFamilyIndices indices = findQueueFamilies(physical_device, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily, indices.computeFamily, indices.transferFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	if (physical_device_features.fillModeNonSolid) {
		deviceFeatures.fillModeNonSolid = VK_TRUE;
	}
	if (physical_device_features.geometryShader) {
		deviceFeatures.geometryShader = VK_TRUE;
	}
	if (physical_device_features.tessellationShader) {
		deviceFeatures.tessellationShader = VK_TRUE;
	}

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

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphics_queue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &present_queue);
	vkGetDeviceQueue(device, indices.computeFamily, 0, &compute_queue);
	vkGetDeviceQueue(device, indices.transferFamily, 0, &transfer_queue);

	vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memoryProperties);
}

void VulkanDevice::createCommandPool(VkSurfaceKHR &surface) {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physical_device, surface);

	// graphics_queue_command_pool
	{
		VkCommandPoolCreateInfo pool_info = initializers::commandPoolCreateInfo();
		pool_info.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
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
		cmd_pool_info.queueFamilyIndex = queueFamilyIndices.computeFamily;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &cmd_pool_info, nullptr, &compute_queue_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}

	// transfer_queue_command_pool
	{
		VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo();
		cmd_pool_info.queueFamilyIndex = queueFamilyIndices.transferFamily;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &cmd_pool_info, nullptr, &transfer_queue_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}
}

VkResult VulkanDevice::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanDevice::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

