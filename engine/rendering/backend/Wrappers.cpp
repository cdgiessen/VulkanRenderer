#include "Wrappers.h"

#include <utility>
#include <cassert>

#include "rendering/Initializers.h"

#include "RenderTools.h"

//////// Fence ////////////

// Default fence timeout in nanoseconds
constexpr long DEFAULT_FENCE_TIMEOUT = 1000000000;

auto create_fence (VkDevice device, VkFenceCreateFlags flags)
{
	VkFence fence;
	VkFenceCreateInfo fenceInfo = initializers::fence_create_info (flags);
	VK_CHECK_RESULT (vkCreateFence (device, &fenceInfo, nullptr, &fence))
	return VulkanHandle (device, fence, vkDestroyFence);
}

VulkanFence::VulkanFence (VkDevice device, VkFenceCreateFlags flags)
: fence (create_fence (device, flags))
{
}

bool VulkanFence::check () const
{
	VkResult out = vkGetFenceStatus (fence.device, fence.handle);
	if (out == VK_SUCCESS)
		return true;
	else if (out == VK_NOT_READY)
		return false;
	assert (out == VK_SUCCESS || out == VK_NOT_READY);
	return false;
}

void VulkanFence::wait (bool condition) const
{
	vkWaitForFences (fence.device, 1, &fence.handle, condition, DEFAULT_FENCE_TIMEOUT);
}

VkFence VulkanFence::get () const { return fence.handle; }

void VulkanFence::reset () const { vkResetFences (fence.device, 1, &fence.handle); }

///////////// Semaphore ///////////////

auto create_semaphore (VkDevice device)
{
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo semaphoreInfo = initializers::semaphore_create_info ();
	VK_CHECK_RESULT (vkCreateSemaphore (device, &semaphoreInfo, nullptr, &semaphore));
	return VulkanHandle (device, semaphore, vkDestroySemaphore);
}

VulkanSemaphore::VulkanSemaphore (VkDevice device) : semaphore (create_semaphore (device)) {}

VkSemaphore VulkanSemaphore::get () { return semaphore.handle; }

VkSemaphore* VulkanSemaphore::get_ptr () { return &semaphore.handle; }

///////////// Command Queue ////////////////

CommandQueue::CommandQueue (VkDevice device, int queueFamily)
{
	vkGetDeviceQueue (device, queueFamily, 0, &queue);
	this->queueFamily = queueFamily;
}

void CommandQueue::submit (VkCommandBuffer buffer, VulkanFence const& fence)
{
	std::vector<VkCommandBuffer> bufs = { buffer };
	submits (bufs, fence, {}, {}, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

void CommandQueue::submits (std::vector<VkCommandBuffer> cmdBuffer,
    VulkanFence const& fence,
    std::vector<VkSemaphore> const& waitSemaphores,
    std::vector<VkSemaphore> const& signalSemaphores,
    VkPipelineStageFlags const stageMask)
{

	auto submitInfo = initializers::submit_info ();
	submitInfo.commandBufferCount = static_cast<uint32_t> (cmdBuffer.size ());
	submitInfo.pCommandBuffers = cmdBuffer.data ();
	submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size ();
	submitInfo.pSignalSemaphores = signalSemaphores.data ();
	submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size ();
	submitInfo.pWaitSemaphores = waitSemaphores.data ();
	submitInfo.pWaitDstStageMask = &stageMask;

	submit (submitInfo, fence);
}

void CommandQueue::submit (VkSubmitInfo const& submitInfo, VulkanFence const& fence)
{
	std::lock_guard lock (submissionMutex);
	VK_CHECK_RESULT (vkQueueSubmit (queue, 1, &submitInfo, fence.get ()));
}

int CommandQueue::queue_family () const { return queueFamily; }
VkQueue CommandQueue::get () const { return queue; }
int CommandQueue::queue_index () const { return 0; };
VkResult CommandQueue::present (VkPresentInfoKHR presentInfo)
{
	std::lock_guard lock (submissionMutex);
	return vkQueuePresentKHR (queue, &presentInfo);
}

void CommandQueue::wait ()
{
	std::lock_guard lock (submissionMutex);
	vkQueueWaitIdle (queue);
}
/////// Command Pool ////////////

auto create_command_pool (VkDevice device, CommandQueue& queue, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo cmd_pool_info = initializers::command_pool_create_info ();
	cmd_pool_info.queueFamilyIndex = queue.queue_family ();
	cmd_pool_info.flags = flags;
	VkCommandPool pool;
	auto res = vkCreateCommandPool (device, &cmd_pool_info, nullptr, &pool);
	assert (res == VK_SUCCESS);
	return VulkanHandle (device, pool, vkDestroyCommandPool);
}

CommandPool::CommandPool (VkDevice device, CommandQueue& queue, VkCommandPoolCreateFlags flags)
: queue (&queue), pool (create_command_pool (device, queue, flags))
{
}

VkBool32 CommandPool::reset_pool ()
{
	auto res = vkResetCommandPool (pool.device, pool.handle, 0);
	assert (res == VK_SUCCESS);
	return VK_TRUE;
}

VkBool32 CommandPool::reset_command_buffer (VkCommandBuffer cmdBuf)
{
	auto res = vkResetCommandBuffer (cmdBuf, {});
	assert (res == VK_SUCCESS);
	return VK_TRUE;
}

VkCommandBuffer CommandPool::allocate (VkCommandBufferLevel level)
{
	VkCommandBuffer buf;

	VkCommandBufferAllocateInfo allocInfo =
	    initializers::command_buffer_allocate_info (pool.handle, level, 1);

	auto res = vkAllocateCommandBuffers (pool.device, &allocInfo, &buf);
	assert (res == VK_SUCCESS);
	return buf;
}

void CommandPool::begin (VkCommandBuffer buf, VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo = initializers::command_buffer_begin_info ();
	beginInfo.flags = flags;

	auto res = vkBeginCommandBuffer (buf, &beginInfo);
	assert (res == VK_SUCCESS);
}

void CommandPool::end (VkCommandBuffer buf)
{
	auto res = vkEndCommandBuffer (buf);
	assert (res == VK_SUCCESS);
}

void CommandPool::free (VkCommandBuffer buf)
{
	vkFreeCommandBuffers (pool.device, pool.handle, 1, &buf);
}

void CommandPool::write (VkCommandBuffer buf, std::function<void (VkCommandBuffer)> const& cmds)
{
	cmds (buf);
}

///////// CommandBuffer /////////

CommandBuffer::CommandBuffer (CommandPool& pool, VkCommandBufferLevel level)
: pool (&pool), level (level), fence (pool.pool.device)
{
}

CommandBuffer::~CommandBuffer ()
{
	if (cmdBuf != nullptr) pool->free (cmdBuf);
}

CommandBuffer::CommandBuffer (CommandBuffer&& cmd) noexcept
: pool (cmd.pool), level (cmd.level), state (cmd.state), cmdBuf (cmd.cmdBuf), fence (std::move (cmd.fence))
{
	cmd.cmdBuf = nullptr;
}

CommandBuffer& CommandBuffer::operator= (CommandBuffer&& cmd) noexcept
{
	pool = cmd.pool;
	level = cmd.level;
	state = cmd.state;
	cmdBuf = cmd.cmdBuf;
	fence = std::move (cmd.fence);
	cmd.cmdBuf = nullptr;
	return *this;
}

CommandBuffer& CommandBuffer::allocate ()
{
	assert (state == State::empty);
	cmdBuf = pool->allocate (level);
	state = State::allocated;
	return *this;
}

CommandBuffer& CommandBuffer::begin (VkCommandBufferUsageFlags flags)
{
	assert (state == State::allocated);
	pool->begin (cmdBuf, flags);
	state = State::recording;
	return *this;
}

CommandBuffer& CommandBuffer::write (std::function<void (const VkCommandBuffer)> const& work)
{
	assert (state == State::recording);
	pool->write (cmdBuf, work);
	return *this;
}

CommandBuffer& CommandBuffer::end ()
{
	assert (state == State::recording);
	pool->end (cmdBuf);
	state = State::ready;
	return *this;
}

CommandBuffer& CommandBuffer::submit ()
{
	assert (state == State::ready);
	pool->queue->submit (cmdBuf, fence);
	state = State::submitted;
	return *this;
}

CommandBuffer& CommandBuffer::submit (std::vector<VkSemaphore> const& waits,
    std::vector<VkSemaphore> const& signals,
    VkPipelineStageFlags const stageMask)
{
	assert (state == State::ready);
	std::vector<VkCommandBuffer> bufs = { cmdBuf };
	pool->queue->submits (bufs, fence, waits, signals, stageMask);
	state = State::submitted;
	return *this;
}

CommandBuffer& CommandBuffer::wait ()
{
	// assert (state == State::submitted);
	fence.wait ();
	fence.reset ();
	state = State::allocated;
	return *this;
}

CommandBuffer& CommandBuffer::free ()
{
	pool->free (cmdBuf);
	cmdBuf = nullptr;
	state = State::empty;
	return *this;
}
