#include "Device.h"

#include "Initializers.h"
#include "SwapChain.h"

#include "../core/Logger.h"




VulkanDevice::VulkanDevice(bool validationLayers) : enableValidationLayers(validationLayers)
{

}

VulkanDevice::~VulkanDevice()
{
	Log::Debug << "device deleted\n";
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	Log::Debug << "validation layer: " << msg << "\n";// << "\n";

	return VK_FALSE;
}

void VulkanDevice::InitVulkanDevice(VkSurfaceKHR &surface)
{
	CreateInstance("My Vulkan App");
	SetupDebugCallback();
	CreateSurface(surface);
	PickPhysicalDevice(surface);
	FindQueueFamilies(surface);

	CreateLogicalDevice();

	CreateQueues();

	CreateVulkanAllocator();
}

void VulkanDevice::Cleanup(VkSurfaceKHR &surface) {
	vmaDestroyAllocator(allocator);
	vmaDestroyAllocator(linear_allocator);
	vmaDestroyAllocator(optimal_allocator);

	vkDestroyDevice(device, nullptr);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {

	QueueFamilyIndices indices = FindQueueFamilies(device, surface);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = VulkanSwapChain::querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
	}

	vkGetPhysicalDeviceFeatures(device, &physical_device_features);

	return indices.isComplete() && extensionsSupported && physical_device_features.samplerAnisotropy;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
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

bool VulkanDevice::CheckValidationLayerSupport() {
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



void VulkanDevice::CreateInstance(std::string appName) {
	if (enableValidationLayers && !CheckValidationLayerSupport()) {
		Log::Debug << "validation layers requested, but not available! " << "\n";
		enableValidationLayers = false;
		//throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

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


	VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));

}



void VulkanDevice::SetupDebugCallback() {
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}



void VulkanDevice::CreateSurface(VkSurfaceKHR &surface) {
	//VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
	VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (res != VK_SUCCESS) {
		Log::Error << errorString(res) << "\n";
		throw std::runtime_error("failed to create window surface!");
	}
}


void VulkanDevice::PickPhysicalDevice(VkSurfaceKHR &surface) {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (IsDeviceSuitable(device, surface)) {
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

	//finds graphics, present and optionally a transfer and compute queue
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
	//if (indices.presentFamily == -1) {
	//	indices.presentFamily = 0;
	//}
	return indices;
}

const QueueFamilyIndices VulkanDevice::GetFamilyIndices() const {
	return familyIndices;
}

void VulkanDevice::CreateLogicalDevice() {
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

	vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
	vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memoryProperties);
}

void VulkanDevice::CreateQueues() {
	graphics_queue = std::make_unique<CommandQueue>(*this, familyIndices.graphicsFamily);
	singleQueueDevice = true;

	//Make sure it is a unique queue, else don't make anything (empty pointer means it isn't unique)
	if (familyIndices.graphicsFamily != familyIndices.computeFamily) {
		compute_queue = std::make_unique<CommandQueue>(*this, familyIndices.computeFamily);
		singleQueueDevice = false;
	}

	if (familyIndices.graphicsFamily != familyIndices.transferFamily) {
		transfer_queue = std::make_unique<CommandQueue>(*this, familyIndices.transferFamily);
		singleQueueDevice = false;
	}

	if (familyIndices.graphicsFamily != familyIndices.presentFamily) {
		present_queue = std::make_unique<CommandQueue>(*this, familyIndices.presentFamily);
		singleQueueDevice = false;
	}

}

CommandQueue& VulkanDevice::GraphicsQueue() {
	return *graphics_queue;
}
CommandQueue& VulkanDevice::ComputeQueue() {
	if (compute_queue)
		return *compute_queue;
	return GraphicsQueue();
}
CommandQueue& VulkanDevice::TransferQueue() {
	if (transfer_queue)
		return *transfer_queue;
	return GraphicsQueue();
}
CommandQueue& VulkanDevice::PresentQueue() {
	if (present_queue) {
		return *present_queue;

	}
	return GraphicsQueue();
}

void VulkanDevice::CreateVulkanAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physical_device;
	allocatorInfo.device = device;

	VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &allocator));

	VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &linear_allocator));
	VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &optimal_allocator));


}

void VulkanDevice::VmaMapMemory(VmaBuffer& buffer, void** pData) {
	vmaMapMemory(allocator, buffer.allocation, pData);

}

void VulkanDevice::VmaUnmapMemory(VmaBuffer& buffer) {
	vmaUnmapMemory(allocator, buffer.allocation);
}

void VulkanDevice::FlushBuffer(VmaBuffer& buffer) {
	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(*buffer.allocator, buffer.allocationInfo.memoryType, &memFlags);
	if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
		VkMappedMemoryRange memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		memRange.memory = buffer.allocationInfo.deviceMemory;
		memRange.offset = buffer.allocationInfo.offset;
		memRange.size = buffer.allocationInfo.size;
		vkFlushMappedMemoryRanges(device, 1, &memRange);
	}
}

void VulkanDevice::CreateUniformBufferMapped(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;
}

void VulkanDevice::CreateUniformBuffer(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;
}

void VulkanDevice::CreateStagingUniformBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo();
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;

	memcpy(buffer.allocationInfo.pMappedData, data, bufferSize);
}

void VulkanDevice::CreateDynamicUniformBuffer(VmaBuffer& buffer, uint32_t count, VkDeviceSize sizeOfData) {
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
	buffer.allocator = &allocator;

}

void VulkanDevice::CreateMeshBufferVertex(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;
}

void VulkanDevice::CreateMeshBufferIndex(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;
}

void VulkanDevice::CreateMeshStagingBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo();
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;

	memcpy(buffer.allocationInfo.pMappedData, data, bufferSize);
}

void VulkanDevice::CreateInstancingBuffer(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;
}

void VulkanDevice::CreateStagingInstancingBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo();
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;

	memcpy(buffer.allocationInfo.pMappedData, data, bufferSize);
}

void VulkanDevice::CreateMappedInstancingBuffer(VmaBuffer& buffer, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;
}

// void VulkanDevice::DestroyVmaAllocatedBuffer(VkBuffer* buffer, VmaAllocation* allocation) {
// 	vmaDestroyBuffer(allocator, *buffer, *allocation);
// }

void VulkanDevice::DestroyVmaAllocatedBuffer(VmaBuffer& buffer) {
	vmaDestroyBuffer(*buffer.allocator, buffer.buffer, buffer.allocation);
}

void VulkanDevice::CreateImage2D(VkImageCreateInfo imageInfo, VmaImage& image) {

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	if (imageInfo.tiling == VK_IMAGE_TILING_OPTIMAL) {
		VK_CHECK_RESULT(vmaCreateImage(optimal_allocator, &imageInfo, &imageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));
		image.allocator = &optimal_allocator;
	}
	else if (imageInfo.tiling == VK_IMAGE_TILING_LINEAR) {
		VK_CHECK_RESULT(vmaCreateImage(linear_allocator, &imageInfo, &imageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));
		image.allocator = &linear_allocator;
	}


}

void VulkanDevice::CreateDepthImage(VkImageCreateInfo imageInfo, VmaImage& image) {
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK_RESULT(vmaCreateImage(optimal_allocator, &imageInfo, &imageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));
	image.allocator = &optimal_allocator;
}

void VulkanDevice::CreateStagingImage2D(VkImageCreateInfo imageInfo, VmaImage& image) {

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo stagingImageAllocCreateInfo = {};
	stagingImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	stagingImageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageInfo, &stagingImageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));
	image.allocator = &allocator;

}

void VulkanDevice::CreateStagingImageBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize) {
	VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo();
	bufferInfo.size = bufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK_RESULT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));
	buffer.allocator = &allocator;

	memcpy(buffer.allocationInfo.pMappedData, data, bufferSize);
}


void VulkanDevice::DestroyVmaAllocatedImage(VmaImage& image) {

	vmaDestroyImage(*image.allocator, image.image, image.allocation);
}
