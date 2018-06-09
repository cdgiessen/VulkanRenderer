#include "Wrappers.h"

#include "Initializers.h"

#include "Device.h"
#include "Buffer.h"

VulkanFence::VulkanFence(const VulkanDevice& device,
	long int timeout,
	VkFenceCreateFlags flags)
	:device(device), timeout(timeout)
{
	VkFenceCreateInfo fenceInfo =
		initializers::fenceCreateInfo(flags);
	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

}
void VulkanFence::CleanUp()
{
	vkDestroyFence(device.device, fence, nullptr);
}

void VulkanFence::WaitTillTrue() {
	vkWaitForFences(device.device, 1, &fence, VK_TRUE, timeout);
}
void VulkanFence::WaitTillFalse() {
	vkWaitForFences(device.device, 1, &fence, VK_FALSE, timeout);
}
VkFence VulkanFence::Get() {
	return fence;
}

std::vector<VkFence> CreateFenceArray(std::vector<VulkanFence>& fences)
{
	std::vector<VkFence> outFences(fences.size());
	for(auto& fence : fences)
		outFences.push_back(fence.Get());
	return outFences;
}

VulkanSemaphore::VulkanSemaphore(const VulkanDevice& device) {
	VkSemaphoreCreateInfo semaphoreInfo = initializers::semaphoreCreateInfo();

	VK_CHECK_RESULT(vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &semaphore));

}

void VulkanSemaphore::CleanUp(const VulkanDevice & device) {
	vkDestroySemaphore(device.device, semaphore, nullptr);
}

VkSemaphore VulkanSemaphore::Get() {
	return semaphore;
}

std::vector<VkSemaphore> CreateSemaphoreArray(std::vector<VulkanSemaphore>& sems)
{
	std::vector<VkSemaphore> outSems;
	for(auto& sem : sems)
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

CommandPool::CommandPool(VulkanDevice& device) :
	device(device)
{

}

VkBool32 CommandPool::Setup(VkCommandPoolCreateFlags flags, CommandQueue* queue)
{
	this->queue = queue;

	VkCommandPoolCreateInfo cmd_pool_info = initializers::commandPoolCreateInfo();
	cmd_pool_info.queueFamilyIndex = queue->GetQueueFamily();
	cmd_pool_info.flags = flags;

	if (vkCreateCommandPool(device.device, &cmd_pool_info, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}

	return VK_TRUE;
}

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
	{
		std::lock_guard<std::mutex> lock(poolLock);
		vkFreeCommandBuffers(device.device, commandPool, 1, &buf);
	}
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

template <> void CommandBufferWorker<CommandBufferWork>::Work() {

	while (keepWorking)
	{
		{
			std::unique_lock<std::mutex> uniqueLock(workQueue.lock);
			workQueue.condVar.wait(uniqueLock);
		}

		auto pos_work = workQueue.GetWork();
		while (pos_work.has_value())
		{
			VkCommandBuffer buf = pool.GetOneTimeUseCommandBuffer();

			pos_work->work(buf);

			VulkanFence fence(device);

			//std::vector<VkSemaphore> waits;
			//for (auto& w : pos_work->waitSemaphores)
			//	waits.push_back(w.Get());
			//std::vector<VkSemaphore> signals;
			//for (auto& s : pos_work->signalSemaphores)
			//	signals.push_back(s.Get());

			pool.SubmitCommandBuffer(buf, fence, pos_work->waitSemaphores, pos_work->signalSemaphores);

			fence.WaitTillTrue();

			for (auto& flag : pos_work->flags)
				*flag = true;
			for (auto& sem : pos_work->waitSemaphores)
				sem.CleanUp(device);

			pool.FreeCommandBuffer(buf);

			fence.CleanUp();

			pos_work = workQueue.GetWork();
		}
	}
}

template <> void CommandBufferWorker<TransferCommandWork>::Work() {
	while (keepWorking)
	{
		{
			std::unique_lock<std::mutex> uniqueLock(workQueue.lock);
			workQueue.condVar.wait(uniqueLock);
		}

		auto pos_work = workQueue.GetWork();
		while (pos_work.has_value())
		{
			VkCommandBuffer buf = pool.GetOneTimeUseCommandBuffer();

			pos_work->work(buf);

			VulkanFence fence(device);

			//std::vector<VkSemaphore> waits;
			//for (auto& w : pos_work->waitSemaphores)
			//	waits.push_back(w.Get());
			//std::vector<VkSemaphore> signals;
			//for (auto& s : pos_work->signalSemaphores)
			//	signals.push_back(s.Get());

			pool.SubmitCommandBuffer(buf, fence, pos_work->waitSemaphores, pos_work->signalSemaphores);

			fence.WaitTillTrue();

			for (auto& flag : pos_work->flags)
				*flag = true;

			for (auto& sem : pos_work->waitSemaphores)
				sem.CleanUp(device);

			for (auto& buffer : pos_work->buffersToClean)
				buffer.CleanBuffer();


			fence.CleanUp();

			pool.FreeCommandBuffer(buf);

			pos_work = workQueue.GetWork();
		}
	}
}
