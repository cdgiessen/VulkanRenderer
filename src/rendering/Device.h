#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <cstring>

#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>

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
	VulkanInstance (std::string appName, bool validationLayersEnabled);
	~VulkanInstance ();

	VkInstance instance;

	private:
	bool validationLayersEnabled = false;

	bool CheckValidationLayerSupport ();
	void SetupDebugCallback ();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT messageType,
	    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	    void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT (VkInstance instance,
	    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	    const VkAllocationCallbacks* pAllocator,
	    VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT (
	    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
};

struct VulkanSurface
{
	VulkanSurface (VulkanInstance const& instance, Window& window);
	~VulkanSurface ();

	VkSurfaceKHR surface;
	VulkanInstance const& instance;
	Window& window;
};

struct VulkanPhysicalDevice
{
	VulkanPhysicalDevice (VulkanInstance const& instance, VulkanSurface const& surface);
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
	private:
	bool enableValidationLayers = false;

	public:
	VulkanInstance instance;
	VulkanSurface surface;
	VulkanPhysicalDevice physical_device;

	VkDevice device;


	bool singleQueueDevice; // for devices with only 1 queue (intel integrated
	                        // specifically)
	VulkanDevice (Window& window, bool validationLayers = false);

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

	VMA_MemoryResource allocator_general;
	VMA_MemoryResource allocator_linear_tiling;
	VMA_MemoryResource allocator_optimal_tiling;

	void CreateLogicalDevice ();
	void CreateQueues ();

	void CreateVulkanAllocator ();
};
