#pragma once

#include <vector>


#include <stdlib.h>  
#include <crtdbg.h>  

#include <memory>

#include <vulkan\vulkan.h>


#include <GLFW\glfw3.h>

#include "VulkanInitializers.hpp"
#include "VulkanTools.h"
#include "vulkanDevice.hpp"

class VulkanSwapChain {
public:
	VulkanSwapChain();

	void initSwapChain(std::shared_ptr<VulkanDevice> device, GLFWwindow* window);

	void recreateSwapChain(GLFWwindow* window);
	
	void CleanUp(VkImageView depthImageView, VkImage depthImage, VkDeviceMemory depthImageMemory, VkRenderPass renderPass);

	VkSurfaceKHR surface;

	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

private:

	std::shared_ptr<VulkanDevice> device; 
	VkInstance instance;
	VkPhysicalDevice physicalDevice;

	void createSwapChain(GLFWwindow* window);

	//7
	void createImageViews();

};