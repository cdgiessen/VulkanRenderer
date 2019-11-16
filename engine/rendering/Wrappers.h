#pragma once

#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include "vk_mem_alloc.h"
#include <vulkan/vulkan.h>
class VulkanFence
{
	public:
	VulkanFence (VkDevice device, VkFenceCreateFlags flags = 0);
	~VulkanFence ();

	VulkanFence (VulkanFence const& fence) = delete;
	VulkanFence& operator= (VulkanFence const& fence) = delete;

	VulkanFence (VulkanFence&& other);
	VulkanFence& operator= (VulkanFence&& other);


	bool Check () const;

	void Wait (bool condition = VK_TRUE) const;

	VkFence Get () const;

	void Reset () const;

	private:
	VkDevice device;
	VkFence fence;
};

class VulkanSemaphore
{
	public:
	VulkanSemaphore (VkDevice device);
	~VulkanSemaphore ();

	VulkanSemaphore (VulkanSemaphore const& fence) = delete;
	VulkanSemaphore& operator= (VulkanSemaphore const& fence) = delete;

	VulkanSemaphore (VulkanSemaphore&& other);
	VulkanSemaphore& operator= (VulkanSemaphore&& other);

	VkSemaphore Get ();
	VkSemaphore* GetPtr ();

	private:
	VkDevice device;
	VkSemaphore semaphore;
};

class CommandQueue
{
	public:
	CommandQueue (VkDevice const device, int queueFamily);

	// not a copyable/movable type
	CommandQueue (CommandQueue const& cmd) = delete;
	CommandQueue& operator= (CommandQueue const& cmd) = delete;
	CommandQueue (CommandQueue&& cmd) = delete;
	CommandQueue& operator= (CommandQueue&& cmd) = delete;

	void SubmitCommandBuffer (VkCommandBuffer buffer, VulkanFence const& fence);

	void SubmitCommandBuffers (std::vector<VkCommandBuffer> buffer,
	    VulkanFence const& fence,
	    std::vector<VkSemaphore> const& waitSemaphores,
	    std::vector<VkSemaphore> const& signalSemaphores,
	    VkPipelineStageFlags const stageMask);

	void Submit (VkSubmitInfo const& submitInfo, VulkanFence const& fence);

	int GetQueueFamily ();

	VkResult PresentQueueSubmit (VkPresentInfoKHR presentInfo);
	void QueueWaitIdle ();

	private:
	VkDevice const device;
	std::mutex submissionMutex;
	VkQueue queue;
	int queueFamily;
};

class CommandPool
{
	public:
	CommandPool (VkDevice device, CommandQueue& queue, VkCommandPoolCreateFlags flags = 0);
	~CommandPool ();
	CommandPool (CommandPool& cmd) = delete;
	CommandPool& operator= (const CommandPool& cmd) = delete;
	CommandPool (CommandPool&& cmd);
	CommandPool& operator= (CommandPool&& cmd) noexcept;

	VkBool32 ResetPool ();
	VkBool32 ResetCommandBuffer (VkCommandBuffer cmdBuf);

	VkCommandBuffer GetCommandBuffer (VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    VkCommandBufferUsageFlags flags = 0);

	VkBool32 ReturnCommandBuffer (VkCommandBuffer,
	    VulkanFence const& fence,
	    std::vector<VkSemaphore> const& waitSemaphores,
	    std::vector<VkSemaphore> const& signalSemaphores);

	VkCommandBuffer AllocateCommandBuffer (VkCommandBufferLevel level);

	void BeginBufferRecording (VkCommandBuffer buf, VkCommandBufferUsageFlags flags = 0);
	void EndBufferRecording (VkCommandBuffer buf);
	void FreeCommandBuffer (VkCommandBuffer buf);

	void WriteToBuffer (VkCommandBuffer buf, std::function<void (VkCommandBuffer)> const& cmd);

	private:
	friend class CommandBuffer;

	VkDevice device;
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
	~CommandBuffer ();
	CommandBuffer (CommandBuffer& cmd) = delete;
	CommandBuffer& operator= (const CommandBuffer& cmd) = delete;
	CommandBuffer (CommandBuffer&& cmd);
	CommandBuffer& operator= (CommandBuffer&& cmd) noexcept;

	CommandBuffer& Allocate ();

	CommandBuffer& Begin (VkCommandBufferUsageFlags flags = 0);
	CommandBuffer& End ();

	CommandBuffer& Submit ();
	CommandBuffer& Submit (std::vector<VkSemaphore> const& waits,
	    std::vector<VkSemaphore> const& signals,
	    VkPipelineStageFlags const stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	CommandBuffer& Wait ();

	CommandBuffer& Free ();

	CommandBuffer& WriteTo (std::function<void (const VkCommandBuffer)> const& work);

	VkCommandBuffer Get () const { return cmdBuf; }

	VulkanFence const& GetFence () const { return fence; }

	private:
	CommandPool* pool;
	VkCommandBufferLevel level;
	State state = State::empty;
	VkCommandBuffer cmdBuf = nullptr;

	VulkanFence fence;
};
