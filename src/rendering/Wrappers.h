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

	void Wait (bool condition = VK_TRUE);

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
	CommandPool (VulkanDevice& device, VkCommandPoolCreateFlags flags, CommandQueue& queue);
	CommandPool (CommandPool& cmd) = delete;
	CommandPool& operator= (const CommandPool& cmd) = delete;
	~CommandPool ();

	VkBool32 ResetPool ();
	VkBool32 ResetCommandBuffer (VkCommandBuffer cmdBuf);

	VkCommandBuffer GetCommandBuffer (VkCommandBufferLevel level, VkCommandBufferUsageFlags flags = 0);

	VkBool32 ReturnCommandBuffer (VkCommandBuffer,
	    VulkanFence& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores = {},
	    std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores = {});

	void SubmitCommandBuffer (VkCommandBuffer,
	    VulkanFence& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores = {},
	    std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores = {});

	VkCommandBuffer AllocateCommandBuffer (VkCommandBufferLevel level);

	void BeginBufferRecording (VkCommandBuffer buf, VkCommandBufferUsageFlags flags = 0);
	void EndBufferRecording (VkCommandBuffer buf);
	void FreeCommandBuffer (VkCommandBuffer buf);

	void WriteToBuffer (VkCommandBuffer buf, std::function<void(VkCommandBuffer)> cmd);

	private:
	VulkanDevice& device;
	std::mutex poolLock;
	CommandQueue& queue;
	VkCommandPool commandPool;
};

class CommandBuffer
{
	public:
	enum class State
	{
		error,
		empty,
		allocated,
		recording,
		ready,
		submitted
	};

	explicit CommandBuffer (CommandPool& pool, VkCommandBufferLevel level);

	void Allocate ();

	void Begin (VkCommandBufferUsageFlags flags = 0);
	void End ();

	void SetFence (std::shared_ptr<VulkanFence>& fence);
	void SetWaitSemaphores (std::vector<std::shared_ptr<VulkanSemaphore>>& sems);
	void SetSignalSemaphores (std::vector<std::shared_ptr<VulkanSemaphore>>& sems);

	void Submit ();

	void Wait ();

	void Free ();

	VkCommandBuffer Get () { return cmdBuf; }
	VkCommandBuffer* GetPtr () { return &cmdBuf; }


	std::shared_ptr<VulkanFence> fence;
	std::vector<std::shared_ptr<VulkanSemaphore>> wait_semaphores;
	std::vector<std::shared_ptr<VulkanSemaphore>> signal_semaphores;

	private:
	CommandPool* pool; // can't be copied if its a reference...
	VkCommandBufferLevel level;
	State state = State::empty;
	VkCommandBuffer cmdBuf = nullptr;
};

class CommandPoolGroup
{
	public:
	CommandPoolGroup (VulkanDevice& device);

	CommandPool graphicsPool;
	CommandPool transferPool;
	CommandPool computePool;
};

enum class WorkType
{
	graphics,
	transfer,
	compute,
};

struct GraphicsCleanUpWork
{
	CommandBuffer cmdBuf;
	std::vector<std::shared_ptr<VulkanBuffer>> buffers;
	std::vector<Signal> signals;

	explicit GraphicsCleanUpWork (
	    CommandBuffer cmdBuf, std::vector<std::shared_ptr<VulkanBuffer>>& buffers, std::vector<Signal>& signals)
	: cmdBuf (cmdBuf), buffers (buffers), signals (signals)
	{
	}
};

class FrameObject
{
	public:
	FrameObject (VulkanDevice& device, int frameD);
	~FrameObject ();

	VkResult AquireNextSwapchainImage (VkSwapchainKHR swapchain);

	void PrepareFrame ();
	void SubmitFrame ();

	VkSubmitInfo GetDepthSubmitInfo ();

	VkSubmitInfo GetSubmitInfo ();
	VkPresentInfoKHR GetPresentInfo ();

	VkCommandBuffer GetPrimaryCmdBuf ();

	VkFence GetCommandFence ();

	private:
	VulkanDevice& device;
	uint32_t frameIndex; // which frame in the queue it is

	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	std::shared_ptr<VulkanFence> commandFence;

	CommandPool commandPool;
	CommandBuffer primary_command_buffer;
};