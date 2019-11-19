#include "Wrappers.h"

#include <utility>

#include "rendering/Initializers.h"

#include "RenderTools.h"

//////// Fence ////////////

// Default fence timeout in nanoseconds
constexpr long DEFAULT_FENCE_TIMEOUT = 1000000000;

VulkanFence::VulkanFence (VkDevice device, VkFenceCreateFlags flags) : device (device)
{
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo (flags);
	VK_CHECK_RESULT (vkCreateFence (device, &fenceInfo, nullptr, &fence))
}

VulkanFence::~VulkanFence ()
{
	if (fence != nullptr) vkDestroyFence (device, fence, nullptr);
}

VulkanFence::VulkanFence (VulkanFence&& other) : device (other.device), fence (other.fence)
{
	other.fence = nullptr;
}
VulkanFence& VulkanFence::operator= (VulkanFence&& other)
{
	device = other.device;
	fence = other.fence;
	other.fence = nullptr;
	return *this;
}

bool VulkanFence::Check () const
{
	VkResult out = vkGetFenceStatus (device, fence);
	if (out == VK_SUCCESS)
		return true;
	else if (out == VK_NOT_READY)
		return false;
	assert (out == VK_SUCCESS || out == VK_NOT_READY);
	return false;
}

void VulkanFence::Wait (bool condition) const
{
	vkWaitForFences (device, 1, &fence, condition, DEFAULT_FENCE_TIMEOUT);
}

VkFence VulkanFence::Get () const { return fence; }

void VulkanFence::Reset () const { vkResetFences (device, 1, &fence); }

///////////// Semaphore ///////////////

VulkanSemaphore::VulkanSemaphore (VkDevice device) : device (device)
{
	VkSemaphoreCreateInfo semaphoreInfo = initializers::semaphoreCreateInfo ();

	VK_CHECK_RESULT (vkCreateSemaphore (device, &semaphoreInfo, nullptr, &semaphore));
}

VulkanSemaphore::~VulkanSemaphore ()
{
	if (semaphore != nullptr) vkDestroySemaphore (device, semaphore, nullptr);
}

VulkanSemaphore::VulkanSemaphore (VulkanSemaphore&& other)
: device (other.device), semaphore (other.semaphore)
{
	other.semaphore = nullptr;
}
VulkanSemaphore& VulkanSemaphore::operator= (VulkanSemaphore&& other)
{
	device = other.device;
	semaphore = other.semaphore;
	other.semaphore = nullptr;
	return *this;
}

VkSemaphore VulkanSemaphore::Get () { return semaphore; }

VkSemaphore* VulkanSemaphore::GetPtr () { return &semaphore; }

///////////// Command Queue ////////////////

CommandQueue::CommandQueue (VkDevice device, int queueFamily) : device (device)
{
	vkGetDeviceQueue (device, queueFamily, 0, &queue);
	this->queueFamily = queueFamily;
}

void CommandQueue::SubmitCommandBuffer (VkCommandBuffer buffer, VulkanFence const& fence)
{
	std::vector<VkCommandBuffer> bufs = { buffer };
	SubmitCommandBuffers (bufs, fence, {}, {}, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

void CommandQueue::SubmitCommandBuffers (std::vector<VkCommandBuffer> cmdBuffer,
    VulkanFence const& fence,
    std::vector<VkSemaphore> const& waitSemaphores,
    std::vector<VkSemaphore> const& signalSemaphores,
    VkPipelineStageFlags const stageMask)
{

	auto submitInfo = initializers::submitInfo ();
	submitInfo.commandBufferCount = cmdBuffer.size ();
	submitInfo.pCommandBuffers = cmdBuffer.data ();
	submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size ();
	submitInfo.pSignalSemaphores = signalSemaphores.data ();
	submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size ();
	submitInfo.pWaitSemaphores = waitSemaphores.data ();
	submitInfo.pWaitDstStageMask = &stageMask;

	Submit (submitInfo, fence);
}

void CommandQueue::Submit (VkSubmitInfo const& submitInfo, VulkanFence const& fence)
{
	std::lock_guard lock (submissionMutex);
	VK_CHECK_RESULT (vkQueueSubmit (queue, 1, &submitInfo, fence.Get ()));
}

int CommandQueue::GetQueueFamily () const { return queueFamily; }
VkQueue CommandQueue::GetQueue () const { return queue; }

VkResult CommandQueue::PresentQueueSubmit (VkPresentInfoKHR presentInfo)
{
	std::lock_guard lock (submissionMutex);
	return vkQueuePresentKHR (queue, &presentInfo);
}

void CommandQueue::QueueWaitIdle ()
{
	std::lock_guard lock (submissionMutex);
	vkQueueWaitIdle (queue);
}
/////// Command Pool ////////////

CommandPool::CommandPool (VkDevice device, CommandQueue& queue, VkCommandPoolCreateFlags flags)
: device (device), queue (&queue)
{
	VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo ();
	cmd_pool_info.queueFamilyIndex = queue.GetQueueFamily ();
	cmd_pool_info.flags = flags;

	auto res = vkCreateCommandPool (device, &cmd_pool_info, nullptr, &command_pool);
	assert (res == VK_SUCCESS);
}

CommandPool::~CommandPool ()
{
	if (command_pool != nullptr) vkDestroyCommandPool (device, command_pool, nullptr);
}

CommandPool::CommandPool (CommandPool&& other)
: device (other.device), queue (other.queue), command_pool (other.command_pool)
{
	other.command_pool = nullptr;
}
CommandPool& CommandPool::operator= (CommandPool&& other) noexcept
{
	device = other.device;
	queue = other.queue;
	command_pool = other.command_pool;

	other.command_pool = nullptr;
	return *this;
}

VkBool32 CommandPool::ResetPool ()
{
	auto res = vkResetCommandPool (device, command_pool, 0);
	assert (res == VK_SUCCESS);
	return VK_TRUE;
}

VkBool32 CommandPool::ResetCommandBuffer (VkCommandBuffer cmdBuf)
{
	auto res = vkResetCommandBuffer (cmdBuf, {});
	assert (res == VK_SUCCESS);
	return VK_TRUE;
}

VkCommandBuffer CommandPool::AllocateCommandBuffer (VkCommandBufferLevel level)
{
	VkCommandBuffer buf;

	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo (command_pool, level, 1);

	auto res = vkAllocateCommandBuffers (device, &allocInfo, &buf);
	assert (res == VK_SUCCESS);
	return buf;
}

void CommandPool::BeginBufferRecording (VkCommandBuffer buf, VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo ();
	beginInfo.flags = flags;

	auto res = vkBeginCommandBuffer (buf, &beginInfo);
	assert (res == VK_SUCCESS);
}

void CommandPool::EndBufferRecording (VkCommandBuffer buf)
{
	auto res = vkEndCommandBuffer (buf);
	assert (res == VK_SUCCESS);
}

void CommandPool::FreeCommandBuffer (VkCommandBuffer buf)
{
	vkFreeCommandBuffers (device, command_pool, 1, &buf);
}

VkCommandBuffer CommandPool::GetCommandBuffer (VkCommandBufferLevel level, VkCommandBufferUsageFlags flags)
{
	VkCommandBuffer cmdBuffer = AllocateCommandBuffer (level);

	BeginBufferRecording (cmdBuffer, flags);

	return cmdBuffer;
}

VkBool32 CommandPool::ReturnCommandBuffer (VkCommandBuffer cmdBuffer,
    VulkanFence const& fence,
    std::vector<VkSemaphore> const& waitSemaphores,
    std::vector<VkSemaphore> const& signalSemaphores)
{
	EndBufferRecording (cmdBuffer);
	std::vector<VkCommandBuffer> bufs = { cmdBuffer };
	queue->SubmitCommandBuffers (bufs, fence, waitSemaphores, signalSemaphores, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	return VK_TRUE;
}

void CommandPool::WriteToBuffer (VkCommandBuffer buf, std::function<void(VkCommandBuffer)> const& cmds)
{
	cmds (buf);
}

///////// CommandBuffer /////////

CommandBuffer::CommandBuffer (CommandPool& pool, VkCommandBufferLevel level)
: pool (&pool), level (level), fence (pool.device)
{
}

CommandBuffer::~CommandBuffer ()
{
	if (cmdBuf != nullptr) pool->FreeCommandBuffer (cmdBuf);
}

CommandBuffer::CommandBuffer (CommandBuffer&& cmd)
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

CommandBuffer& CommandBuffer::Allocate ()
{
	assert (state == State::empty);
	cmdBuf = pool->AllocateCommandBuffer (level);
	state = State::allocated;
	return *this;
}

CommandBuffer& CommandBuffer::Begin (VkCommandBufferUsageFlags flags)
{
	assert (state == State::allocated);
	pool->BeginBufferRecording (cmdBuf, flags);
	state = State::recording;
	return *this;
}

CommandBuffer& CommandBuffer::WriteTo (std::function<void(const VkCommandBuffer)> const& work)
{
	assert (state == State::recording);
	pool->WriteToBuffer (cmdBuf, work);
	return *this;
}

CommandBuffer& CommandBuffer::End ()
{
	assert (state == State::recording);
	pool->EndBufferRecording (cmdBuf);
	state = State::ready;
	return *this;
}

CommandBuffer& CommandBuffer::Submit ()
{
	assert (state == State::ready);
	pool->queue->SubmitCommandBuffer (cmdBuf, fence);
	state = State::submitted;
	return *this;
}

CommandBuffer& CommandBuffer::Submit (std::vector<VkSemaphore> const& waits,
    std::vector<VkSemaphore> const& signals,
    VkPipelineStageFlags const stageMask)
{
	assert (state == State::ready);
	std::vector<VkCommandBuffer> bufs = { cmdBuf };
	pool->queue->SubmitCommandBuffers (bufs, fence, waits, signals, stageMask);
	state = State::submitted;
	return *this;
}

CommandBuffer& CommandBuffer::Wait ()
{
	// assert (state == State::submitted);
	fence.Wait ();
	fence.Reset ();
	state = State::allocated;
	return *this;
}

CommandBuffer& CommandBuffer::Free ()
{
	pool->FreeCommandBuffer (cmdBuf);
	cmdBuf = nullptr;
	state = State::empty;
	return *this;
}
