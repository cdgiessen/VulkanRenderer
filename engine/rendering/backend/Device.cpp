#include "Device.h"

#include <string>

#include <GLFW/glfw3.h>

#include "core/Logger.h"
#include "core/Window.h"

#include "RenderTools.h"

//////// VMA Memory Resource ////////

bool VMA_MemoryResource::Create (
    VkPhysicalDevice physical_device, VkDevice device, VkAllocationCallbacks* custom_allocator)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physical_device;
	allocatorInfo.device = device;
	allocatorInfo.pAllocationCallbacks = custom_allocator;

	VkResult res = vmaCreateAllocator (&allocatorInfo, &allocator);
	if (res != VK_SUCCESS)
	{
		Log.Error (fmt::format ("Failed to create glfw window{}", errorString (res)));
		return false;
	}
	return true;
}
void VMA_MemoryResource::Free ()
{
	if (allocator)
	{
		vmaDestroyAllocator (allocator);
	}
}

void VMA_MemoryResource::LogVMA (bool detailedOutput) const
{
	char* str;
	vmaBuildStatsString (allocator, &str, detailedOutput);
	std::string out_str (str);
	vmaFreeStatsString (allocator, str);

	Log.Debug (fmt::format ("Allocator Data Dump:\n {}", out_str));
}

//////// DebugCallback ////////

static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
	auto ms = vkb::to_string_message_severity (messageSeverity);
	auto mt = vkb::to_string_message_type (messageType);
	Log.Error (fmt::format ("{} {}:\n{}", ms, mt, pCallbackData->pMessage));
	return VK_FALSE;
}

//////// VulkanDevice ////////

VkPhysicalDeviceFeatures QueryDeviceFeatures ();


VulkanDevice::VulkanDevice (Window& window, bool validationLayers)
: enableValidationLayers (validationLayers), window (&window)
{
	vkb::InstanceBuilder inst_builder;
	auto inst_ret = inst_builder.request_validation_layers (validationLayers)
	                    .set_app_name ("VulkanRenderer")
	                    .set_debug_callback (debugUtilsCallback)
	                    .build ();
	if (!inst_ret)
	{
		// TODO
		Log.Error (fmt::format ("Failed to create instance: {}", vkb::to_string (inst_ret.error ().type)));
	}
	vkb_instance = inst_ret.value ();

	bool surf_ret = CreateSurface (vkb_instance.instance, window);
	if (!surf_ret)
	{
		Log.Error (fmt::format ("Failed to create surface"));
	}

	vkb::PhysicalDeviceSelector selector (vkb_instance);
	selector.set_required_features (QueryDeviceFeatures ());
	auto phys_ret = selector.set_surface (surface).select ();
	if (!phys_ret)
	{
		// TODO
		Log.Error (fmt::format (
		    "Failed to select physical device: {}", vkb::to_string (phys_ret.error ().type)));
	}
	phys_device = phys_ret.value ();
	vkb::DeviceBuilder dev_builder (phys_device);
	auto dev_ret = dev_builder.build ();
	if (!dev_ret)
	{
		// TODO
		Log.Error (fmt::format ("Failed to create device: {}", vkb::to_string (dev_ret.error ().type)));
	}
	vkb_device = dev_ret.value ();
	device = vkb_device.device;

	CreateQueues ();

	CreateVulkanAllocator ();
}

VulkanDevice::~VulkanDevice ()
{
	allocator_general.Free ();
	allocator_linear_tiling.Free ();
	allocator_optimal_tiling.Free ();

	vkb::destroy_device (vkb_device);
	DestroySurface ();
	vkb::destroy_instance (vkb_instance);
}

bool VulkanDevice::CreateSurface (VkInstance instance, Window const& window)
{
	VkResult res = glfwCreateWindowSurface (instance, window.GetWindowContext (), nullptr, &surface);

	if (res != VK_SUCCESS)
	{
		Log.Error (fmt::format ("Failed to create glfw window{}", errorString (res)));
		return false;
	}
	return true;
}
void VulkanDevice::DestroySurface ()
{
	vkDestroySurfaceKHR (vkb_instance.instance, surface, nullptr);
}

void VulkanDevice::LogMemory () const
{
	allocator_general.LogVMA ();
	allocator_linear_tiling.LogVMA ();
	allocator_optimal_tiling.LogVMA ();
}

// Put all device specific features here
VkPhysicalDeviceFeatures QueryDeviceFeatures ()
{
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.tessellationShader = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	return deviceFeatures;
}

void VulkanDevice::CreateQueues ()
{
	auto g_queue_ret = vkb_device.get_queue_index (vkb::QueueType::graphics);
	auto p_queue_ret = vkb_device.get_queue_index (vkb::QueueType::present);
	auto c_queue_ret = vkb_device.get_dedicated_queue_index (vkb::QueueType::compute);
	auto t_queue_ret = vkb_device.get_dedicated_queue_index (vkb::QueueType::transfer);

	if (g_queue_ret.has_value ())
	{
		graphics_queue = std::make_unique<CommandQueue> (device, g_queue_ret.value ());
	}
	if (p_queue_ret.has_value ())
	{
		present_queue = std::make_unique<CommandQueue> (device, p_queue_ret.value ());
	}
	if (c_queue_ret.has_value ())
	{
		compute_queue = std::make_unique<CommandQueue> (device, c_queue_ret.value ());
	}
	if (t_queue_ret.has_value ())
	{
		transfer_queue = std::make_unique<CommandQueue> (device, t_queue_ret.value ());
	}
}


bool VulkanDevice::has_dedicated_compute () const { return compute_queue != nullptr; }
bool VulkanDevice::has_dedicated_transfer () const { return transfer_queue != nullptr; }

CommandQueue& VulkanDevice::GraphicsQueue () const { return *graphics_queue; }
CommandQueue& VulkanDevice::ComputeQueue () const
{
	if (compute_queue) return *compute_queue;
	return GraphicsQueue ();
}
CommandQueue& VulkanDevice::TransferQueue () const
{
	if (transfer_queue) return *transfer_queue;
	return GraphicsQueue ();
}
CommandQueue& VulkanDevice::PresentQueue () const
{
	if (present_queue)
	{
		return *present_queue;
	}
	return GraphicsQueue ();
}

void VulkanDevice::CreateVulkanAllocator ()
{
	bool general_ret = allocator_general.Create (phys_device.physical_device, device);
	bool linear_ret = allocator_linear_tiling.Create (phys_device.physical_device, device);
	bool optimal_ret = allocator_optimal_tiling.Create (phys_device.physical_device, device);
}


VmaAllocator VulkanDevice::GetGeneralAllocator () const { return allocator_general.allocator; }

VmaAllocator VulkanDevice::GetImageLinearAllocator () const
{
	return allocator_linear_tiling.allocator;
}

VmaAllocator VulkanDevice::GetImageOptimalAllocator () const
{
	return allocator_optimal_tiling.allocator;
}

VkSurfaceKHR VulkanDevice::GetSurface () const { return surface; }

Window& VulkanDevice::GetWindow () const { return *window; }

VkFormat VulkanDevice::FindSupportedFormat (
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties (phys_device.physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	return VK_FORMAT_UNDEFINED;
}
