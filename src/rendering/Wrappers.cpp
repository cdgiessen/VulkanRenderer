#include "Wrappers.h"

#include "Initializers.h"

#include "Device.h"
#include "Buffer.h"

VulkanFence::VulkanFence(VulkanDevice& device)
	:device(device.device)
{
}

void VulkanFence::Create(
	long int timeout,
	VkFenceCreateFlags flags)
{
	this->timeout = timeout;
	VkFenceCreateInfo fenceInfo =
		initializers::fenceCreateInfo(flags);
	VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence))

}
void VulkanFence::CleanUp()
{
	vkDestroyFence(device, fence, nullptr);
}

bool VulkanFence::Check() {
	VkResult out = vkGetFenceStatus(device, fence);
	if (out == VK_SUCCESS)
		return true;
	else if (out == VK_NOT_READY)
		return false;
	throw std::runtime_error("DEVICE_LOST");
	return false;
}

void VulkanFence::WaitTillTrue() {
	vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
}
void VulkanFence::WaitTillFalse() {
	vkWaitForFences(device, 1, &fence, VK_FALSE, timeout);
}
VkFence VulkanFence::Get() {
	return fence;
}

void VulkanFence::Reset() {
	vkResetFences(device, 1, &fence);
}

std::vector<VkFence> CreateFenceArray(std::vector<VulkanFence>& fences)
{
	std::vector<VkFence> outFences(fences.size());
	for (auto& fence : fences)
		outFences.push_back(fence.Get());
	return outFences;
}

VulkanSemaphore::VulkanSemaphore(VulkanDevice& device)
	:device(&device) {
	VkSemaphoreCreateInfo semaphoreInfo = initializers::semaphoreCreateInfo();

	VK_CHECK_RESULT(vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &semaphore));

}

void VulkanSemaphore::CleanUp() {
	vkDestroySemaphore(device->device, semaphore, nullptr);
}

VkSemaphore VulkanSemaphore::Get() {
	return semaphore;
}

VkSemaphore* VulkanSemaphore::GetPtr() {
	return &semaphore;
}

std::vector<VkSemaphore> CreateSemaphoreArray(std::vector<VulkanSemaphore>& sems)
{
	std::vector<VkSemaphore> outSems;
	for (auto& sem : sems)
		outSems.push_back(sem.Get());

	return outSems;
}

//void CleanUp() {
//	vkDestroyFence(device.device, fence, nullptr);	//}	VkFence VulkanFence::Get() {return fence;};


CommandQueue::CommandQueue(const VulkanDevice& device, int queueFamily) :
	device(device)
{
	vkGetDeviceQueue(device.device, queueFamily, 0, &queue);
	this->queueFamily = queueFamily;
	//Log::Debug << "Queue on " << queueFamily << " type\n";
}

//void CommandQueue::SetupQueue(int queueFamily) {
//	vkGetDeviceQueue(device.device, queueFamily, 0, &queue);
//	this->queueFamily = queueFamily;
//}

void CommandQueue::Submit(VkSubmitInfo submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(submissionMutex);
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
}

void CommandQueue::SubmitCommandBuffer(VkCommandBuffer buffer, VkFence fence)
{
	const auto stageMasks = std::vector<VkPipelineStageFlags>{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	auto submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;
	submitInfo.pWaitDstStageMask = stageMasks.data();
	Submit(submitInfo, fence);
}

void CommandQueue::SubmitCommandBuffer(VkCommandBuffer cmdBuffer,
	VulkanFence& fence,
	std::vector<VulkanSemaphore>& waitSemaphores,
	std::vector<VulkanSemaphore>& signalSemaphores)
{
	auto waits = CreateSemaphoreArray(waitSemaphores);
	auto sigs = CreateSemaphoreArray(signalSemaphores);

	const auto stageMasks = std::vector<VkPipelineStageFlags>{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	auto submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.signalSemaphoreCount = (uint32_t)sigs.size();
	submitInfo.pSignalSemaphores = sigs.data();
	submitInfo.waitSemaphoreCount = (uint32_t)waits.size();
	submitInfo.pWaitSemaphores = waits.data();
	submitInfo.pWaitDstStageMask = stageMasks.data();
	Submit(submitInfo, fence.Get());
}


int CommandQueue::GetQueueFamily() {
	return queueFamily;
}

VkQueue CommandQueue::GetQueue() {
	return queue;
}

std::mutex& CommandQueue::GetQueueMutex() {
	return submissionMutex;
}

void CommandQueue::WaitForFences(VkFence fence) {
	vkWaitForFences(device.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
}

CommandPool::CommandPool(VulkanDevice& device,
	VkCommandPoolCreateFlags flags, CommandQueue* queue) :
	device(device)
{
	this->queue = queue;

	VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo();
	cmd_pool_info.queueFamilyIndex = queue->GetQueueFamily();
	cmd_pool_info.flags = flags;

	if (vkCreateCommandPool(device.device, &cmd_pool_info, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}

	//return VK_TRUE;
}

// VkBool32 CommandPool::Setup(VkCommandPoolCreateFlags flags, CommandQueue* queue)
// {
// 	this->queue = queue;

// 	VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo();
// 	cmd_pool_info.queueFamilyIndex = queue->GetQueueFamily();
// 	cmd_pool_info.flags = flags;

// 	if (vkCreateCommandPool(device.device, &cmd_pool_info, nullptr, &commandPool) != VK_SUCCESS) {
// 		throw std::runtime_error("failed to create graphics command pool!");
// 	}

// 	return VK_TRUE;
// }

VkBool32 CommandPool::CleanUp() {
	std::lock_guard<std::mutex> lock(poolLock);
	vkDestroyCommandPool(device.device, commandPool, nullptr);
	return VK_TRUE;
}

VkBool32 CommandPool::ResetPool() {

	std::lock_guard<std::mutex> lock(poolLock);
	vkResetCommandPool(device.device, commandPool, 0);
	return VK_TRUE;
}

VkBool32 CommandPool::ResetCommandBuffer(VkCommandBuffer cmdBuf) {
	std::lock_guard<std::mutex> lock(poolLock);
	vkResetCommandBuffer(cmdBuf, {});
	return VK_TRUE;
}

VkCommandBuffer CommandPool::AllocateCommandBuffer(VkCommandBufferLevel level)
{
	VkCommandBuffer buf;

	VkCommandBufferAllocateInfo allocInfo =
		initializers::commandBufferAllocateInfo(commandPool, level, 1);

	std::lock_guard<std::mutex> lock(poolLock);
	vkAllocateCommandBuffers(device.device, &allocInfo, &buf);

	return buf;
}

void CommandPool::BeginBufferRecording(VkCommandBuffer buf, VkCommandBufferUsageFlagBits flags) {

	VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
	beginInfo.flags = flags;

	vkBeginCommandBuffer(buf, &beginInfo);
}

void CommandPool::EndBufferRecording(VkCommandBuffer buf) {
	vkEndCommandBuffer(buf);
}

void CommandPool::FreeCommandBuffer(VkCommandBuffer buf) {

	std::lock_guard<std::mutex> lock(poolLock);
	vkFreeCommandBuffers(device.device, commandPool, 1, &buf);

}

VkCommandBuffer CommandPool::GetOneTimeUseCommandBuffer() {
	VkCommandBuffer cmdBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	BeginBufferRecording(cmdBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	return cmdBuffer;
}

VkCommandBuffer CommandPool::GetPrimaryCommandBuffer(bool beginBufferRecording) {

	VkCommandBuffer cmdBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	if (beginBufferRecording == true)
		BeginBufferRecording(cmdBuffer);

	return cmdBuffer;
}

VkCommandBuffer CommandPool::GetSecondaryCommandBuffer(bool beginBufferRecording) {
	VkCommandBuffer cmdBuffer = AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	if (beginBufferRecording == true)
		BeginBufferRecording(cmdBuffer);

	return cmdBuffer;
}

// VkBool32 CommandPool::SubmitCommandBuffer(VkCommandBuffer cmdBuffer, VulkanFence& fence,
// 	std::vector<VkSemaphore>& waitSemaphores, std::vector<VkSemaphore>& signalSemaphores)
// {
// 	EndBufferRecording(cmdBuffer);
// 	queue->SubmitCommandBuffer(cmdBuffer, fence, waitSemaphores, signalSemaphores);

// 	return VK_TRUE;
// }

VkBool32 CommandPool::SubmitCommandBuffer(VkCommandBuffer cmdBuffer, VulkanFence& fence,
	std::vector<VulkanSemaphore>& waitSemaphores,
	std::vector<VulkanSemaphore>& signalSemaphores)
{
	EndBufferRecording(cmdBuffer);
	queue->SubmitCommandBuffer(cmdBuffer, fence, waitSemaphores, signalSemaphores);

	return VK_TRUE;
}


VkBool32 CommandPool::SubmitOneTimeUseCommandBuffer(VkCommandBuffer cmdBuffer, VkFence fence) {
	EndBufferRecording(cmdBuffer);

	if (fence == nullptr) {
		VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))
	}

	queue->SubmitCommandBuffer(cmdBuffer, fence);

	return VK_TRUE;
}

VkBool32 CommandPool::SubmitPrimaryCommandBuffer(VkCommandBuffer cmdBuffer, VkFence fence) {
	EndBufferRecording(cmdBuffer);

	if (fence == nullptr) {
		VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))
	}

	queue->SubmitCommandBuffer(cmdBuffer, fence);

	return VK_TRUE;
}


void CommandPool::WriteToBuffer(VkCommandBuffer buf, std::function<void(VkCommandBuffer)> cmds)
{
	std::lock_guard<std::mutex> lock(poolLock);
	cmds(buf);
}

// CommandBuffer::CommandBuffer(VulkanDevice& device, CommandPool& pool) :
// 	fence(device), pool(pool)
// {
// }

// void CommandBuffer::Create() {
// 	buf = pool.GetOneTimeUseCommandBuffer();
// }

// void CommandBuffer::CleanUp() {
// 	pool.FreeCommandBuffer(buf);
// 	for (auto& sem : waitSemaphores) {
// 		sem.CleanUp();
// 	}
// 	for (auto& sem : waitSemaphores) {
// 		sem.CleanUp();
// 	}
// }

// void CommandBuffer::Begin() {
// 	pool.BeginBufferRecording(buf);
// }
// void CommandBuffer::End() {
// 	pool.EndBufferRecording(buf);
// }
// void CommandBuffer::Submit() {
// 	pool.SubmitCommandBuffer(buf, fence, waitSemaphores, signalSemaphores);
// }

// void CommandBuffer::Write(std::function<void(VkCommandBuffer)> cmds) {
// 	pool.WriteToBuffer(buf, cmds);
// }

// void CommandBuffer::AddSynchronization(std::vector<VulkanSemaphore> waitSemaphores,
// 	std::vector<VulkanSemaphore> signalSemaphores) {
// 	this->waitSemaphores = waitSemaphores;
// 	this->signalSemaphores = signalSemaphores;
// }

// bool CommandBuffer::CheckFence() {
// 	return fence.Check();
// }



GraphicsCommandWorker::GraphicsCommandWorker(
	VulkanDevice &device,
	ConcurrentQueue<GraphicsWork>& workQueue,
	std::vector<GraphicsCleanUpWork>& finishQueue,
	std::mutex& finishQueueLock,
	bool startActive) :

	device(device),
	graphicsPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &device.GraphicsQueue()),
	transferPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &device.TransferQueue()),
	computePool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &device.ComputeQueue()),
	workQueue(workQueue), finishQueue(finishQueue), finishQueueLock(finishQueueLock)
{
	workingThread = std::thread{ &GraphicsCommandWorker::Work, this };

}

GraphicsCommandWorker::~GraphicsCommandWorker() {
	//workingThread.join();
}

void GraphicsCommandWorker::CleanUp() {
	StopWork();
	workingThread.join();

	graphicsPool.CleanUp();
	transferPool.CleanUp();
	computePool.CleanUp();
}

void GraphicsCommandWorker::StopWork() {
	keepWorking = false;
}

void GraphicsCommandWorker::Work() {
	while (keepWorking)
	{

		workQueue.wait_on_value();

		while (!workQueue.empty()) {
			auto pos_work = workQueue.pop_if();
			if (pos_work.has_value()) {


				VkCommandBuffer cmdBuf;
				CommandPool* pool;

				switch (pos_work->type) {
				case(CommandPoolType::graphics):
					pool = &graphicsPool;
					break;
				case(CommandPoolType::transfer):
					pool = &transferPool;
					break;
				case(CommandPoolType::compute):
					pool = &computePool;
					break;
				}
				pos_work->fence.Create();

				cmdBuf = pool->GetOneTimeUseCommandBuffer();
				//pool->BeginBufferRecording(cmdBuf);
				pos_work->work(cmdBuf);
				//pool->EndBufferRecording(cmdBuf);
				pool->SubmitCommandBuffer(cmdBuf, pos_work->fence,
					pos_work->waitSemaphores, pos_work->signalSemaphores);

				//GraphicsCleanUpWork cleanUpWork{ std::move(pos_work->fence),
				//	pool, cmdBuf, std::move(pos_work->buffersToClean),
				//	std::move(pos_work->signals) };
				{
					std::lock_guard<std::mutex>lk(finishQueueLock);
					finishQueue.push_back(std::move(GraphicsCleanUpWork(std::move(*pos_work), pool, cmdBuf)));
				}
			}
		}
	}
}


FrameObject::FrameObject(VulkanDevice& device, int frameIndex) :
	device(device),
	frameIndex(frameIndex),
	commandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		&device.GraphicsQueue()),
	imageAvailSem(device),
	renderFinishSem(device),
	commandFence(device)
{
	primaryCmdBuf = commandPool.GetPrimaryCommandBuffer(false);
	commandFence.Create();
}

FrameObject::~FrameObject() {
	commandPool.FreeCommandBuffer(primaryCmdBuf);
	commandPool.CleanUp();
	imageAvailSem.CleanUp();
	renderFinishSem.CleanUp();
	commandFence.CleanUp();
}

VkResult FrameObject::AquireNextSwapchainImage(VkSwapchainKHR swapchain) {
	return vkAcquireNextImageKHR(
		device.device, swapchain,
		std::numeric_limits<uint64_t>::max(), imageAvailSem.Get(),
		VK_NULL_HANDLE, &swapChainIndex);
}

void FrameObject::PrepareFrame() {
	if (!firstUse) {
		commandFence.WaitTillTrue();
		commandFence.Reset();
	}
	else {
		firstUse = false;
	}
	commandPool.BeginBufferRecording(primaryCmdBuf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

}

void FrameObject::SubmitFrame()
{
	commandPool.EndBufferRecording(primaryCmdBuf);

	auto submitInfo = GetSubmitInfo();

}

VkSubmitInfo FrameObject::GetSubmitInfo() {

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = renderFinishSem.GetPtr();
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = imageAvailSem.GetPtr();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &primaryCmdBuf;

	return submitInfo;
}

VkPresentInfoKHR FrameObject::GetPresentInfo() {
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = renderFinishSem.GetPtr();

	presentInfo.pImageIndices = &swapChainIndex;
	return presentInfo;
}


VkCommandBuffer FrameObject::GetPrimaryCmdBuf() {
	return primaryCmdBuf;
}

VkFence FrameObject::GetCommandFence() {
	return commandFence.Get();
}