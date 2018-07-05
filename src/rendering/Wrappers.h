#pragma once

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <set>

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
	VulkanFence(const VulkanDevice& device,
		long int timeout = DEFAULT_FENCE_TIMEOUT,
		VkFenceCreateFlags flags = VK_FLAGS_NONE);
	void CleanUp();

	bool Check();

	void WaitTillTrue();
	void WaitTillFalse();
	//void CleanUp();
	VkFence Get();

private:
	const VulkanDevice& device;
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
	CommandPool(VulkanDevice& device);
	CommandPool(const CommandPool& cmd) = default;
	CommandPool& operator=(const CommandPool& cmd) = default;
	CommandPool(CommandPool&& cmd) = default;
	CommandPool& operator=(CommandPool&& cmd) = default;

	VkBool32 Setup(VkCommandPoolCreateFlags flags, CommandQueue* queue);
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

class CommandBuffer {
public:
	CommandBuffer(VulkanDevice& device, CommandPool& pool);
	CommandBuffer(const CommandBuffer& cmd) = default;
	CommandBuffer& operator=(const CommandBuffer& cmd) = default;
	CommandBuffer(CommandBuffer&& cmd) = default;
	CommandBuffer& operator=(CommandBuffer&& cmd) = default;

	void Create();
	void CleanUp();

	void Begin();
	void End();

	void AddSynchronization(std::vector<VulkanSemaphore> waitSemaphores, std::vector<VulkanSemaphore> signalSemaphores);
	void Write(std::function<void(VkCommandBuffer)> cmds);

	void Submit();

	bool CheckFence();
private:
	CommandPool & pool;
	VkCommandBuffer buf;
	VulkanFence fence;
	std::vector<VulkanSemaphore> waitSemaphores;
	std::vector<VulkanSemaphore> signalSemaphores;
};

struct GraphicsWork {
	std::function<void(const VkCommandBuffer)> work;
	std::function<void()> cleanUp;
	CommandBuffer cmdBuf;

	GraphicsWork(std::function<void(const VkCommandBuffer)> work,
		std::function<void()> cleanUp, CommandBuffer cmdBuf) :
		work(work), cleanUp(cleanUp), cmdBuf(cmdBuf)
	{}
	GraphicsWork(const GraphicsWork& cmd) = default;
	GraphicsWork& operator=(const GraphicsWork& cmd) = default;
	GraphicsWork(GraphicsWork&& cmd) = default;
	GraphicsWork& operator=(GraphicsWork&& cmd) = default;
};

class GraphicsCommandWorker {
public:
	GraphicsCommandWorker(VulkanDevice& device,
		CommandQueue* queue,
		ConcurrentQueue<GraphicsWork>& workQueue,
		std::vector<GraphicsWork>& finishQueue,
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
	std::vector<GraphicsWork>& finishQueue;
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

private:
	VulkanDevice & device;
	uint32_t frameIndex; //which frame in the queue it is

	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	CommandPool commandPool;
	VkCommandBuffer primaryCmdBuf;
	//std::vector<VkCommandBuffer> secondCmdBufs;

};