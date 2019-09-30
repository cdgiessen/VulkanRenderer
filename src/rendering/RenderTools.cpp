#include "RenderTools.h"

#include <algorithm>

std::string errorString (const VkResult errorCode)
{
	switch (errorCode)
	{
#define STR(r)                                                                                     \
	case VK_##r:                                                                                   \
		return #r
		STR (NOT_READY);
		STR (TIMEOUT);
		STR (EVENT_SET);
		STR (EVENT_RESET);
		STR (INCOMPLETE);
		STR (ERROR_OUT_OF_HOST_MEMORY);
		STR (ERROR_OUT_OF_DEVICE_MEMORY);
		STR (ERROR_INITIALIZATION_FAILED);
		STR (ERROR_DEVICE_LOST);
		STR (ERROR_MEMORY_MAP_FAILED);
		STR (ERROR_LAYER_NOT_PRESENT);
		STR (ERROR_EXTENSION_NOT_PRESENT);
		STR (ERROR_FEATURE_NOT_PRESENT);
		STR (ERROR_INCOMPATIBLE_DRIVER);
		STR (ERROR_TOO_MANY_OBJECTS);
		STR (ERROR_FORMAT_NOT_SUPPORTED);
		STR (ERROR_SURFACE_LOST_KHR);
		STR (ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR (SUBOPTIMAL_KHR);
		STR (ERROR_OUT_OF_DATE_KHR);
		STR (ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR (ERROR_VALIDATION_FAILED_EXT);
		STR (ERROR_INVALID_SHADER_NV);
#undef STR
		default:
			return "UNKNOWN_ERROR";
	}
}

VkSampleCountFlagBits getMaxUsableSampleCount (VkPhysicalDeviceProperties properties)
{
	VkSampleCountFlags counts = std::min (properties.limits.framebufferColorSampleCounts,
	    properties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT)
	{
		return VK_SAMPLE_COUNT_64_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_32_BIT)
	{
		return VK_SAMPLE_COUNT_32_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_16_BIT)
	{
		return VK_SAMPLE_COUNT_16_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_8_BIT)
	{
		return VK_SAMPLE_COUNT_8_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_4_BIT)
	{
		return VK_SAMPLE_COUNT_4_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_2_BIT)
	{
		return VK_SAMPLE_COUNT_2_BIT;
	}
	return VK_SAMPLE_COUNT_1_BIT;
}