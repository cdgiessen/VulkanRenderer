#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <vulkan/vulkan.h>

#include "Buffer.h"

#include "util/ConcurrentQueue.h"

using Signal = std::shared_ptr<bool>;

// Default fence timeout in nanoseconds
constexpr long DEFAULT_FENCE_TIMEOUT = 1000000000;

class VulkanDevice;

class VulkanFence
{
	public:
	VulkanFence (VulkanDevice& device, long timeout = DEFAULT_FENCE_TIMEOUT, VkFenceCreateFlags flags = 0);
	~VulkanFence ();

	bool Check ();

	void WaitTillTrue ();
	void WaitTillFalse ();

	VkFence Get ();

	void Reset ();


	private:
	VulkanDevice& device;
	VkFence fence;
	int timeout;
};

std::vector<VkFence> CreateFenceArray (std::vector<VulkanFence>& fences);

class VulkanSemaphore
{
	public:
	VulkanSemaphore (VulkanDevice& device);
	~VulkanSemaphore ();

	VkSemaphore Get ();
	VkSemaphore* GetPtr ();

	std::mutex sem_lock;
	private:
	VulkanDevice& device;
	VkSemaphore semaphore;
};

std::vector<VkSemaphore> CreateSemaphoreArray (std::vector<VulkanSemaphore>& sems);

class CommandQueue
{
	public:
	CommandQueue (const VulkanDevice& device, int queueFamily);

	// No copy or move construction (so that it won't get duplicated)
	CommandQueue (const CommandQueue& cmd) = delete;
	CommandQueue& operator= (const CommandQueue& cmd) = delete;
	CommandQueue (CommandQueue&& cmd) = delete;
	CommandQueue& operator= (CommandQueue&& cmd) = delete;

	void Submit (VkSubmitInfo info, VkFence fence);

	void SubmitCommandBuffer (VkCommandBuffer buf, VkFence fence);

	void SubmitCommandBuffer (VkCommandBuffer buffer,
	    VulkanFence& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& waitSemaphores,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& signalSemaphores);

	int GetQueueFamily ();
	std::mutex& GetQueueMutex ();
	VkQueue GetQueue ();

	void WaitForFences (VkFence fence);

	private:
	const VulkanDevice& device;
	std::mutex submissionMutex;
	VkQueue queue;
	int queueFamily;
};


class CommandPool
{
	public:
	CommandPool (VulkanDevice& device, VkCommandPoolCreateFlags flags, CommandQueue* queue);
	CommandPool (CommandPool& cmd) = delete;
	CommandPool& operator= (const CommandPool& cmd) = delete;
	CommandPool (CommandPool&& cmd);
	CommandPool& operator= (CommandPool&& cmd);
	~CommandPool ();

	VkBool32 ResetPool ();
	VkBool32 ResetCommandBuffer (VkCommandBuffer cmdBuf);

	VkCommandBuffer GetOneTimeUseCommandBuffer ();
	VkCommandBuffer GetPrimaryCommandBuffer (bool beginBufferRecording = true);
	VkCommandBuffer GetSecondaryCommandBuffer (bool beginBufferRecording = true);

	VkBool32 SubmitCommandBuffer (VkCommandBuffer,
	    VulkanFence& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& waitSemaphores,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& signalSemaphores);

	VkBool32 SubmitOneTimeUseCommandBuffer (VkCommandBuffer cmdBuffer, VkFence fence = nullptr);
	VkBool32 SubmitPrimaryCommandBuffer (VkCommandBuffer buf, VkFence fence = nullptr);

	VkCommandBuffer AllocateCommandBuffer (VkCommandBufferLevel level);

	void BeginBufferRecording (VkCommandBuffer buf,
	    VkCommandBufferUsageFlagBits flags = (VkCommandBufferUsageFlagBits) (0));
	void EndBufferRecording (VkCommandBuffer buf);
	void FreeCommandBuffer (VkCommandBuffer buf);

	void WriteToBuffer (VkCommandBuffer buf, std::function<void(VkCommandBuffer)> cmd);

	private:
	VulkanDevice& device;
	std::mutex poolLock;
	VkCommandPool commandPool;
	CommandQueue* queue;
};

class CommandPoolGroup
{
	public:
	CommandPoolGroup (VulkanDevice& device);

	CommandPool graphicsPool;
	CommandPool transferPool;
	CommandPool computePool;
};

class CommandPoolManager
{
	public:
	CommandPoolManager (int num_groups, VulkanDevice& device);

	CommandPoolGroup& Get (int index);

	private:
	std::vector<CommandPoolGroup> groups;
};

enum class WorkType
{
	graphics,
	transfer,
	compute,
};

struct GraphicsWork
{
	std::function<void(const VkCommandBuffer)> work;

	std::shared_ptr<VulkanFence> fence;
	WorkType type;

	std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores;
	std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores;
	std::vector<std::shared_ptr<VulkanBuffer>> buffersToClean;
	std::vector<Signal> signals; // signal on cpu completion of work

	explicit GraphicsWork (std::function<void(const VkCommandBuffer)> work,
	    WorkType type,
	    VulkanDevice& device,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& waitSemaphores,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& signalSemaphores,
	    std::vector<std::shared_ptr<VulkanBuffer>>& buffersToClean,
	    std::vector<Signal>& signals)
	: work (work),
	  type (type),
	  fence (std::make_shared<VulkanFence> (device)),
	  waitSemaphores (waitSemaphores),
	  signalSemaphores (signalSemaphores),
	  buffersToClean (buffersToClean),
	  signals (signals)
	{
	}

	GraphicsWork (const GraphicsWork& work) = default;
	GraphicsWork& operator= (const GraphicsWork& work) = default;
	GraphicsWork (GraphicsWork&& work) = default;
	GraphicsWork& operator= (GraphicsWork&& work) = default;
};

struct GraphicsCleanUpWork
{
	std::shared_ptr<VulkanFence> fence;
	std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores;
	std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores;
	CommandPool* pool;
	VkCommandBuffer cmdBuf;
	std::vector<std::shared_ptr<VulkanBuffer>> buffers;
	std::vector<Signal> signals;

	explicit GraphicsCleanUpWork (GraphicsWork& work, CommandPool* pool, VkCommandBuffer cmdBuf)
	: pool (pool),
	  cmdBuf (cmdBuf),
	  fence (work.fence),
	  buffers (work.buffersToClean),
	  signals (work.signals),
	  waitSemaphores (work.waitSemaphores),
	  signalSemaphores (work.signalSemaphores)
	{
	}

	explicit GraphicsCleanUpWork (std::shared_ptr<VulkanFence>& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& waitSemaphores,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& signalSemaphores,
	    CommandPool* pool,
	    VkCommandBuffer cmdBuf,
	    std::vector<std::shared_ptr<VulkanBuffer>>& buffers,
	    std::vector<Signal>& signals)
	: fence (fence),
	  pool (pool),
	  cmdBuf (cmdBuf),
	  buffers (buffers),
	  signals (signals),
	  waitSemaphores (waitSemaphores),
	  signalSemaphores (signalSemaphores)
	{
	}


	GraphicsCleanUpWork (const GraphicsCleanUpWork& work) = default;
	GraphicsCleanUpWork& operator= (const GraphicsCleanUpWork& work) = default;
	GraphicsCleanUpWork (GraphicsCleanUpWork&& work) = default;
	GraphicsCleanUpWork& operator= (GraphicsCleanUpWork&& work) = default;
};

class GraphicsCommandWorker
{
	public:
	GraphicsCommandWorker (VulkanDevice& device,
	    ConcurrentQueue<GraphicsWork>& workQueue,
	    std::vector<GraphicsCleanUpWork>& finishQueue,
	    std::mutex& finishQueueLock,
	    bool startActive = true);

	GraphicsCommandWorker (const GraphicsCommandWorker& other) = delete; // copy
	GraphicsCommandWorker (GraphicsCommandWorker&& other) = delete;      // move
	GraphicsCommandWorker& operator= (const GraphicsCommandWorker&) = default;
	GraphicsCommandWorker& operator= (GraphicsCommandWorker&&) = default;

	~GraphicsCommandWorker ();

	void StopWork ();

	bool IsFinishedWorking ();

	private:
	VulkanDevice& device;
	void Work ();
	std::thread workingThread;

	std::atomic_bool keepWorking = true; // default to start working
	ConcurrentQueue<GraphicsWork>& workQueue;
	std::mutex& finishQueueLock;
	std::vector<GraphicsCleanUpWork>& finishQueue;
	std::atomic_bool isDoneWorking = false;

	CommandPool graphicsPool;
	CommandPool transferPool;
	CommandPool computePool;
};

class FrameObject
{
	public:
	FrameObject (VulkanDevice& device, int frameD);
	~FrameObject ();



	VkResult AquireNextSwapchainImage (VkSwapchainKHR swapchain);
	void WaitTillDepthReady ();
	void WaitTillReady ();

	void PrepareDepthPass ();
	void EndDepthPass ();

	void PrepareFrame ();
	void SubmitFrame ();

	VkSubmitInfo GetDepthSubmitInfo ();

	VkSubmitInfo GetSubmitInfo ();
	VkPresentInfoKHR GetPresentInfo ();

	VkCommandBuffer GetDepthCmdBuf ();
	VkCommandBuffer GetPrimaryCmdBuf ();

	VkFence GetCommandFence ();
	VkFence GetDepthFence ();

	private:
	VulkanDevice& device;
	uint32_t frameIndex; // which frame in the queue it is

	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	VulkanFence depthFence;
	VulkanFence commandFence;

	CommandPool commandPool;
	VkCommandBuffer primaryCmdBuf;
	VkCommandBuffer depthCmdBuf;
};