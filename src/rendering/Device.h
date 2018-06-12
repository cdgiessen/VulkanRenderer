#pragma once

#include <vector>
#include <memory>
#include <set>
#include <thread>
#include <mutex>
#include <functional>

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

#include "../util/ConcurrentQueue.h"

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
	GLFWwindow * window;

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

	VmaAllocator allocator;
	VmaAllocator linear_allocator;
	VmaAllocator optimal_allocator;

	VulkanDevice(bool validationLayers);

	~VulkanDevice();

	void InitVulkanDevice(VkSurfaceKHR &surface);

	void Cleanup(VkSurfaceKHR &surface);

	const QueueFamilyIndices GetFamilyIndices() const;

	CommandQueue& GraphicsQueue();
	CommandQueue& ComputeQueue();
	CommandQueue& TransferQueue();
	CommandQueue& PresentQueue();

	void VmaMapMemory(VmaBuffer& buffer, void** pData);
	void VmaUnmapMemory(VmaBuffer& buffer);

	void CreateUniformBuffer(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateUniformBufferMapped(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateStagingUniformBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize);

	void CreateDynamicUniformBuffer(VmaBuffer& buffer, uint32_t count, VkDeviceSize sizeOfData);

	void CreateMeshBufferVertex(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateMeshBufferIndex(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateMeshStagingBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize);

	void CreateInstancingBuffer(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateStagingInstancingBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize);

	//void DestroyVmaAllocatedBuffer(VkBuffer* buffer, VmaAllocation* allocation);
	void DestroyVmaAllocatedBuffer(VmaBuffer& buffer);

	void CreateImage2D(VkImageCreateInfo imageInfo, VmaImage& image);
	void CreateDepthImage(VkImageCreateInfo imageInfo, VmaImage& image);
	void CreateStagingImage2D(VkImageCreateInfo imageInfo, VmaImage& image);
	void CreateStagingImageBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize);

	void DestroyVmaAllocatedImage(VmaImage& image);

private:
	QueueFamilyIndices familyIndices;

	std::unique_ptr<CommandQueue> graphics_queue;
	std::unique_ptr<CommandQueue> compute_queue;
	std::unique_ptr<CommandQueue> transfer_queue;
	std::unique_ptr<CommandQueue> present_queue;

	bool enableValidationLayers = false;

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

	void FindQueueFamilies(VkSurfaceKHR windowSurface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physDevice, VkSurfaceKHR windowSurface);

	VkPhysicalDeviceFeatures QueryDeviceFeatures();

	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
};
