#pragma once

#include <vulkan/vulkan.h>

#include "core/Logger.h"

/** @brief Returns an error code as a string */
const char* errorString (const VkResult errorCode);

#define VK_CHECK_RESULT(f)                                                                          \
	{                                                                                               \
		VkResult res = (f);                                                                         \
		if (res != VK_SUCCESS)                                                                      \
		{                                                                                           \
			Log.Error (fmt::format (                                                                \
			    "Fatal : VkResult is {} in {} at line {}", errorString (res), __FILE__, __LINE__)); \
			assert (res == VK_SUCCESS);                                                             \
		}                                                                                           \
	}
