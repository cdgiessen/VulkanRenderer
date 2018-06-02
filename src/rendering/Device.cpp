#include "Device.h"

#include "Initializers.h"
#include "SwapChain.h"
//#include "Buffer.h"

#include "../core/Logger.h"

VulkanFence::VulkanFence(const VulkanDevice& device, 
	long int timeout, 
	VkFenceCreateFlags flags)  		:device(device), timeout(timeout) {
		VkFenceCreateInfo fenceInfo =
			initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

	}
VulkanFence::~VulkanFence()
{
	vkDestroyFence(device.device, fence, nullptr);
}

void VulkanFence::WaitTillTrue() {
	vkWaitForFences(device.device, 1, &fence, VK_TRUE, timeout);
}
void VulkanFence::WaitTillFalse() {
	vkWaitForFences(device.device, 1, &fence, VK_FALSE, timeout);
}
VkFence VulkanFence::GetFence(){
	return fence;
}

//void CleanUp() {
//	vkDestroyFence(device.device, fence, nullptr);	//}	VkFence VulkanFence::GetFence() {return fence;};

CommandQueue::CommandQueue(const VulkanDevice& device, int queueFamily) :
	device(device)
{
	vkGetDeviceQueue(device.device, queueFamily, 0, &queue);
	this->queueFamily = queueFamily;
	//Log::Debug << "Queue on " << queueFamily << " type\n";
}

//void CommandQueue::SetupQueue(int queueFamily) {
//	vkGetDeviceQueue(device.device, queueFamily, 0, &queue);
//	this->queueFamily = queueFamily;
//}

void CommandQueue::SubmitCommandBuffer(VkCommandBuffer buffer, VkFence fence,
	std::vector<VkSemaphore> waitSemaphores,
	std::vector<VkSemaphore> signalSemaphores)
{
	const auto stageMasks = std::vector<VkPipelineStageFlags>{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	auto submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;
	submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();
	submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = stageMasks.data();
	Submit(submitInfo, fence);
}

void CommandQueue::Submit(VkSubmitInfo submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(submissionMutex);
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
}

int CommandQueue::GetQueueFamily() {
	return queueFamily;
}

VkQueue CommandQueue::GetQueue() {
	return queue;
}

void CommandQueue::WaitForFences(VkFence fence) {
	vkWaitForFences(device.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
}

CommandPool::CommandPool(VulkanDevice& device) :
	device(device)
{

}

VkBool32 CommandPool::Setup(VkCommandPoolCreateFlags flags, CommandQueue* queue)
{
	this->queue = queue;

	VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo();
	cmd_pool_info.queueFamilyIndex = queue->GetQueueFamily();
	cmd_pool_info.flags = flags;

	if (vkCreateCommandPool(device.device, &cmd_pool_info, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}

	return VK_TRUE;
}

VkBool32 CommandPool::CleanUp() {
	std::lock_guard<std::mutex> lock(poolLock);
	vkDestroyCommandPool(device.device, commandPool, nullptr);
	return VK_TRUE;
}

VkBool32 CommandPool::ResetPool() {

	std::lock_guard<std::mutex> lock(poolLock);
	vkResetCommandPool(device.device, commandPool, 0);
	return VK_TRUE;
}

VkCommandBuffer CommandPool::AllocateCommandBuffer(VkCommandBufferLevel level)
{
	VkCommandBuffer buf;

	VkCommandBufferAllocateInfo allocInfo =
		initializers::commandBufferAllocateInfo(commandPool, level, 1);

	std::lock_guard<std::mutex> lock(poolLock);
	vkAllocateCommandBuffers(device.device, &allocInfo, &buf);

	return buf;
}

void CommandPool::BeginBufferRecording(VkCommandBuffer buf, VkCommandBufferUsageFlagBits flags) {

	VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
	beginInfo.flags = flags;

	vkBeginCommandBuffer(buf, &beginInfo);
}

void CommandPool::EndBufferRecording(VkCommandBuffer buf) {
	vkEndCommandBuffer(buf);
}

void CommandPool::FreeCommandBuffer(VkCommandBuffer buf) {
	{
		std::lock_guard<std::mutex> lock(poolLock);
		vkFreeCommandBuffers(device.device, commandPool, 1, &buf);
	}
}

VkCommandBuffer CommandPool::GetOneTimeUseCommandBuffer() {
	VkCommandBuffer cmdBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	BeginBufferRecording(cmdBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	return cmdBuffer;
}

VkCommandBuffer CommandPool::GetPrimaryCommandBuffer(bool beginBufferRecording) {

	VkCommandBuffer cmdBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	if (beginBufferRecording == true)
		BeginBufferRecording(cmdBuffer);

	return cmdBuffer;
}

VkCommandBuffer CommandPool::GetSecondaryCommandBuffer(bool beginBufferRecording) {
	VkCommandBuffer cmdBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	if (beginBufferRecording == true)
		BeginBufferRecording(cmdBuffer);

	return cmdBuffer;
}

VkBool32 CommandPool::SubmitOneTimeUseCommandBuffer(VkCommandBuffer cmdBuffer, VkFence fence) {
	EndBufferRecording(cmdBuffer);

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	if (fence == nullptr) {
		VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))
	}

	queue->SubmitCommandBuffer(cmdBuffer, fence);

	return VK_TRUE;
}

VkBool32 CommandPool::SubmitPrimaryCommandBuffer(VkCommandBuffer cmdBuffer, VkFence fence) {
	EndBufferRecording(cmdBuffer);

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	if (fence == nullptr) {
		VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))
	}

	queue->SubmitCommandBuffer(cmdBuffer, fence);


	return VK_TRUE;
}

//VkBool32 CommandPool::SubmitSecondaryCommandBuffer(VkCommandBuffer cmdBuffer, VkFence fence){
//	EndBufferRecording(cmdBuffer);
//
//	VkSubmitInfo submitInfo = initializers::submitInfo();
//	submitInfo.commandBufferCount = 1;
//	submitInfo.pCommandBuffers = &cmdBuffer;
//
//	if (fence == nullptr) {
//		VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
//		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))
//	}
//
//	queue.SubmitCommandBuffer(cmdBuffer, fence);
//
//
//	return VK_TRUE;	
//}



// TransferQueue::TransferQueue(VulkanDevice& device, CommandQueue& transferQueue) :
// 	device(device),
// 	transferQueue(transferQueue),
// 	transferCommandPool(device, transferQueue, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
// {
// }

// void TransferQueue::CleanUp() {
// 	transferCommandPool.CleanUp();
// }

// CommandPool& TransferQueue::GetCommandPool() {
// 	return transferCommandPool;
// }



// VkCommandBuffer TransferQueue::GetTransferCommandBuffer() {

// 	return transferCommandPool.GetOneTimeUseCommandBuffer();
// }

// void WaitForSubmissionFinish(VkDevice device, TransferQueue* queue, 
// 	VkCommandBuffer buf, VkFence fence, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean) {

// 	vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
// 	if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
// 		for (auto& sig : readySignal)
// 			*sig = true;		
// 	}
// 	else if (vkGetFenceStatus(device, fence) == VK_NOT_READY) {
// 		Log::Error << "Transfer exeeded maximum fence timeout! Is too much stuff happening?\n";
// 		vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
// 		if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
// 			for (auto& sig : readySignal)
// 				*sig = true;
// 		}
// 	}
// 	else if (vkGetFenceStatus(device, fence) == VK_ERROR_DEVICE_LOST){
// 		Log::Error << "AAAAAAAAAAAHHHHHHHHHHHHH EVERYTHING IS ONE FIRE\n";
// 		throw std::runtime_error("Fence lost device!\n");
// 	}


// 	vkDestroyFence(device, fence, nullptr);

// 	queue->GetCommandPool().FreeCommandBuffer(buf);

// 	for (auto& buffer : bufsToClean) {
// 		buffer.CleanBuffer();
// 	}
// }

// void TransferQueue::SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean) {

// 	VkFence fence;
// 	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
// 	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

// 	transferCommandPool.SubmitOneTimeUseCommandBuffer(buf, fence);

// 	std::thread submissionCompletion = std::thread(WaitForSubmissionFinish, device.device, this, buf, fence, readySignal, bufsToClean);
// 	submissionCompletion.detach();
// }

// void TransferQueue::SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal) {
// 	SubmitTransferCommandBuffer(buf, readySignal, {});
// }

// void TransferQueue::SubmitTransferCommandBufferAndWait(VkCommandBuffer buf) {

// 	VkFence fence;
// 	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
// 	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

// 	transferCommandPool.SubmitOneTimeUseCommandBuffer(buf, fence);

// 	vkWaitForFences(device.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
// 	vkDestroyFence(device.device, fence, nullptr);

// 	transferCommandPool.FreeCommandBuffer(buf);
// }



//CommandBufferWorker::CommandBufferWorker(VulkanDevice& device, 
//	CommandQueue queue, ConcurrentQueue<CommandBufferWorker>& workQueue, 
//	bool startActive)
//:pool(device, queue), workQueue(workQueue)
//{
//	
//	workingThread = std::thread(this->Work);
//	
//}
//
//CommandBufferWorker::~CommandBufferWorker(){
//	workingThread.join();
//}
//
//void CommandBufferWorker::Work(){
//
//	while(keepWorking){
//		auto pos_work = workQueue.pop_if();
//		if(pos_work.has_value()){
//			VkCommandBuffer buf = pool.GetOneTimeUseCommandBuffer();
//	
//			pos_work->get().work(buf);
//	
//			pool.SubmitCommandBuffer(buf);
//	
//			/*Do I wait for work to finish or start new work?
//			Cause I want to get as much going on as possible.
//			Or should I batch a bunch of work together by waiting for submission
//			also, where to put fences/semaphores? 
//			Should I create a buffer object that holds those resources,
//			since I *should* be recycling buffers instead of recreating them.
//			the joys of a threaded renderer.*/
//		} 
//
//
//	}
//
//}

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
	createInstance("My Vulkan App");
	setupDebugCallback();
	createSurface(surface);
	pickPhysicalDevice(surface);
	FindQueueFamilies(surface);

	CreateLogicalDevice();

	if (familyIndices.graphicsFamily != familyIndices.transferFamily) {
		//transferQueue = std::make_unique<TransferQueue>(*this, transfer_queue);
		separateTransferQueue = true;
	}
	else
		separateTransferQueue = false;

	//when the hardware doesn't have a separate transfer queue ( can't do asynchronous submissions)

	CreateQueues();
	//CreateCommandPools();
	CreateVulkanAllocator();
}

void VulkanDevice::Cleanup(VkSurfaceKHR &surface) {
	vmaDestroyAllocator(allocator);
	vmaDestroyAllocator(linear_allocator);
	vmaDestroyAllocator(optimal_allocator);
	//vkDestroyCommandPool(device, graphics_queue_command_pool, nullptr);
	//vkDestroyCommandPool(device, compute_queue_command_pool, nullptr);
	//if (!separateTransferQueue)
	//	vkDestroyCommandPool(device, transfer_queue_command_pool, nullptr);
	//else
	//transferQueue->CleanUp();


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
	
	//Make sure it is a unique queue, else don't make anything (empty pointer means it isn't unique)
	if (familyIndices.graphicsFamily != familyIndices.computeFamily)
		compute_queue = std::make_unique<CommandQueue>(*this, familyIndices.computeFamily);

	if (familyIndices.graphicsFamily != familyIndices.transferFamily)
		transfer_queue = std::make_unique<CommandQueue>(*this, familyIndices.transferFamily);
	
	if (familyIndices.graphicsFamily != familyIndices.presentFamily)
		present_queue = std::make_unique<CommandQueue>(*this, familyIndices.presentFamily);

	
	//vkGetDeviceQueue(device, familyIndices.graphicsFamily, 0, &graphics_queue);
	//vkGetDeviceQueue(device, familyIndices.presentFamily, 0, &present_queue);
	//vkGetDeviceQueue(device, familyIndices.computeFamily, 0, &compute_queue);
	//
	//if (!separateTransferQueue)
	//	vkGetDeviceQueue(device, familyIndices.transferFamily, 0, &transfer_queue);
}

CommandQueue& VulkanDevice::GetCommandQueue(CommandQueueType queueType) {
	switch (queueType)
	{
	case VulkanDevice::CommandQueueType::graphics:
		return GraphicsQueue();

	case VulkanDevice::CommandQueueType::compute:
		return ComputeQueue();

	case VulkanDevice::CommandQueueType::transfer:
		return TransferQueue();

	case VulkanDevice::CommandQueueType::present:
	 	return PresentQueue();

	default:
		break;
	}
	return GraphicsQueue();
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
	if (transfer_queue)
		return *present_queue;
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
