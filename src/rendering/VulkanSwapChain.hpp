#pragma once

#include <vector>


  
  

#include <memory>

#include <vulkan/vulkan.h>


#include <GLFW/glfw3.h>

#include "VulkanInitializers.hpp"
#include "VulkanTools.h"
#include "VulkanDevice.hpp"

class VulkanSwapChain {
public:
	VulkanSwapChain(const VulkanDevice& device);
	~VulkanSwapChain();

	void InitSwapChain(GLFWwindow* window);

	void RecreateSwapChain(GLFWwindow* window);
	
	void CreateFramebuffers(VkImageView depthImageView, VkRenderPass renderPass);

	void CleanUp();

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

	void createSwapChain(GLFWwindow* window);

	//7
	void createImageViews();

};