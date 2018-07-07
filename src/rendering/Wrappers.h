#pragma once

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <set>
#include <utility>  

#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <vulkan/vulkan.h>

#include "RenderStructs.h"
#include "RenderTools.h"
#include "Buffer.h"

#include "../util/ConcurrentQueue.h"


class VulkanDevice;

class VulkanFence {
public:
	VulkanFence(VulkanDevice& device);

	void Create(long int timeout = DEFAULT_FENCE_TIMEOUT,
		VkFenceCreateFlags flags = VK_FLAGS_NONE);
	void CleanUp();

	bool Check();

	void WaitTillTrue();
	void WaitTillFalse();
	//void CleanUp();
	VkFence Get();

	void Reset();


private:
	VkDevice device;
	VkFence fence;
	int timeout;
};

std::vector<VkFence> CreateFenceArray(std::vector<VulkanFence>& fences);

class VulkanSemaphore {
public:
	VulkanSemaphore(VulkanDevice& device);

	VkSemaphore Get();
	VkSemaphore* GetPtr();

	void CleanUp();

private:
	VulkanDevice * device;
	VkSemaphore semaphore;
};

std::vector<VkSemaphore> CreateSemaphoreArray(std::vector<VulkanSemaphore>& sems);

class CommandQueue {
public:
	CommandQueue(const VulkanDevice&  device, int queueFamily);

	//No copy or move construction (so that it won't get duplicated)
	CommandQueue(const CommandQueue& cmd) = delete;
	CommandQueue& operator=(const CommandQueue& cmd) = delete;
	CommandQueue(CommandQueue&& cmd) = delete;
	CommandQueue& operator=(CommandQueue&& cmd) = delete;

	void Submit(VkSubmitInfo info, VkFence fence);

	void SubmitCommandBuffer(VkCommandBuffer buf, VkFence fence);

	void SubmitCommandBuffer(VkCommandBuffer buffer,
		VulkanFence& fence,
		std::vector<VulkanSemaphore>& waitSemaphores,
		std::vector<VulkanSemaphore>& signalSemaphores);

	int GetQueueFamily();
	std::mutex& GetQueueMutex();
	VkQueue GetQueue();

	void WaitForFences(VkFence fence);

private:
	const VulkanDevice & device;
	std::mutex submissionMutex;
	VkQueue queue;
	int queueFamily;
};


class CommandPool {
public:
	CommandPool(VulkanDevice& device,
		VkCommandPoolCreateFlags flags, CommandQueue* queue);

	//VkBool32 Setup(VkCommandPoolCreateFlags flags, CommandQueue* queue);
	VkBool32 CleanUp();

	VkBool32 ResetPool();
	VkBool32 ResetCommandBuffer(VkCommandBuffer cmdBuf);

	VkCommandBuffer GetOneTimeUseCommandBuffer();
	VkCommandBuffer GetPrimaryCommandBuffer(bool beginBufferRecording = true);
	VkCommandBuffer GetSecondaryCommandBuffer(bool beginBufferRecording = true);

	//VkBool32 SubmitCommandBuffer(VkCommandBuffer, VulkanFence& fence,
	//	std::vector<VkSemaphore>& waitSemaphores, std::vector<VkSemaphore>& signalSemaphores);
	VkBool32 SubmitCommandBuffer(VkCommandBuffer, VulkanFence& fence,
		std::vector<VulkanSemaphore>& waitSemaphores, std::vector<VulkanSemaphore>& signalSemaphores);

	VkBool32 SubmitOneTimeUseCommandBuffer(VkCommandBuffer cmdBuffer, VkFence fence = nullptr);
	VkBool32 SubmitPrimaryCommandBuffer(VkCommandBuffer buf, VkFence fence = nullptr);

	VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel level);

	void BeginBufferRecording(VkCommandBuffer buf, VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits)(0));
	void EndBufferRecording(VkCommandBuffer buf);
	void FreeCommandBuffer(VkCommandBuffer buf);

	void WriteToBuffer(VkCommandBuffer buf, std::function<void(VkCommandBuffer)> cmd);

private:
	VulkanDevice & device;
	std::mutex poolLock;
	VkCommandPool commandPool;
	CommandQueue* queue;

};

enum class CommandPoolType {
	graphics,
	transfer,
	compute,
};

struct GraphicsWork {
	std::function<void(const VkCommandBuffer)> work;

	VulkanFence fence;
	CommandPoolType type;

	std::vector<VulkanSemaphore> waitSemaphores;
	std::vector<VulkanSemaphore> signalSemaphores;
	std::vector<std::shared_ptr<VulkanBuffer>> buffersToClean;
	std::vector<Signal> signals; //signal on cpu completion of work

	explicit GraphicsWork(std::function<void(const VkCommandBuffer)> work,
		CommandPoolType type,
		VulkanDevice& device,
		std::vector<VulkanSemaphore>&& waitSemaphores,
		std::vector<VulkanSemaphore>&& signalSemaphores,
		std::vector<std::shared_ptr<VulkanBuffer>>&& buffersToClean,
		std::vector<Signal>&& signals)
		:
		work(work), type(type),
		fence(device),
		waitSemaphores(std::move(waitSemaphores)),
		signalSemaphores(std::move(signalSemaphores)),
		buffersToClean(std::move(buffersToClean)),
		signals(std::move(signals))
	{}

	GraphicsWork(const GraphicsWork& work) = default;
	GraphicsWork& operator=(const GraphicsWork& work) = default;
	GraphicsWork(GraphicsWork&& work) = default;
	GraphicsWork& operator=(GraphicsWork&& work) = default;

};

struct GraphicsCleanUpWork {
	VulkanFence fence;
	CommandPool* pool;
	VkCommandBuffer cmdBuf;
	std::vector<std::shared_ptr<VulkanBuffer>> buffers;
	std::vector<Signal> signals;

	explicit GraphicsCleanUpWork(GraphicsWork&& work,
		CommandPool* pool,
		VkCommandBuffer cmdBuf) :
		pool(pool),
		cmdBuf(cmdBuf),
		fence(std::move(work.fence)),
		buffers(std::move(work.buffersToClean)),
		signals(std::move(work.signals))
	{}

	explicit GraphicsCleanUpWork(VulkanFence&& fence,
		CommandPool* pool,
		VkCommandBuffer cmdBuf,
		std::vector<std::shared_ptr<VulkanBuffer>>&& buffers,
		std::vector<Signal>&& signals) :
		fence(std::move(fence)),
		pool(pool),
		cmdBuf(cmdBuf),
		buffers(std::move(buffers)),
		signals(std::move(signals))
	{}


	GraphicsCleanUpWork(const GraphicsCleanUpWork& work) = default;
	GraphicsCleanUpWork& operator=(const GraphicsCleanUpWork& work) = default;
	GraphicsCleanUpWork(GraphicsCleanUpWork&& work) = default;
	GraphicsCleanUpWork& operator=(GraphicsCleanUpWork&& work) = default;
};

class GraphicsCommandWorker {
public:
	GraphicsCommandWorker(VulkanDevice& device,
		ConcurrentQueue<GraphicsWork>& workQueue,
		std::vector<GraphicsCleanUpWork>& finishQueue,
		std::mutex& finishQueueLock,
		bool startActive = true);

	GraphicsCommandWorker(const GraphicsCommandWorker& other) = delete; //copy
	GraphicsCommandWorker(GraphicsCommandWorker&& other) = delete; //move
	GraphicsCommandWorker& operator=(const GraphicsCommandWorker&) = default;
	GraphicsCommandWorker& operator=(GraphicsCommandWorker&&) = default;

	~GraphicsCommandWorker();

	void CleanUp();

	void StopWork();

private:
	VulkanDevice & device;
	void Work();
	std::thread workingThread;

	bool keepWorking = true; //default to start working
	ConcurrentQueue<GraphicsWork>& workQueue;
	std::mutex& finishQueueLock;
	std::vector<GraphicsCleanUpWork>& finishQueue;

	CommandPool graphicsPool;
	CommandPool transferPool;
	CommandPool computePool;

};



class FrameObject {
public:
	FrameObject(VulkanDevice& device, int frameD);
	~FrameObject();



	VkResult AquireNextSwapchainImage(VkSwapchainKHR swapchain);
	void PrepareFrame();
	void SubmitFrame();

	VkSubmitInfo GetSubmitInfo();
	VkPresentInfoKHR GetPresentInfo();

	VkCommandBuffer GetPrimaryCmdBuf();

	VkFence GetCommandFence();

private:
	VulkanDevice & device;
	uint32_t frameIndex; //which frame in the queue it is

	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	VulkanFence commandFence;
	bool firstUse = true;

	CommandPool commandPool;
	VkCommandBuffer primaryCmdBuf;
	//std::vector<VkCommandBuffer> secondCmdBufs;

};