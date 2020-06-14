#pragma once

#include <string.h>
#include <optional>
#include <vector>

#include <openxr/openxr.h>

#include <vulkan/vulkan.h>

#include "rendering/Renderer.h"

const XrViewConfigurationType view_config_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

struct OpenXR
{
	XrInstance inst;
	XrInstanceProperties props;
	struct SysInfo
	{
		XrSystemId id;
		XrSystemProperties props;
	} sys;
	XrSession session;

	struct Display
	{
		std::vector<XrViewConfigurationView> configViews;
		struct Swapchain
		{
			int32_t width;
			int32_t height;
			XrSwapchain swapchain;
		};
		std::vector<Swapchain> swapchains;
	} display;
};

std::optional<OpenXR> create_openxr (VulkanOpenXRInit vk_init);
void destroy_openxr (OpenXR openxr);

void poll_events (OpenXR openxr);
bool is_running (OpenXR openxr);
bool is_focused (OpenXR openxr);

void poll_actions (OpenXR openxr);
void render_frame (OpenXR openxr);