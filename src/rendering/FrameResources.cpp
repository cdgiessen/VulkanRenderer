#include "FrameResources.h"

#include "Device.h"
#include "Initializers.h"
#include "SwapChain.h"

FrameObject::FrameObject (VulkanDevice& device, VulkanSwapChain& swapChain, int frameIndex)
: device (device),
  swapchain (swapChain),
  frameIndex (frameIndex),
  imageAvailSem (device.device),
  renderFinishSem (device.device),
  commandFence (device.device, VK_FENCE_CREATE_SIGNALED_BIT),
  commandPool (device.device, device.GraphicsQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  primary_command_buffer (commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
{
	primary_command_buffer.Allocate ();
}

FrameObject::~FrameObject () { primary_command_buffer.Free (); }


VkResult FrameObject::AcquireNextSwapchainImage ()
{
	return vkAcquireNextImageKHR (
	    device.device, swapchain.Get (), std::numeric_limits<uint64_t>::max (), imageAvailSem.Get (), VK_NULL_HANDLE, &swapChainIndex);
}

void FrameObject::PrepareFrame ()
{
	primary_command_buffer.Wait ();
	primary_command_buffer.Begin (VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
}

void FrameObject::Submit ()
{
	primary_command_buffer.End ();

	std::vector<VkSemaphore> image_avail_sem;
	image_avail_sem.push_back (imageAvailSem.Get ());
	std::vector<VkSemaphore> render_finish_sem;
	render_finish_sem.push_back (renderFinishSem.Get ());

	primary_command_buffer.Submit (image_avail_sem, render_finish_sem, stageMasks);
}

VkResult FrameObject::Present ()
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = renderFinishSem.GetPtr ();
	presentInfo.pImageIndices = &swapChainIndex;

	VkSwapchainKHR swapChains[] = { swapchain.Get () };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	return device.PresentQueue ().PresentQueueSubmit (presentInfo);
}

VkCommandBuffer FrameObject::GetPrimaryCmdBuf () { return primary_command_buffer.Get (); }
