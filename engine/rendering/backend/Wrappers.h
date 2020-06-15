#pragma once

#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include "RenderTools.h"

class VulkanFence
{
	public:
	VulkanFence (VkDevice device, VkFenceCreateFlags flags = 0);

	bool check () const;

	void wait (bool condition = VK_TRUE) const;

	VkFence get () const;

	void reset () const;

	private:
	VulkanHandle<VkFence, PFN_vkDestroyFence> fence;
};

class VulkanSemaphore
{
	public:
	VulkanSemaphore (VkDevice device);

	VkSemaphore get ();
	VkSemaphore* get_ptr ();

	private:
	VulkanHandle<VkSemaphore, PFN_vkDestroySemaphore> semaphore;
};

class CommandQueue
{
	public:
	CommandQueue (VkDevice const device, int queueFamily);

	void submit (VkCommandBuffer buffer, VulkanFence const& fence);

	void submits (std::vector<VkCommandBuffer> buffer,
	    VulkanFence const& fence,
	    std::vector<VkSemaphore> const& waitSemaphores,
	    std::vector<VkSemaphore> const& signalSemaphores,
	    VkPipelineStageFlags const stageMask);

	void submit (VkSubmitInfo const& submitInfo, VulkanFence const& fence);

	VkQueue get () const;
	int queue_family () const;
	int queue_index () const;

	VkResult present (VkPresentInfoKHR presentInfo);
	void wait ();

	private:
	std::mutex submissionMutex;
	VkQueue queue;
	int queueFamily;
};

class CommandPool
{
	public:
	CommandPool (VkDevice device, CommandQueue& queue, VkCommandPoolCreateFlags flags = 0);

	VkBool32 reset_pool ();
	VkBool32 reset_command_buffer (VkCommandBuffer cmdBuf);

	VkCommandBuffer allocate (VkCommandBufferLevel level);

	void begin (VkCommandBuffer buf, VkCommandBufferUsageFlags flags = 0);
	void end (VkCommandBuffer buf);
	void free (VkCommandBuffer buf);

	void write (VkCommandBuffer buf, std::function<void (VkCommandBuffer)> const& cmd);

	private:
	friend class CommandBuffer;

	CommandQueue* queue;
	VulkanHandle<VkCommandPool, PFN_vkDestroyCommandPool> pool;
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
	CommandBuffer (CommandBuffer&& cmd) noexcept;
	CommandBuffer& operator= (CommandBuffer&& cmd) noexcept;

	CommandBuffer& allocate ();

	CommandBuffer& begin (VkCommandBufferUsageFlags flags = 0);
	CommandBuffer& end ();

	CommandBuffer& submit ();
	CommandBuffer& submit (std::vector<VkSemaphore> const& waits,
	    std::vector<VkSemaphore> const& signals,
	    VkPipelineStageFlags const stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	CommandBuffer& wait ();

	CommandBuffer& free ();

	CommandBuffer& write (std::function<void (const VkCommandBuffer)> const& work);

	VkCommandBuffer get () const { return cmdBuf; }

	VulkanFence const& get_fence () const { return fence; }

	private:
	CommandPool* pool;
	VkCommandBufferLevel level;
	State state = State::empty;
	VkCommandBuffer cmdBuf = nullptr;

	VulkanFence fence;
};
