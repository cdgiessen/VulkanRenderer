#include "Device.h"

#include <set>
#include <GLFW/glfw3.h>

#include "core/Logger.h"
#include "core/Window.h"

#include "Initializers.h"
#include "SwapChain.h"
#include "RenderTools.h"

std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_LUNARG_standard_validation"

};

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

//////// VMA Memory Resource ////////

void VMA_MemoryResource::Create (
    VkPhysicalDevice physical_device, VkDevice device, VkAllocationCallbacks* custom_allocator)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physical_device;
	allocatorInfo.device = device;
	allocatorInfo.pAllocationCallbacks = custom_allocator;

	VK_CHECK_RESULT (vmaCreateAllocator (&allocatorInfo, &allocator));
}
void VMA_MemoryResource::Free ()
{
	if (allocator)
	{
		vmaDestroyAllocator (allocator);
	}
}

void VMA_MemoryResource::LogVMA (bool detailedOutput)
{
	char* str;
	vmaBuildStatsString (allocator, &str, detailedOutput);
	Log.Debug (fmt::format ("Allocator Data Dump:\n {}\n", str));

	// Log::Debug << "Allocator Data Dump:\n" << std::string (str) << "\n";
	vmaFreeStatsString (allocator, str);
}

//////// VulkanInstance ////////

VulkanInstance::VulkanInstance (std::string appName, bool validationLayersEnabled, Window& window)
: validationLayersEnabled (validationLayersEnabled), window (window)
{
	if (validationLayersEnabled && !CheckValidationLayerSupport ())
	{
		Log.Error (fmt::format ("Validation layers requested, but not found!\n"));
		//		Log::Debug << "validation layers requested, but not available! "<< "\n";
		validationLayersEnabled = false;
		// throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName.c_str ();
	appInfo.applicationVersion = VK_MAKE_VERSION (1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_VERSION (1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION (1, 1, 0);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetWindowExtensions ();

	if (validationLayersEnabled)
	{
		extensions.push_back (VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t> (extensions.size ());
	createInfo.ppEnabledExtensionNames = extensions.data ();

	createInfo.enabledLayerCount = 0;
	if (validationLayersEnabled)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t> (VALIDATION_LAYERS.size ());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data ();
	}


	VK_CHECK_RESULT (vkCreateInstance (&createInfo, nullptr, &instance));

	SetupDebugCallback ();
	CreateSurface ();
}

VulkanInstance::~VulkanInstance ()
{
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	//DestroyDebugReportCallbackEXT (instance, callback, nullptr);
	vkDestroySurfaceKHR (instance, surface, nullptr);
	vkDestroyInstance (instance, nullptr);
}


bool VulkanInstance::CheckValidationLayerSupport ()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties (&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers (layerCount);
	vkEnumerateInstanceLayerProperties (&layerCount, availableLayers.data ());


	for (const char* layerName : VALIDATION_LAYERS)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp (layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

void VulkanInstance::CreateSurface ()
{
	// VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
	VkResult res = glfwCreateWindowSurface (instance, window.getWindowContext (), nullptr, &surface);
	if (res != VK_SUCCESS)
	{
		Log.Error (fmt::format ("{}\n", errorString (res)));

		//		Log::Error << errorString (res) << "\n";
		throw std::runtime_error ("failed to create window surface!");
	}
}

// VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugCallback (VkDebugReportFlagsEXT flags,
//     VkDebugReportObjectTypeEXT objType,
//     uint64_t obj,
//     size_t location,
//     int32_t code,
//     const char* layerPrefix,
//     const char* msg,
//     void* userData)
// {
// 	Log.Debug (fmt::format ("Validation Layer:\n{}\n", msg));

// 	// Log::Debug << "\nvalidation layer: " << msg << "\n"; // << "\n";
// 	return VK_FALSE;
// }

// VkResult VulkanInstance::CreateDebugReportCallbackEXT (VkInstance instance,
//     const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
//     const VkAllocationCallbacks* pAllocator,
//     VkDebugReportCallbackEXT* pCallback)
// {
// 	auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT> (
// 	    vkGetInstanceProcAddr (instance, "vkCreateDebugReportCallbackEXT"));
// 	if (func != nullptr)
// 	{
// 		return func (instance, pCreateInfo, pAllocator, pCallback);
// 	}
// 	else
// 	{
// 		return VK_ERROR_EXTENSION_NOT_PRESENT;
// 	}
// }

// void VulkanInstance::DestroyDebugReportCallbackEXT (
//     VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
// {
// 	auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT> (
// 	    vkGetInstanceProcAddr (instance, "vkDestroyDebugReportCallbackEXT"));
// 	if (func != nullptr)
// 	{
// 		func (instance, callback, pAllocator);
// 	}
// }

void VulkanInstance::SetupDebugCallback ()
{
	if (!validationLayersEnabled) return;

	//VkDebugReportCallbackCreateInfoEXT createInfo = {};
	//createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	//createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	//createInfo.pfnCallback = debugCallback;
//
	//VkResult res = CreateDebugReportCallbackEXT (instance, &createInfo, nullptr, &callback);
	//if (res != VK_SUCCESS)
	//{
	//	throw std::runtime_error ("failed to set up debug callback!");
	//}

	VkDebugUtilsMessengerCreateInfoEXT utils_createInfo = {};
	utils_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	utils_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	utils_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	utils_createInfo.pfnUserCallback = debugUtilsCallback;

	if (CreateDebugUtilsMessengerEXT(instance, &utils_createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    	throw std::runtime_error("failed to set up debug messenger!");
	}

}

VkBool32 VulkanInstance::debugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	Log.Debug (fmt::format ("Validation Layer:\n{}\n", pCallbackData->pMessage));

	return VK_FALSE;	
}

VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
///////// VulkaPhysicalDevice /////////

VulkanPhysicalDevice::VulkanPhysicalDevice (VkInstance instance, VkSurfaceKHR surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices (instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error ("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices (deviceCount);
	vkEnumeratePhysicalDevices (instance, &deviceCount, devices.data ());

	for (const auto& device : devices)
	{
		if (IsDeviceSuitable (device, surface))
		{
			physical_device = device;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE)
	{
		throw std::runtime_error ("failed to find a suitable GPU!");
	}
	familyIndices = FindQueueFamilies (physical_device, surface);
	vkGetPhysicalDeviceFeatures (physical_device, &physical_device_features);
	vkGetPhysicalDeviceProperties (physical_device, &physical_device_properties);
	vkGetPhysicalDeviceMemoryProperties (physical_device, &memoryProperties);
}

VulkanPhysicalDevice::~VulkanPhysicalDevice () {}

//////// VulkanDeivce ////////

VulkanDevice::VulkanDevice (bool validationLayers, Window& window)
: enableValidationLayers (validationLayers),
  instance ("My Vulkan App", enableValidationLayers, window),
  physical_device (instance.instance, instance.surface)

{

	CreateLogicalDevice ();

	CreateQueues ();

	CreateVulkanAllocator ();
}

VulkanDevice::~VulkanDevice ()
{
	allocator_general.Free ();
	allocator_linear_tiling.Free ();

	vkDestroyDevice (device, nullptr);
}

void VulkanDevice::LogMemory ()
{
	allocator_general.LogVMA ();
	allocator_linear_tiling.LogVMA ();
}

bool VulkanPhysicalDevice::IsDeviceSuitable (VkPhysicalDevice device, VkSurfaceKHR surface)
{

	QueueFamilyIndices indices = FindQueueFamilies (device, surface);

	bool extensionsSupported = CheckDeviceExtensionSupport (device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = VulkanSwapChain::querySwapChainSupport (device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty () && !swapChainSupport.present_modes.empty ();
	}

	vkGetPhysicalDeviceFeatures (device, &physical_device_features);

	return indices.isComplete () && extensionsSupported && physical_device_features.samplerAnisotropy;
}

bool VulkanPhysicalDevice::CheckDeviceExtensionSupport (VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions (extensionCount);
	vkEnumerateDeviceExtensionProperties (device, nullptr, &extensionCount, availableExtensions.data ());

	std::set<std::string> requiredExtensions (DEVICE_EXTENSIONS.begin (), DEVICE_EXTENSIONS.end ());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase (extension.extensionName);
	}

	return requiredExtensions.empty ();
}


// Put all device specific features here
VkPhysicalDeviceFeatures VulkanPhysicalDevice::QueryDeviceFeatures ()
{
	VkPhysicalDeviceFeatures deviceFeatures = {};
	if (physical_device_features.samplerAnisotropy) deviceFeatures.samplerAnisotropy = VK_TRUE;

	if (physical_device_features.fillModeNonSolid) deviceFeatures.fillModeNonSolid = VK_TRUE;

	if (physical_device_features.geometryShader) deviceFeatures.geometryShader = VK_TRUE;

	if (physical_device_features.tessellationShader) deviceFeatures.tessellationShader = VK_TRUE;

	if (physical_device_features.sampleRateShading) deviceFeatures.sampleRateShading = VK_TRUE;


	return deviceFeatures;
}

QueueFamilyIndices VulkanPhysicalDevice::FindQueueFamilies (VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties (physDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies (queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties (physDevice, &queueFamilyCount, queueFamilies.data ());

	// finds a transfer only queue
	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
		    ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
		    ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
		{

			indices.transferFamily = i;
		}
		i++;
	}

	// finds graphics, present and optionally a transfer and compute queue
	i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR (physDevice, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			indices.computeFamily = i;
		}

		if (indices.isComplete ())
		{
			break;
		}

		i++;
	}

	// make sure that they are set since they might get used
	if (indices.transferFamily == -1)
	{
		indices.transferFamily = 0;
	}
	if (indices.computeFamily == -1)
	{
		indices.computeFamily = 0;
	}
	// if (indices.presentFamily == -1) {
	//	indices.presentFamily = 0;
	//}
	return indices;
}

const QueueFamilyIndices VulkanDevice::GetFamilyIndices () const
{
	return physical_device.familyIndices;
}

void VulkanDevice::CreateLogicalDevice ()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { physical_device.familyIndices.graphicsFamily,
		physical_device.familyIndices.presentFamily,
		physical_device.familyIndices.computeFamily,
		physical_device.familyIndices.transferFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back (queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = physical_device.QueryDeviceFeatures ();

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t> (queueCreateInfos.size ());
	createInfo.pQueueCreateInfos = queueCreateInfos.data ();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t> (DEVICE_EXTENSIONS.size ());
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data ();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t> (DEVICE_EXTENSIONS.size ());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data ();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice (physical_device.physical_device, &createInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create logical device!");
	}
}

void VulkanDevice::CreateQueues ()
{
	graphics_queue = std::make_unique<CommandQueue> (*this, physical_device.familyIndices.graphicsFamily);
	singleQueueDevice = true;

	// Make sure it is a unique queue, else don't make anything (empty pointer means it isn't unique)
	if (physical_device.familyIndices.graphicsFamily != physical_device.familyIndices.computeFamily)
	{
		compute_queue = std::make_unique<CommandQueue> (*this, physical_device.familyIndices.computeFamily);
		singleQueueDevice = false;
	}

	if (physical_device.familyIndices.graphicsFamily != physical_device.familyIndices.transferFamily)
	{
		transfer_queue = std::make_unique<CommandQueue> (*this, physical_device.familyIndices.transferFamily);
		singleQueueDevice = false;
	}

	if (physical_device.familyIndices.graphicsFamily != physical_device.familyIndices.presentFamily)
	{
		present_queue = std::make_unique<CommandQueue> (*this, physical_device.familyIndices.presentFamily);
		singleQueueDevice = false;
	}
}

CommandQueue& VulkanDevice::GraphicsQueue () { return *graphics_queue; }
CommandQueue& VulkanDevice::ComputeQueue ()
{
	if (compute_queue) return *compute_queue;
	return GraphicsQueue ();
}
CommandQueue& VulkanDevice::TransferQueue ()
{
	if (transfer_queue) return *transfer_queue;
	return GraphicsQueue ();
}
CommandQueue& VulkanDevice::PresentQueue ()
{
	if (present_queue)
	{
		return *present_queue;
	}
	return GraphicsQueue ();
}

void VulkanDevice::CreateVulkanAllocator ()
{
	allocator_general.Create (physical_device.physical_device, device);
	allocator_linear_tiling.Create (physical_device.physical_device, device);
}


VmaAllocator VulkanDevice::GetGeneralAllocator () { return allocator_general.allocator; }

VmaAllocator VulkanDevice::GetImageLinearAllocator () { return allocator_linear_tiling.allocator; }

VmaAllocator VulkanDevice::GetImageOptimalAllocator () { return allocator_general.allocator; }

VkSurfaceKHR VulkanDevice::GetSurface () { return instance.surface; }
