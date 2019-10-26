#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

class VulkanDevice;
class Window;

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class VulkanSwapChain
{
	public:
	VulkanSwapChain (VulkanDevice& device, Window& window);
	VulkanSwapChain (VulkanSwapChain const& chain) = delete;
	VulkanSwapChain& operator= (VulkanSwapChain const& chain) = delete;
	VulkanSwapChain (VulkanSwapChain&& chain) = delete;
	VulkanSwapChain& operator= (VulkanSwapChain&& chain) = delete;

	~VulkanSwapChain ();

	void RecreateSwapChain ();

	VkSwapchainKHR Get () { return swapChain; }

	VkFormat GetFormat () { return swapChainImageFormat; }

	uint32_t GetChainCount () { return image_count; }

	VkExtent2D GetImageExtent () { return swapChainExtent; }

	VkImageView GetSwapChainImageView (int i) { return swapChainImageViews.at (i); }

	static SwapChainSupportDetails querySwapChainSupport (VkPhysicalDevice device, VkSurfaceKHR surface);

	private:
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	private:
	VulkanDevice& device;

	Window& window;

	uint32_t image_count = 0;

	SwapChainSupportDetails details;

	void CreateSwapChain ();
	void CreateImageViews ();

	void DestroySwapchainResources ();

	VkPresentModeKHR chooseSwapPresentMode ();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat ();
	VkExtent2D chooseSwapExtent ();
};
