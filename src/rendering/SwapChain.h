#pragma once

#include <vector>
#include <set>
#include <string>
#include <memory>

#include <vulkan/vulkan.h>

#include "Initializers.h"
#include "Device.h"

class Window;

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class VulkanSwapChain {
public:
	VulkanSwapChain(VulkanDevice& device, Window& window);
	~VulkanSwapChain();

	//void InitSwapChain(Window* window);

	void RecreateSwapChain();

	void CreateFramebuffers(VkImageView depthImageView, VkRenderPass renderPass);

	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	// Active frame buffer index -- not likely needed (as having multiple in flight is useful)
	//uint32_t currentBuffer = 0;

private:

	VulkanDevice & device;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;

	Window& window;

	SwapChainSupportDetails details;

	void createSwapChain();
	void createImageViews();

	void DestroySwapchainResources();

	VkPresentModeKHR chooseSwapPresentMode();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat();
	VkExtent2D chooseSwapExtent();
};
