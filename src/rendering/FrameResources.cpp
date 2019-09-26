#include "FrameResources.h"

#include "Initializers.h"

FrameObject::FrameObject (VulkanDevice& device, int frameIndex)
: device (device),
  frameIndex (frameIndex),
  imageAvailSem (device),
  renderFinishSem (device),
  commandFence (std::make_shared<VulkanFence> (device, DEFAULT_FENCE_TIMEOUT, VK_FENCE_CREATE_SIGNALED_BIT)),
  commandPool (device, device.GraphicsQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  primary_command_buffer (commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
{
	primary_command_buffer.Allocate ();
	primary_command_buffer.SetFence (commandFence);
}

FrameObject::~FrameObject () { primary_command_buffer.Free (); }


VkResult FrameObject::AcquireNextSwapchainImage (VkSwapchainKHR swapchain)
{
	return vkAcquireNextImageKHR (
	    device.device, swapchain, std::numeric_limits<uint64_t>::max (), imageAvailSem.Get (), VK_NULL_HANDLE, &swapChainIndex);
}

void FrameObject::PrepareFrame ()
{
	primary_command_buffer.Wait ();
	primary_command_buffer.Begin (VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
}

void FrameObject::Submit (CommandQueue& queue)
{
	primary_command_buffer.End ();

	VkSubmitInfo submitInfo = initializers::submitInfo ();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = renderFinishSem.GetPtr ();
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = imageAvailSem.GetPtr ();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = primary_command_buffer.GetPtr ();
	submitInfo.pWaitDstStageMask = &stageMasks;

	queue.Submit (submitInfo, *commandFence.get ());
}

VkResult FrameObject::Present (VulkanSwapChain& swapChain, CommandQueue& presentQueue)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = renderFinishSem.GetPtr ();
	presentInfo.pImageIndices = &swapChainIndex;

	VkSwapchainKHR swapChains[] = { swapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	return presentQueue.PresentQueueSubmit (presentInfo);
}

VkCommandBuffer FrameObject::GetPrimaryCmdBuf () { return primary_command_buffer.Get (); }
