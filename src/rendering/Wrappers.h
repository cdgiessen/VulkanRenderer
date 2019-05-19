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
class VulkanSwapChain;

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

	void SubmitCommandBuffer (VkCommandBuffer buffer, VulkanFence& fence);

	void SubmitCommandBuffer (VkCommandBuffer buffer,
	    VulkanFence& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& waitSemaphores,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& signalSemaphores);

	void Submit (VkSubmitInfo& submitInfo, VulkanFence& fence);

	int GetQueueFamily ();

	VkResult PresentQueueSubmit (VkPresentInfoKHR presentInfo);
	void QueueWaitIdle ();

	private:
	const VulkanDevice& device;
	std::mutex submissionMutex;
	VkQueue queue;
	int queueFamily;
};

class CommandPool
{
	public:
	CommandPool (VulkanDevice& device, CommandQueue& queue, VkCommandPoolCreateFlags flags = 0);
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

	explicit CommandBuffer (CommandPool& pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	CommandBuffer& Allocate ();

	CommandBuffer& Begin (VkCommandBufferUsageFlags flags = 0);
	CommandBuffer& End ();

	CommandBuffer& SetFence (std::shared_ptr<VulkanFence> fence);

	CommandBuffer& Submit ();
	CommandBuffer& Submit (std::vector<std::shared_ptr<VulkanSemaphore>>& waits,
	    std::vector<std::shared_ptr<VulkanSemaphore>>& signals);

	CommandBuffer& Wait ();

	CommandBuffer& Free ();

	VkCommandBuffer Get () { return cmdBuf; }
	VkCommandBuffer* GetPtr () { return &cmdBuf; }

	VulkanFence& GetFence () { return *fence.get (); }

	private:
	CommandPool* pool; // can't be copied if its a reference...
	VkCommandBufferLevel level;
	State state = State::empty;
	VkCommandBuffer cmdBuf = nullptr;

	std::shared_ptr<VulkanFence> fence;
};

///////// Single Use Command Buffer ////////

class SingleUseCommandBuffer
{
	public:
	SingleUseCommandBuffer (VulkanDevice& device);
	void Submit ();
	VkCommandBuffer Get () { return buffer.Get (); }

	private:
	VulkanDevice& device;
	CommandPool pool;
	CommandBuffer buffer;
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

	VkResult AcquireNextSwapchainImage (VkSwapchainKHR swapchain);

	void PrepareFrame ();
	void Submit (CommandQueue& queue);

	VkResult Present (VulkanSwapChain& swapChain, CommandQueue& presentQueue);

	VkCommandBuffer GetPrimaryCmdBuf ();

	private:
	VulkanDevice& device;
	uint32_t frameIndex; // which frame in the queue it is

	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	std::shared_ptr<VulkanFence> commandFence;
	CommandPool commandPool;
	CommandBuffer primary_command_buffer;

	VkPipelineStageFlags stageMasks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
};