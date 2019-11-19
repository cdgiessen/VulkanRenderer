#include "SwapChain.h"

#include <limits>

#include "core/Logger.h"
#include "core/Window.h"

#include "Device.h"
#include "Initializers.h"
#include "RenderTools.h"


VulkanSwapChain::VulkanSwapChain (VulkanDevice& device, Window& window)
: device (device), window (window)
{
	CreateSwapChain ();
}

VulkanSwapChain::~VulkanSwapChain () { DestroySwapchainResources (); }

void VulkanSwapChain::RecreateSwapChain ()
{
	DestroySwapchainResources ();

	CreateSwapChain ();
}

void VulkanSwapChain::CreateSwapChain ()
{
	details = querySwapChainSupport (device.physical_device.physical_device, device.GetSurface ());

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat ();
	VkPresentModeKHR presentMode = chooseSwapPresentMode ();
	VkExtent2D extent = chooseSwapExtent ();

	image_count = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount)
	{
		image_count = details.capabilities.maxImageCount;
	}

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = device.GetSurface ();

	createInfo.minImageCount = image_count;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties (device.physical_device.physical_device, swapChainImageFormat, &formatProps);
	if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)
	{
		createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	QueueFamilyIndices indices = device.GetFamilyIndices ();
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR (device.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create swap chain!");
	}

	if (vkGetSwapchainImagesKHR (device.device, swapChain, &image_count, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to get number of swapchain images!");
	}
	swapChainImages.resize (image_count);

	if (vkGetSwapchainImagesKHR (device.device, swapChain, &image_count, swapChainImages.data ()) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to get swapchain images!");
	}

	swapChainImageViews.resize (swapChainImages.size ());

	for (uint32_t i = 0; i < swapChainImages.size (); i++)
	{
		VkImageViewCreateInfo viewInfo = initializers::imageViewCreateInfo ();
		viewInfo.image = swapChainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = swapChainImageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT (vkCreateImageView (device.device, &viewInfo, nullptr, &swapChainImageViews[i]));
	}
}

void VulkanSwapChain::DestroySwapchainResources ()
{

	for (size_t i = 0; i < swapChainImageViews.size (); i++)
	{
		vkDestroyImageView (device.device, swapChainImageViews[i], nullptr);
	}
	swapChainImageViews.clear ();

	if (swapChain != nullptr) vkDestroySwapchainKHR (device.device, swapChain, nullptr);
	swapChain = nullptr;
}

VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat ()
{
	if (details.formats.size () == 1 && details.formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : details.formats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return details.formats[0];
}

VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode ()
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : details.present_modes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D VulkanSwapChain::chooseSwapExtent ()
{
	if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max ())
	{
		return details.capabilities.currentExtent;
	}
	else
	{
		auto size = window.GetWindowSize ();

		VkExtent2D actualExtent = { static_cast<uint32_t> (size.x), static_cast<uint32_t> (size.y) };

		actualExtent.width = std::max (details.capabilities.minImageExtent.width,
		    std::min (details.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max (details.capabilities.minImageExtent.height,
		    std::min (details.capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

SwapChainSupportDetails VulkanSwapChain::querySwapChainSupport (VkPhysicalDevice device, VkSurfaceKHR surface)
{

	SwapChainSupportDetails details;

	VK_CHECK_RESULT (vkGetPhysicalDeviceSurfaceCapabilitiesKHR (device, surface, &details.capabilities));
	uint32_t formatCount;

	VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR (device, surface, &formatCount, nullptr);
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error ("Failed to get device surface formats");
	}
	assert (res == VK_SUCCESS);

	if (formatCount != 0)
	{
		details.formats.resize (formatCount);
		VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR (
		    device, surface, &formatCount, details.formats.data ());
		assert (res == VK_SUCCESS);
	}

	uint32_t presentModeCount;
	VK_CHECK_RESULT (vkGetPhysicalDeviceSurfacePresentModesKHR (device, surface, &presentModeCount, nullptr));

	if (presentModeCount != 0)
	{
		details.present_modes.resize (presentModeCount);
		VK_CHECK_RESULT (vkGetPhysicalDeviceSurfacePresentModesKHR (
		    device, surface, &presentModeCount, details.present_modes.data ()));
	}

	return details;
}
