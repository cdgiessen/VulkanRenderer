#include "OpenXR.h"

#include <algorithm>
#include <string>

#include <vulkan/vulkan.h>

#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>

#include "core/Logger.h"

std::optional<XrInstance> create_inst (
    std::vector<std::string> layers_to_enable, std::vector<std::string> extensions_to_enable)
{
	// Layers
	uint32_t layer_count;
	auto layer_ret = xrEnumerateApiLayerProperties (0, &layer_count, nullptr);
	if (layer_ret != XR_SUCCESS) return {};
	std::vector<XrApiLayerProperties> layers (layer_count);
	for (XrApiLayerProperties& layer : layers)
	{
		layer.type = XR_TYPE_API_LAYER_PROPERTIES;
	}

	layer_ret = xrEnumerateApiLayerProperties (
	    static_cast<uint32_t> (layers.size ()), &layer_count, layers.data ());
	if (layer_ret != XR_SUCCESS) return {};

	std::vector<const char*> enabled_layers;
	for (auto& layer_to_enable : layers_to_enable)
	{
		bool found = false;
		for (auto& layer : layers)
		{
			if (strcmp (layer.layerName, layer_to_enable.data ()) == 0) found = true;
		}
		if (found) enabled_layers.push_back (layer_to_enable.data ());
	}

	// Extensions
	uint32_t ext_count;
	auto ext_ret = xrEnumerateInstanceExtensionProperties (nullptr, 0, &ext_count, nullptr);
	if (ext_ret != XR_SUCCESS) return {};
	std::vector<XrExtensionProperties> extensions (ext_count);
	for (XrExtensionProperties& extension : extensions)
	{
		extension.type = XR_TYPE_EXTENSION_PROPERTIES;
	}
	ext_ret = xrEnumerateInstanceExtensionProperties (
	    nullptr, static_cast<uint32_t> (extensions.size ()), &ext_count, extensions.data ());

	std::vector<const char*> enabled_extensions;
	for (auto& ext_to_enable : extensions_to_enable)
	{
		bool found = false;
		for (auto& ext : extensions)
		{
			if (strcmp (ext.extensionName, ext_to_enable.data ()) == 0) found = true;
		}
		if (found) enabled_extensions.push_back (ext_to_enable.data ());
	}

	// XrInstance
	XrInstanceCreateInfo inst_info{ XR_TYPE_INSTANCE_CREATE_INFO };
	inst_info.applicationInfo.apiVersion = XR_MAKE_VERSION (1, 0, 9);
	auto app_name = "HelloXR";
	auto engine_name = "VulkanRenderer";
	strncpy (inst_info.applicationInfo.applicationName, app_name, sizeof (app_name));
	inst_info.applicationInfo.applicationVersion = 0;
	strncpy (inst_info.applicationInfo.engineName, engine_name, sizeof (engine_name));
	inst_info.applicationInfo.engineVersion = 0;
	inst_info.enabledApiLayerCount = static_cast<uint32_t> (enabled_layers.size ());
	inst_info.enabledApiLayerNames = enabled_layers.data ();
	inst_info.enabledExtensionCount = static_cast<uint32_t> (enabled_extensions.size ());
	inst_info.enabledExtensionNames = enabled_extensions.data ();

	XrInstance inst;
	auto ret = xrCreateInstance (&inst_info, &inst);
	if (ret == XR_SUCCESS) return inst;
	return {};
}

std::optional<OpenXR::SysInfo> get_sys_info (XrInstance inst)
{

	XrSystemGetInfo sys_info{ XR_TYPE_SYSTEM_GET_INFO };
	sys_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrSystemId sys_id;
	auto ret = xrGetSystem (inst, &sys_info, &sys_id);
	if (ret != XR_SUCCESS) return {};
	XrSystemProperties sys_props{ XR_TYPE_SYSTEM_PROPERTIES };
	ret = xrGetSystemProperties (inst, sys_id, &sys_props);
	if (ret != XR_SUCCESS) return {};

	return OpenXR::SysInfo{ sys_id, sys_props };
}

std::optional<XrSession> create_session (XrInstance inst, OpenXR::SysInfo sys, VulkanOpenXRInit vk_init)
{
	XrGraphicsBindingVulkanKHR gfx_binding{ XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR };
	gfx_binding.instance = vk_init.inst;
	gfx_binding.physicalDevice = vk_init.phys_dev;
	gfx_binding.device = vk_init.device;
	gfx_binding.queueFamilyIndex = vk_init.queue.GetQueueFamily ();
	gfx_binding.queueIndex = vk_init.queue.GetQueueIndex ();

	XrSessionCreateInfo session_info{ XR_TYPE_SESSION_CREATE_INFO };
	session_info.next = &gfx_binding;
	session_info.systemId = sys.id;
	XrSession session;
	auto ret = xrCreateSession (inst, &session_info, &session);
	if (ret == XR_SUCCESS) return session;
	return {};
}

int64_t SelectColorSwapchainFormat (std::vector<int64_t> const& runtime_formats)
{
	// List of supported color swapchain formats.
	constexpr int64_t SupportedColorSwapchainFormats[] = {
		VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM
	};

	auto swapchainFormatIt = std::find_first_of (runtime_formats.begin (),
	    runtime_formats.end (),
	    std::begin (SupportedColorSwapchainFormats),
	    std::end (SupportedColorSwapchainFormats));
	if (swapchainFormatIt == runtime_formats.end ())
	{
		return -1;
	}

	return *swapchainFormatIt;
}

// std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs (
//     uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo)
// {
// 	// Allocate and initialize the buffer of image structs (must be sequential in memory for xrEnumerateSwapchainImages).
// 	// Return back an array of pointers to each swapchain image struct so the consumer doesn't need to know the type/size.
// 	// Keep the buffer alive by adding it into the list of buffers.
// 	m_swapchainImageContexts.emplace_back ();
// 	SwapchainImageContext& swapchainImageContext = m_swapchainImageContexts.back ();

// 	std::vector<XrSwapchainImageBaseHeader*> bases = swapchainImageContext.Create (
// 	    m_vkDevice, &m_memAllocator, capacity, swapchainCreateInfo, m_pipelineLayout, m_shaderProgram, m_drawBuffer);

// 	// Map every swapchainImage base pointer to this context
// 	for (auto& base : bases)
// 	{
// 		m_swapchainImageContextMap.emplace (std::make_pair (base, &swapchainImageContext));
// 	}

// 	return bases;
// }

std::optional<OpenXR::Display> create_swapchains (XrInstance inst, OpenXR::SysInfo sys, XrSession session)
{
	std::vector<XrViewConfigurationView> configuration_views;

	uint32_t view_count;
	auto ret = xrEnumerateViewConfigurationViews (inst, sys.id, view_config_type, 0, &view_count, nullptr);
	if (ret != XR_SUCCESS) return {};
	configuration_views.resize (view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	ret = xrEnumerateViewConfigurationViews (
	    inst, sys.id, view_config_type, view_count, &view_count, configuration_views.data ());
	if (ret != XR_SUCCESS) return {};

	if (view_count < 0) return {};

	uint32_t swapchain_format_count;
	ret = xrEnumerateSwapchainFormats (session, 0, &swapchain_format_count, nullptr);
	if (ret != XR_SUCCESS) return {};
	std::vector<int64_t> swapchain_formats (swapchain_format_count);
	ret = xrEnumerateSwapchainFormats (
	    session, (uint32_t)swapchain_formats.size (), &swapchain_format_count, swapchain_formats.data ());
	if (ret != XR_SUCCESS) return {};

	uint64_t color_swapchains_format = SelectColorSwapchainFormat (swapchain_formats);

	std::vector<OpenXR::Display::Swapchain> swapchains;
	for (uint32_t i = 0; i < view_count; i++)
	{
		const XrViewConfigurationView& vp = configuration_views[i];
		XrSwapchainCreateInfo swapchain_info{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
		swapchain_info.arraySize = 1;
		swapchain_info.format = color_swapchains_format;
		swapchain_info.width = vp.recommendedImageRectWidth;
		swapchain_info.height = vp.recommendedImageRectHeight;
		swapchain_info.mipCount = 1;
		swapchain_info.faceCount = 1;
		swapchain_info.sampleCount = vp.recommendedSwapchainSampleCount;
		swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		XrSwapchain xr_swapchain;
		auto ret = xrCreateSwapchain (session, &swapchain_info, &xr_swapchain);
		if (ret != XR_SUCCESS) return {};

		OpenXR::Display::Swapchain swapchain;
		swapchain.width = swapchain_info.width;
		swapchain.height = swapchain_info.height;
		swapchain.swapchain = xr_swapchain;
		swapchains.push_back (swapchain);

		uint32_t image_count;
		ret = xrEnumerateSwapchainImages (swapchain.swapchain, 0, &image_count, nullptr);
		if (ret != XR_SUCCESS) return {};

		// XXX This should really just return XrSwapchainImageBaseHeader*
		// std::vector<XrSwapchainImageBaseHeader*> swapchain_images =
		//     m_graphicsPlugin->AllocateSwapchainImageStructs (image_count, swapchain_info);
		// ret = xrEnumerateSwapchainImages (swapchain.swapchain, image_count, &image_count,
		// swapchain_images[0]); if (ret != XR_SUCCESS) return {};


		// m_swapchainImages.insert (std::make_pair (swapchain.swapchain, std::move (swapchain_images)));
	}


	return {};
}


std::optional<OpenXR> create_openxr (VulkanOpenXRInit vk_init)
{
	auto inst = create_inst ({}, { "XR_KHR_vulkan_enable" });
	if (!inst) return {};
	XrInstanceProperties instance_props{ XR_TYPE_INSTANCE_PROPERTIES };
	auto prop_res = xrGetInstanceProperties (*inst, &instance_props);
	if (!prop_res) return {};
	auto sys = get_sys_info (*inst);
	if (!sys) return {};
	auto session = create_session (*inst, *sys, vk_init);
	if (!session) return {};

	OpenXR openxr{};
	openxr.inst = *inst;
	openxr.sys = *sys;
	openxr.session = *session;
}
void destroy_openxr (OpenXR openxr)
{
	xrDestroySession (openxr.session);
	xrDestroyInstance (openxr.inst);
}

void poll_events (OpenXR openxr) {}
bool is_running (OpenXR openxr) {}
bool is_focused (OpenXR openxr) {}

void poll_actions (OpenXR openxr) {}
void render_frame (OpenXR openxr) {}