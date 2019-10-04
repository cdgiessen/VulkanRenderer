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

// Default fence timeout in nanoseconds
constexpr long DEFAULT_FENCE_TIMEOUT = 1000000000;

class VulkanDevice;

class VulkanFence
{
	public:
	VulkanFence (VulkanDevice& device, long timeout = DEFAULT_FENCE_TIMEOUT, VkFenceCreateFlags flags = 0);
	~VulkanFence ();

	bool Check () const;

	void Wait (bool condition = VK_TRUE) const;

	VkFence Get () const;

	void Reset () const;

	private:
	VulkanDevice& device;
	VkFence fence;
	int timeout;
};

std::vector<VkFence> CreateFenceArray (std::vector<VulkanFence> const& fences);

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

std::vector<VkSemaphore> CreateSemaphoreArray (std::vector<VulkanSemaphore> const& sems);

class CommandQueue
{
	public:
	CommandQueue (VulkanDevice const& device, int queueFamily);

	// not a copyable/movable type
	CommandQueue (CommandQueue const& cmd) = delete;
	CommandQueue& operator= (CommandQueue const& cmd) = delete;
	CommandQueue (CommandQueue&& cmd) = delete;
	CommandQueue& operator= (CommandQueue&& cmd) = delete;

	void SubmitCommandBuffer (VkCommandBuffer buffer, VulkanFence const& fence);

	void SubmitCommandBuffer (VkCommandBuffer buffer,
	    VulkanFence const& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>> const& waitSemaphores,
	    std::vector<std::shared_ptr<VulkanSemaphore>> const& signalSemaphores);

	void Submit (VkSubmitInfo const& submitInfo, VulkanFence const& fence);

	int GetQueueFamily ();

	VkResult PresentQueueSubmit (VkPresentInfoKHR presentInfo);
	void QueueWaitIdle ();

	private:
	VulkanDevice const& device;
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
	CommandPool (CommandPool&& cmd);
	CommandPool& operator= (CommandPool&& cmd) noexcept;
	~CommandPool ();

	VkBool32 ResetPool ();
	VkBool32 ResetCommandBuffer (VkCommandBuffer cmdBuf);

	VkCommandBuffer GetCommandBuffer (VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    VkCommandBufferUsageFlags flags = 0);

	VkBool32 ReturnCommandBuffer (VkCommandBuffer,
	    VulkanFence const& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>> const& waitSemaphores = {},
	    std::vector<std::shared_ptr<VulkanSemaphore>> const& signalSemaphores = {});

	void SubmitCommandBuffer (VkCommandBuffer,
	    VulkanFence const& fence,
	    std::vector<std::shared_ptr<VulkanSemaphore>> const& waitSemaphores = {},
	    std::vector<std::shared_ptr<VulkanSemaphore>> const& signalSemaphores = {});

	VkCommandBuffer AllocateCommandBuffer (VkCommandBufferLevel level);

	void BeginBufferRecording (VkCommandBuffer buf, VkCommandBufferUsageFlags flags = 0);
	void EndBufferRecording (VkCommandBuffer buf);
	void FreeCommandBuffer (VkCommandBuffer buf);

	void WriteToBuffer (VkCommandBuffer buf, std::function<void(VkCommandBuffer)> const& cmd);

	private:
	VulkanDevice* device;
	CommandQueue* queue;
	VkCommandPool command_pool;
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

	CommandBuffer& SetFence (std::shared_ptr<VulkanFence> const& fence);

	CommandBuffer& Submit ();
	CommandBuffer& Submit (std::vector<std::shared_ptr<VulkanSemaphore>> const& waits,
	    std::vector<std::shared_ptr<VulkanSemaphore>> const& signals);

	CommandBuffer& Wait ();

	CommandBuffer& Free ();

	CommandBuffer& WriteTo (std::function<void(const VkCommandBuffer)> const& work);

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
