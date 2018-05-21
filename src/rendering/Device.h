#pragma once

#include <vector>
#include <memory>
#include <set>
#include <thread>
#include <mutex>
#include <functional>

#include <vulkan/vulkan.h>
//#include <vulkan/vulkan.hpp>

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

class CommandQueue {
public:
	CommandQueue(VulkanDevice& device);
	void SetupQueue(int queueFamily);

	void Submit(VkSubmitInfo info, VkFence fence);
	void SubmitCommandBuffer(VkCommandBuffer buf, VkFence fence,
		std::vector<VkSemaphore> waitSemaphores = std::vector<VkSemaphore>(), 
		std::vector<VkSemaphore> signalSemaphores = std::vector<VkSemaphore>());

	int GetQueueFamily();
	VkQueue GetQueue();

	void WaitForFences(VkFence fence);
	
private:
	VulkanDevice& device;
	std::mutex submissionMutex;
	VkQueue queue;
	int queueFamily;
};

class CommandPool {
public:
	CommandPool(VulkanDevice& device, CommandQueue& queue);

	VkBool32 Setup(VkCommandPoolCreateFlags flags);
	VkBool32 CleanUp();

	VkBool32 ResetPool();

	VkCommandBuffer GetOneTimeUseCommandBuffer();
	VkCommandBuffer GetPrimaryCommandBuffer(bool beginBufferRecording = true);
	VkCommandBuffer GetSecondaryCommandBuffer(bool beginBufferRecording = true);

	VkBool32 SubmitOneTimeUseCommandBuffer(VkCommandBuffer cmdBuffer, VkFence fence = nullptr);
	VkBool32 SubmitPrimaryCommandBuffer(VkCommandBuffer buf, VkFence fence = nullptr);
	VkBool32 SubmitSecondaryCommandBuffer(VkCommandBuffer buf, VkFence fence = nullptr);

	VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel level);
	void BeginBufferRecording(VkCommandBuffer buf, VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)(0));
	void EndBufferRecording(VkCommandBuffer buf);
	void FreeCommandBuffer(VkCommandBuffer buf);

private:
	VulkanDevice& device;
	std::mutex poolLock;
	VkCommandPool commandPool;
	CommandQueue& queue;

	//std::vector<VkCommandBuffer> cmdBuffers;
};


// class NotTransferQueue {
// public:
// 	NotTransferQueue(VulkanDevice& device, CommandQueue& transferQueue);
// 	void CleanUp();

// 	CommandPool& GetCommandPool();
// 	//std::mutex& GetTransferMutex();

// 	VkCommandBuffer GetTransferCommandBuffer();
// 	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal);
// 	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean);
// 	void SubmitTransferCommandBufferAndWait(VkCommandBuffer buf);

// private:
// 	VulkanDevice& device;
// 	CommandQueue& transferQueue;
// 	//VkQueue transfer_queue;
// 	CommandPool transferCommandPool;
// 	//VkCommandPool transfer_queue_command_pool;
// 	//std::mutex transferMutex;
// };

struct CommandBufferWork {
	std::function<void(VkCommandBuffer)> work; //function to run (preferably a lambda) 
	std::vector<Signal> flags;
}

class CommandBufferWorker {
public:
	CommandBufferWorker(VulkanDevice& device, 
		CommandQueue queue, bool startActive = true)
	~CommandBufferWorker();
	AddWork(CommandBufferWork);


private:
	std::thread workingThread;
	void Work();

	bool keepWorking = true; //default to start working
	ConcurrentQueue<T> workQueue;
	std::condition_variable waitVar;
	CommandPool pool;
}

class VulkanDevice {
public:
	GLFWwindow* window;

	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	VkDevice device;
	
	//VDeleter<VkSurfaceKHR> window_surface{ instance, vkDestroySurfaceKHR };

	VkPhysicalDevice physical_device;

	//CommandQueue graphics_queue;
	//CommandQueue compute_queue;
	//CommandQueue present_queue;
	//
	//CommandPool graphics_command_pool;

	//std::mutex graphics_lock;
	//std::mutex graphics_command_pool_lock;

	enum class CommandQueueType {
		graphics,
		compute,
		transfer,
		present
	};

	CommandQueue graphics_queue;
	CommandQueue compute_queue;
	CommandQueue transfer_queue;
	CommandQueue present_queue;

	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceFeatures physical_device_features;
	VkPhysicalDeviceMemoryProperties memoryProperties;

	VmaAllocator allocator;

	//VkMemoryPropertyFlags uniformBufferMemPropertyFlags = 
	//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VulkanDevice(bool validationLayers);

	~VulkanDevice();

	void InitVulkanDevice(VkSurfaceKHR &surface);

	void Cleanup(VkSurfaceKHR &surface);

	const QueueFamilyIndices GetFamilyIndices() const;

	CommandQueue& GetCommandQueue(CommandQueueType queueType);

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
	
	void DestroyVmaAllocatedBuffer(VkBuffer* buffer, VmaAllocation* allocation);
	void DestroyVmaAllocatedBuffer(VmaBuffer& buffer);

	void CreateImage2D(VkImageCreateInfo imageInfo, VmaImage& image);
	void CreateDepthImage(VkImageCreateInfo imageInfo, VmaImage& image);
	void CreateStagingImage2D(VkImageCreateInfo imageInfo, VmaImage& image);

	void DestroyVmaAllocatedImage(VmaImage& image);

	//VkCommandBuffer GetGraphicsCommandBuffer();
	//VkCommandBuffer GetSingleUseGraphicsCommandBuffer();
	//void SubmitGraphicsCommandBufferAndWait(VkCommandBuffer buffer);

	//VkCommandBuffer GetTransferCommandBuffer();

	//void SubmitTransferCommandBufferAndWait(VkCommandBuffer buf);
	//void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal);
	//void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean);


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

	bool separateTransferQueue = false;
private:
	QueueFamilyIndices familyIndices;

	bool enableValidationLayers = false;



	//std::unique_ptr<TransferQueue> transferQueue;

	void createInstance(std::string appName);

	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	bool checkValidationLayerSupport();
	
	void setupDebugCallback();
	
	void createSurface(VkSurfaceKHR &surface);
	
	void pickPhysicalDevice(VkSurfaceKHR &surface);

	void CreateLogicalDevice();
	void CreateQueues();
	//void CreateCommandPools();

	void CreateVulkanAllocator();

	void FindQueueFamilies(VkSurfaceKHR windowSurface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physDevice, VkSurfaceKHR windowSurface);

	VkPhysicalDeviceFeatures QueryDeviceFeatures();
	
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
};
