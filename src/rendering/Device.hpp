#pragma once

#include <vector>
#include <memory>
#include <set>
#include <thread>
#include <mutex>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

#include "RenderTools.h"
#include "RenderStructs.h"

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

struct VmaBuffer {
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo;
};

struct VmaImage {
	VkImage image = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo;
};

class VulkanDevice;
class VulkanBuffer;

class TransferQueue {
public:
	TransferQueue(VulkanDevice& device, uint32_t transferFamily);
	void CleanUp();

	VkCommandBuffer GetTransferCommandBuffer();
	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal);
	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean);
	void SubmitTransferCommandBufferAndWait(VkCommandBuffer buf);

private:
	VulkanDevice& device;
	VkQueue transfer_queue;
	VkCommandPool transfer_queue_command_pool;
	std::mutex transferLock;
};

class VulkanDevice {
public:
	GLFWwindow* window;

	VkDevice device;
	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	
	//VDeleter<VkSurfaceKHR> window_surface{ instance, vkDestroySurfaceKHR };

	VkPhysicalDevice physical_device;
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue compute_queue;

	VkCommandPool graphics_queue_command_pool;
	VkCommandPool compute_queue_command_pool;

	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceFeatures physical_device_features;
	VkPhysicalDeviceMemoryProperties memoryProperties;

	VmaAllocator allocator;

	VkMemoryPropertyFlags uniformBufferMemPropertyFlags = 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VulkanDevice(bool validationLayers);

	~VulkanDevice();

	void InitVulkanDevice(VkSurfaceKHR &surface);

	void Cleanup(VkSurfaceKHR &surface);

	const QueueFamilyIndices GetFamilyIndices() const;

	void VmaMapMemory(VmaBuffer& buffer, void** pData);
	void VmaUnmapMemory(VmaBuffer& buffer);

	void CreateUniformBuffer(VmaBuffer& buffer, VkDeviceSize bufferSize); 
	void CreateUniformBufferMapped(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateStagingUniformBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize);

	void CreateDynamicUniformBuffer(VmaBuffer& buffer, uint32_t count, VkDeviceSize sizeOfData);

	void CreateMeshBufferVertex(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateMeshBufferIndex(VmaBuffer& buffer, VkDeviceSize bufferSize);
	void CreateMeshStagingBuffer(VmaBuffer& buffer, void* data, VkDeviceSize bufferSize);

	void DestroyVmaAllocatedBuffer(VkBuffer* buffer, VmaAllocation* allocation);
	void DestroyVmaAllocatedBuffer(VmaBuffer& buffer);

	void CreateImage2D(VkImageCreateInfo imageInfo, VmaImage& image);
	void CreateDepthImage(VkImageCreateInfo imageInfo, VmaImage& image);
	void CreateStagingImage2D(VkImageCreateInfo imageInfo, VmaImage& image);

	void DestroyVmaAllocatedImage(VmaImage& image);

	VkCommandBuffer GetTransferCommandBuffer();

	void SubmitTransferCommandBufferAndWait(VkCommandBuffer buf);
	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal);
	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean);

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
	//VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data = nullptr);

	/**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param buffer Pointer to a vk::Vulkan buffer object
	* @param size Size of the buffer in byes
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/
	//VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VulkanBuffer *buffer, VkDeviceSize size, void *data = nullptr);

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
	uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr);


	/**
	* Allocate a command buffer from the command pool
	*
	* @param level Level of the new command buffer (primary or secondary)
	* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
	*
	* @return A handle to the allocated command buffer
	*/
	VkCommandBuffer createCommandBuffer(VkCommandPool commandPool, VkCommandBufferLevel level, bool begin = false);

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
	void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
	void flushCommandBuffer(VkCommandPool pool, VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

private:
	QueueFamilyIndices familyIndices;

	bool enableValidationLayers = false;

	bool separateTransferQueue = false;

	//Non separate transfer queue (no threads where they don't help)
	bool isDmaCmdBufWritable = false;
	VkQueue transfer_queue;
	VkCommandPool transfer_queue_command_pool;
	VkCommandBuffer dmaCmdBuf;

	std::unique_ptr<TransferQueue> transferQueue;

	void createInstance(std::string appName);

	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	bool checkValidationLayerSupport();
	
	void setupDebugCallback();
	
	void createSurface(VkSurfaceKHR &surface);
	
	void pickPhysicalDevice(VkSurfaceKHR &surface);

	void CreateLogicalDevice();
	void CreateQueues();
	void CreateCommandPools();

	void CreateVulkanAllocator();

	void FindQueueFamilies(VkSurfaceKHR windowSurface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physDevice, VkSurfaceKHR windowSurface);

	VkPhysicalDeviceFeatures QueryDeviceFeatures();
	
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
};
