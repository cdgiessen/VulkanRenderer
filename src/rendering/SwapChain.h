#pragma once

#include <vector>
#include <set>
#include <string>
#include <memory>

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "Initializers.h"
#include "Device.h"

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class VulkanSwapChain {
public:
	VulkanSwapChain(const VulkanDevice& device);
	~VulkanSwapChain();

	void InitSwapChain(GLFWwindow* window);

	void RecreateSwapChain(GLFWwindow* window);
	
	void CreateFramebuffers(VkImageView depthImageView, VkRenderPass renderPass);

	void CleanUp();

	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkSurfaceKHR surface;

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	// Active frame buffer index
	uint32_t currentBuffer = 0;

private:

	const VulkanDevice& device; 
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	GLFWwindow *window;

	SwapChainSupportDetails details;

	void createSwapChain();

	//7
	void createImageViews();

	VkPresentModeKHR chooseSwapPresentMode();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat();
	VkExtent2D chooseSwapExtent();
};
