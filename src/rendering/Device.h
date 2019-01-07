#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <cstring>

#include <vulkan/vulkan.h>

#include "util/ConcurrentQueue.h"



#include "RenderStructs.h"
#include "RenderTools.h"
#include "Wrappers.h"

class Window;

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;
	int transferFamily = -1;
	int computeFamily = -1;

	bool isComplete () { return graphicsFamily >= 0 && presentFamily >= 0; }
};

struct VMA_MemoryResource
{
	public:
	void Create (VkPhysicalDevice physical_device, VkDevice device, VkAllocationCallbacks* custom_allocator = nullptr);

	void Free ();

	void LogVMA (bool detailedOutput = false);

	VmaAllocator allocator;
};

class VulkanInstance
{
	public:

	VulkanInstance (std::string appName, bool validationLayersEnabled, Window& window);
	~VulkanInstance ();
	
	VkInstance instance;
	VkSurfaceKHR surface;
	Window& window;

	private:
	bool validationLayersEnabled = false;

	bool CheckValidationLayerSupport ();
	void SetupDebugCallback ();
	void CreateSurface ();


	VkResult CreateDebugReportCallbackEXT (VkInstance instance,
	    const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	    const VkAllocationCallbacks* pAllocator,
	    VkDebugReportCallbackEXT* pCallback);

	void DestroyDebugReportCallbackEXT (
	    VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (VkDebugReportFlagsEXT flags,
	    VkDebugReportObjectTypeEXT objType,
	    uint64_t obj,
	    size_t location,
	    int32_t code,
	    const char* layerPrefix,
	    const char* msg,
	    void* userData);

	VkDebugReportCallbackEXT callback;
};

class VulkanPhysicalDevice
{
	public:
	VulkanPhysicalDevice (VkInstance instance, VkSurfaceKHR surface);
	~VulkanPhysicalDevice ();
	bool IsDeviceSuitable (VkPhysicalDevice device, VkSurfaceKHR surface);

	bool CheckDeviceExtensionSupport (VkPhysicalDevice device);

	QueueFamilyIndices FindQueueFamilies (VkPhysicalDevice physDevice, VkSurfaceKHR windowSurface);

	VkPhysicalDeviceFeatures QueryDeviceFeatures ();

		VkPhysicalDevice physical_device;

	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceFeatures physical_device_features;
	VkPhysicalDeviceMemoryProperties memoryProperties;


	QueueFamilyIndices familyIndices;
};

class VulkanDevice
{
	public:

	VulkanInstance instance;
	VulkanPhysicalDevice physical_device;

	VkDevice device;


	bool singleQueueDevice; // for devices with only 1 queue (intel integrated
	                        // specifically)
	enum class CommandQueueType
	{
		graphics,
		compute,
		transfer,
		present
	};

	VulkanDevice (bool validationLayers, Window& window);

	~VulkanDevice ();

	void LogMemory ();

	const QueueFamilyIndices GetFamilyIndices () const;

	CommandQueue& GraphicsQueue ();
	CommandQueue& ComputeQueue ();
	CommandQueue& TransferQueue ();
	CommandQueue& PresentQueue ();

	VmaAllocator GetGeneralAllocator ();
	VmaAllocator GetImageLinearAllocator ();
	VmaAllocator GetImageOptimalAllocator ();

	VkSurfaceKHR GetSurface ();

	private:
	std::unique_ptr<CommandQueue> graphics_queue;
	std::unique_ptr<CommandQueue> compute_queue;
	std::unique_ptr<CommandQueue> transfer_queue;
	std::unique_ptr<CommandQueue> present_queue;

	bool enableValidationLayers = false;

	VMA_MemoryResource allocator_general;
	VMA_MemoryResource allocator_linear_tiling;

	void CreateLogicalDevice ();
	void CreateQueues ();

	void CreateVulkanAllocator ();

};
