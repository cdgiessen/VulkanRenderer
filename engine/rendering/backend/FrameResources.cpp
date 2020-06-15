#include "FrameResources.h"

#include "Device.h"
#include "SwapChain.h"
#include "rendering/Initializers.h"

FrameObject::FrameObject (VulkanDevice& device, VulkanSwapChain& swapChain)
: device (&device),
  swapchain (&swapChain),
  imageAvailSem (device.device),
  renderFinishSem (device.device),
  commandPool (device.device, device.graphics_queue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  primary_command_buffer (commandPool)
{
	primary_command_buffer.allocate ();
}

FrameObject::~FrameObject () { primary_command_buffer.free (); }

FrameObject::FrameObject (FrameObject&& fb) noexcept
: device (fb.device),
  swapchain (fb.swapchain),
  swapChainIndex (fb.swapChainIndex),
  imageAvailSem (std::move (fb.imageAvailSem)),
  renderFinishSem (std::move (fb.renderFinishSem)),
  commandPool (std::move (fb.commandPool)),
  primary_command_buffer (std::move (fb.primary_command_buffer))
{
}
FrameObject& FrameObject::operator= (FrameObject&& fb) noexcept
{
	device = fb.device;
	swapchain = fb.swapchain;
	swapChainIndex = fb.swapChainIndex;
	imageAvailSem = std::move (fb.imageAvailSem);
	renderFinishSem = std::move (fb.renderFinishSem);
	commandPool = std::move (fb.commandPool);
	primary_command_buffer = std::move (fb.primary_command_buffer);
	return *this;
}


VkResult FrameObject::AcquireNextSwapchainImage ()
{
	return vkAcquireNextImageKHR (device->device,
	    swapchain->get (),
	    std::numeric_limits<uint64_t>::max (),
	    imageAvailSem.get (),
	    VK_NULL_HANDLE,
	    &swapChainIndex);
}

void FrameObject::PrepareFrame ()
{
	primary_command_buffer.wait ();
	primary_command_buffer.begin (VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
}

void FrameObject::submit ()
{
	primary_command_buffer.end ();

	std::vector<VkSemaphore> image_avail_sem;
	image_avail_sem.push_back (imageAvailSem.get ());
	std::vector<VkSemaphore> render_finish_sem;
	render_finish_sem.push_back (renderFinishSem.get ());

	primary_command_buffer.submit (image_avail_sem, render_finish_sem, stageMasks);
}

VkResult FrameObject::Present ()
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = renderFinishSem.get_ptr ();
	presentInfo.pImageIndices = &swapChainIndex;

	VkSwapchainKHR swapChains[] = { swapchain->get () };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	return device->present_queue ().present (presentInfo);
}

VkCommandBuffer FrameObject::GetPrimaryCmdBuf () { return primary_command_buffer.get (); }
