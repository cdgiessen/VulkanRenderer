#pragma once

#include <vector>
#include <memory>
#include <set>
#include <thread>
#include <mutex>
#include <functional>

#include <vulkan/vulkan.h>

#include "../util/ConcurrentQueue.h"

class Window;

#include "RenderTools.h"
#include "RenderStructs.h"
#include "Wrappers.h"


const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"

};

const std::vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;
	int transferFamily = -1;
	int computeFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};

class VulkanDevice {
public:
	Window* window;

	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	VkDevice device;

	VkPhysicalDevice physical_device;

	bool singleQueueDevice; //for devices with only 1 queue (intel integrated specifically)
	enum class CommandQueueType {
		graphics,
		compute,
		transfer,
		present
	};

	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceFeatures physical_device_features;
	VkPhysicalDeviceMemoryProperties memoryProperties;

	VulkanDevice(bool validationLayers, Window* window);

	~VulkanDevice();

	//void InitVulkanDevice();

	void CleanUp();

	const QueueFamilyIndices GetFamilyIndices() const;

	CommandQueue& GraphicsQueue();
	CommandQueue& ComputeQueue();
	CommandQueue& TransferQueue();
	CommandQueue& PresentQueue();

	VmaAllocator GetGeneralAllocator();
	VmaAllocator GetImageLinearAllocator();
	VmaAllocator GetImageOptimalAllocator();

	VkSurfaceKHR GetSurface();

private:
	QueueFamilyIndices familyIndices;

	std::unique_ptr<CommandQueue> graphics_queue;
	std::unique_ptr<CommandQueue> compute_queue;
	std::unique_ptr<CommandQueue> transfer_queue;
	std::unique_ptr<CommandQueue> present_queue;

	bool enableValidationLayers = false;

	VmaAllocator allocator;
	VmaAllocator linear_allocator;
	VmaAllocator optimal_allocator;

	VkSurfaceKHR surface;

	void CreateInstance(std::string appName);

	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	bool CheckValidationLayerSupport();

	void SetupDebugCallback();

	void CreateSurface(VkSurfaceKHR &surface);

	void PickPhysicalDevice(VkSurfaceKHR &surface);

	void CreateLogicalDevice();
	void CreateQueues();

	void CreateVulkanAllocator();

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physDevice, VkSurfaceKHR windowSurface);

	VkPhysicalDeviceFeatures QueryDeviceFeatures();

	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
};
