#include "Wrappers.h"

#include "Initializers.h"

#include "Buffer.h"
#include "Device.h"
#include "RenderTools.h"
#include "SwapChain.h"

//////// Fence ////////////

VulkanFence::VulkanFence (VulkanDevice& device, long int timeout, VkFenceCreateFlags flags)
: device (device), timeout (timeout)
{
	this->timeout = timeout;
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo (flags);
	VK_CHECK_RESULT (vkCreateFence (device.device, &fenceInfo, nullptr, &fence))
}

VulkanFence::~VulkanFence () { vkDestroyFence (device.device, fence, nullptr); }

bool VulkanFence::Check ()
{
	VkResult out = vkGetFenceStatus (device.device, fence);
	if (out == VK_SUCCESS)
		return true;
	else if (out == VK_NOT_READY)
		return false;
	assert (out == VK_SUCCESS || out == VK_NOT_READY);
	return false;
}

void VulkanFence::Wait (bool condition)
{
	vkWaitForFences (device.device, 1, &fence, condition, timeout);
}

VkFence VulkanFence::Get () { return fence; }

void VulkanFence::Reset () { vkResetFences (device.device, 1, &fence); }

std::vector<VkFence> CreateFenceArray (std::vector<VulkanFence>& fences)
{
	std::vector<VkFence> outFences (fences.size ());
	for (auto& fence : fences)
		outFences.push_back (fence.Get ());
	return outFences;
}

///////////// Semaphore ///////////////

VulkanSemaphore::VulkanSemaphore (VulkanDevice& device) : device (device)
{
	VkSemaphoreCreateInfo semaphoreInfo = initializers::semaphoreCreateInfo ();

	VK_CHECK_RESULT (vkCreateSemaphore (device.device, &semaphoreInfo, nullptr, &semaphore));
}

VulkanSemaphore::~VulkanSemaphore () { vkDestroySemaphore (device.device, semaphore, nullptr); }

VkSemaphore VulkanSemaphore::Get () { return semaphore; }

VkSemaphore* VulkanSemaphore::GetPtr () { return &semaphore; }

std::vector<VkSemaphore> CreateSemaphoreArray (std::vector<std::shared_ptr<VulkanSemaphore>>& sems)
{
	std::vector<VkSemaphore> outSems;
	for (auto& sem : sems)
		outSems.push_back (sem->Get ());

	return outSems;
}

///////////// Command Queue ////////////////

CommandQueue::CommandQueue (const VulkanDevice& device, int queueFamily) : device (device)
{
	vkGetDeviceQueue (device.device, queueFamily, 0, &queue);
	this->queueFamily = queueFamily;
}

void CommandQueue::SubmitCommandBuffer (VkCommandBuffer buffer, VulkanFence& fence)
{
	auto waits = std::vector<std::shared_ptr<VulkanSemaphore>> ();
	auto signals = std::vector<std::shared_ptr<VulkanSemaphore>> ();

	SubmitCommandBuffer (buffer, fence, waits, signals);
}

void CommandQueue::SubmitCommandBuffer (VkCommandBuffer cmdBuffer,
    VulkanFence& fence,
    std::vector<std::shared_ptr<VulkanSemaphore>>& waitSemaphores,
    std::vector<std::shared_ptr<VulkanSemaphore>>& signalSemaphores)
{
	auto waits = CreateSemaphoreArray (waitSemaphores);
	auto sigs = CreateSemaphoreArray (signalSemaphores);

	const auto stageMasks = std::vector<VkPipelineStageFlags>{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	auto submitInfo = initializers::submitInfo ();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.signalSemaphoreCount = (uint32_t)sigs.size ();
	submitInfo.pSignalSemaphores = sigs.data ();
	submitInfo.waitSemaphoreCount = (uint32_t)waits.size ();
	submitInfo.pWaitSemaphores = waits.data ();
	submitInfo.pWaitDstStageMask = stageMasks.data ();

	Submit (submitInfo, fence);
}

void CommandQueue::Submit (VkSubmitInfo& submitInfo, VulkanFence& fence)
{
	std::lock_guard lock (submissionMutex);
	VK_CHECK_RESULT (vkQueueSubmit (queue, 1, &submitInfo, fence.Get ()));
}


int CommandQueue::GetQueueFamily () { return queueFamily; }

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

CommandPool::CommandPool (VulkanDevice& device, CommandQueue& queue, VkCommandPoolCreateFlags flags)
: device (&device), queue (&queue)
{
	VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo ();
	cmd_pool_info.queueFamilyIndex = queue.GetQueueFamily ();
	cmd_pool_info.flags = flags;

	auto res = vkCreateCommandPool (device.device, &cmd_pool_info, nullptr, &command_pool);
	assert (res == VK_SUCCESS);
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

CommandPool::~CommandPool ()
{
	if (command_pool != nullptr) vkDestroyCommandPool (device->device, command_pool, nullptr);
}

VkBool32 CommandPool::ResetPool ()
{
	auto res = vkResetCommandPool (device->device, command_pool, 0);
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

	auto res = vkAllocateCommandBuffers (device->device, &allocInfo, &buf);
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
	vkFreeCommandBuffers (device->device, command_pool, 1, &buf);
}

VkCommandBuffer CommandPool::GetCommandBuffer (VkCommandBufferLevel level, VkCommandBufferUsageFlags flags)
{
	VkCommandBuffer cmdBuffer = AllocateCommandBuffer (level);

	BeginBufferRecording (cmdBuffer, flags);

	return cmdBuffer;
}

VkBool32 CommandPool::ReturnCommandBuffer (VkCommandBuffer cmdBuffer,
    VulkanFence& fence,
    std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores,
    std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores)
{
	EndBufferRecording (cmdBuffer);
	queue->SubmitCommandBuffer (cmdBuffer, fence, waitSemaphores, signalSemaphores);

	return VK_TRUE;
}

void CommandPool::SubmitCommandBuffer (VkCommandBuffer cmdBuffer,
    VulkanFence& fence,
    std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores,
    std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores)
{
	queue->SubmitCommandBuffer (cmdBuffer, fence, waitSemaphores, signalSemaphores);
}

void CommandPool::WriteToBuffer (VkCommandBuffer buf, std::function<void(VkCommandBuffer)> cmds)
{
	cmds (buf);
}

///////// CommandBuffer /////////

CommandBuffer::CommandBuffer (CommandPool& pool, VkCommandBufferLevel level)
: pool (&pool), level (level)
{
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

CommandBuffer& CommandBuffer::WriteTo (std::function<void(const VkCommandBuffer)> work)
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

CommandBuffer& CommandBuffer::SetFence (std::shared_ptr<VulkanFence> fence)
{
	this->fence = fence;
	return *this;
}

CommandBuffer& CommandBuffer::Submit ()
{
	assert (state == State::ready);
	pool->SubmitCommandBuffer (cmdBuf, *fence.get ());
	state = State::submitted;
	return *this;
}

CommandBuffer& CommandBuffer::Submit (std::vector<std::shared_ptr<VulkanSemaphore>>& wats,
    std::vector<std::shared_ptr<VulkanSemaphore>>& signals)
{
	assert (state == State::ready);
	pool->SubmitCommandBuffer (cmdBuf, *fence.get (), wats, signals);
	state = State::submitted;
	return *this;
}

CommandBuffer& CommandBuffer::Wait ()
{
	// assert (state == State::submitted);
	fence->Wait ();
	fence->Reset ();
	state = State::allocated;
	return *this;
}

CommandBuffer& CommandBuffer::Free ()
{
	pool->FreeCommandBuffer (cmdBuf);
	state = State::empty;
	return *this;
}
