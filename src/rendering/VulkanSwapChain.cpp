#include "VulkanSwapChain.hpp"
#include "VulkanTools.h"

#include "../core/Logger.h"

#include <limits>

VulkanSwapChain::VulkanSwapChain(const VulkanDevice& device) : device(device) {

}

VulkanSwapChain::~VulkanSwapChain()
{
	Log::Debug << "swapchain deleted\n";
	//CleanUp();
}

void VulkanSwapChain::InitSwapChain(GLFWwindow* window) {

	this->instance = device.instance;
	this->physicalDevice = device.physical_device;
	this->window = window;

	createSwapChain();
	createImageViews();
}

void VulkanSwapChain::RecreateSwapChain(GLFWwindow* window) {
	CleanUp();
	this->window = window;

	createSwapChain();
	createImageViews();
}

void VulkanSwapChain::createSwapChain() {
	details = querySwapChainSupport(physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat();
	VkPresentModeKHR presentMode = chooseSwapPresentMode();
	VkExtent2D extent = chooseSwapExtent();

	uint32_t imageCount = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
		imageCount = details.capabilities.maxImageCount;
	}

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, swapChainImageFormat, &formatProps);
	if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) {
		createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	if (vkGetSwapchainImagesKHR(device.device, swapChain, &imageCount, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to get number of swapchain images!");
	}
	swapChainImages.resize(imageCount);

	if (vkGetSwapchainImagesKHR(device.device, swapChain, &imageCount, swapChainImages.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to get swapchain images!");
	}
}

//7
void VulkanSwapChain::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo viewInfo = initializers::imageViewCreateInfo();
		viewInfo.image = swapChainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = swapChainImageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImageView(device.device, &viewInfo, nullptr, &swapChainImageViews[i]));
	}
}

void VulkanSwapChain::CreateFramebuffers(VkImageView depthImageView, VkRenderPass renderPass) {
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo = initializers::framebufferCreateInfo();
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device.device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanSwapChain::CleanUp() {


	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(device.device, swapChainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vkDestroyImageView(device.device, swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device.device, swapChain, nullptr);
}

VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat() {
	if (details.formats.size() == 1 && details.formats[0].format == VK_FORMAT_UNDEFINED) {
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : details.formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return details.formats[0];
}

VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode() {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : details.present_modes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D VulkanSwapChain::chooseSwapExtent() {
	if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return details.capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(details.capabilities.minImageExtent.width, std::min(details.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(details.capabilities.minImageExtent.height, std::min(details.capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

SwapChainSupportDetails VulkanSwapChain::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {

	SwapChainSupportDetails details;

	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities) != VK_SUCCESS) {
		throw std::runtime_error("failed to get physical device surface capabilities!");
	}
	uint32_t formatCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to get physical device surface formats count!");
	}

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		if(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to get physical device surface formats!");
		}
	}

	uint32_t presentModeCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to get physical device surface present modes count!");
		}

	if (presentModeCount != 0) {
		details.present_modes.resize(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.present_modes.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to get physical device surface present modes!");
		}
	}

	return details;
}
