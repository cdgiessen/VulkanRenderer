#include "Device.h"

#include <string>

#include <GLFW/glfw3.h>

#include "core/Logger.h"
#include "core/Window.h"

#include "RenderTools.h"

//////// VMA Memory Resource ////////

bool VMA_MemoryResource::create (
    VkPhysicalDevice physical_device, VkDevice device, VkAllocationCallbacks* custom_allocator)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physical_device;
	allocatorInfo.device = device;
	allocatorInfo.pAllocationCallbacks = custom_allocator;

	VkResult res = vmaCreateAllocator (&allocatorInfo, &allocator);
	if (res != VK_SUCCESS)
	{
		Log.error (fmt::format ("Failed to create glfw window{}", errorString (res)));
		return false;
	}
	return true;
}
void VMA_MemoryResource::free ()
{
	if (allocator)
	{
		vmaDestroyAllocator (allocator);
	}
}

void VMA_MemoryResource::log (bool detailedOutput) const
{
	char* str;
	vmaBuildStatsString (allocator, &str, detailedOutput);
	std::string out_str (str);
	vmaFreeStatsString (allocator, str);

	Log.debug (fmt::format ("Allocator Data Dump:\n {}", out_str));
}

//////// DebugCallback ////////

static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
	auto ms = vkb::to_string_message_severity (messageSeverity);
	auto mt = vkb::to_string_message_type (messageType);
	Log.error (fmt::format ("{} {}:\n{}", ms, mt, pCallbackData->pMessage));
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
		Log.error (fmt::format ("Failed to create instance: {}", inst_ret.error ().message ()));
	}
	vkb_instance = inst_ret.value ();

	bool surf_ret = create_surface (vkb_instance.instance, window);
	if (!surf_ret)
	{
		Log.error (fmt::format ("Failed to create surface"));
	}

	vkb::PhysicalDeviceSelector selector (vkb_instance);
	selector.set_required_features (QueryDeviceFeatures ());
	auto phys_ret = selector.set_surface (surface).select ();
	if (!phys_ret)
	{
		// TODO
		Log.error (fmt::format ("Failed to select physical device: {}", phys_ret.error ().message ()));
	}
	phys_device = phys_ret.value ();
	vkb::DeviceBuilder dev_builder (phys_device);
	auto dev_ret = dev_builder.build ();
	if (!dev_ret)
	{
		// TODO
		Log.error (fmt::format ("Failed to create device: {}", dev_ret.error ().message ()));
	}
	vkb_device = dev_ret.value ();
	device = vkb_device.device;

	create_queues ();

	create_vulkan_allocator ();
}

VulkanDevice::~VulkanDevice ()
{
	allocator_general.free ();
	allocator_linear_tiling.free ();
	allocator_optimal_tiling.free ();

	vkb::destroy_device (vkb_device);
	destroy_surface ();
	vkb::destroy_instance (vkb_instance);
}

bool VulkanDevice::create_surface (VkInstance instance, Window const& window)
{
	VkResult res = glfwCreateWindowSurface (instance, window.get_window_context (), nullptr, &surface);

	if (res != VK_SUCCESS)
	{
		Log.error (fmt::format ("Failed to create glfw window{}", errorString (res)));
		return false;
	}
	return true;
}
void VulkanDevice::destroy_surface ()
{
	vkDestroySurfaceKHR (vkb_instance.instance, surface, nullptr);
}

void VulkanDevice::LogMemory () const
{
	allocator_general.log ();
	allocator_linear_tiling.log ();
	allocator_optimal_tiling.log ();
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

void VulkanDevice::create_queues ()
{
	auto g_queue_ret = vkb_device.get_queue_index (vkb::QueueType::graphics);
	auto p_queue_ret = vkb_device.get_queue_index (vkb::QueueType::present);
	auto c_queue_ret = vkb_device.get_dedicated_queue_index (vkb::QueueType::compute);
	auto t_queue_ret = vkb_device.get_dedicated_queue_index (vkb::QueueType::transfer);

	if (g_queue_ret.has_value ())
	{
		m_graphics_queue = std::make_unique<CommandQueue> (device, g_queue_ret.value ());
	}
	if (p_queue_ret.has_value ())
	{
		m_present_queue = std::make_unique<CommandQueue> (device, p_queue_ret.value ());
	}
	if (c_queue_ret.has_value ())
	{
		m_compute_queue = std::make_unique<CommandQueue> (device, c_queue_ret.value ());
	}
	if (t_queue_ret.has_value ())
	{
		m_transfer_queue = std::make_unique<CommandQueue> (device, t_queue_ret.value ());
	}
}


bool VulkanDevice::has_dedicated_compute () const { return m_compute_queue != nullptr; }
bool VulkanDevice::has_dedicated_transfer () const { return m_transfer_queue != nullptr; }

CommandQueue& VulkanDevice::graphics_queue () const { return *m_graphics_queue; }
CommandQueue& VulkanDevice::compute_queue () const
{
	if (m_compute_queue) return *m_compute_queue;
	return graphics_queue ();
}
CommandQueue& VulkanDevice::transfer_queue () const
{
	if (m_transfer_queue) return *m_transfer_queue;
	return graphics_queue ();
}
CommandQueue& VulkanDevice::present_queue () const
{
	if (m_present_queue)
	{
		return *m_present_queue;
	}
	return graphics_queue ();
}

void VulkanDevice::create_vulkan_allocator ()
{
	bool general_ret = allocator_general.create (phys_device.physical_device, device);
	bool linear_ret = allocator_linear_tiling.create (phys_device.physical_device, device);
	bool optimal_ret = allocator_optimal_tiling.create (phys_device.physical_device, device);
}


VmaAllocator VulkanDevice::get_general_allocator () const { return allocator_general.allocator; }

VmaAllocator VulkanDevice::get_image_allocator () const
{
	return allocator_linear_tiling.allocator;
}

VmaAllocator VulkanDevice::get_image_optimal_allocator () const
{
	return allocator_optimal_tiling.allocator;
}

VkSurfaceKHR VulkanDevice::get_surface () const { return surface; }

Window& VulkanDevice::get_window () const { return *window; }

VkFormat VulkanDevice::find_supported_format (
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
