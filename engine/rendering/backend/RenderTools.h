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
			Log.error (fmt::format (                                                                \
			    "Fatal : VkResult is {} in {} at line {}", errorString (res), __FILE__, __LINE__)); \
			assert (res == VK_SUCCESS);                                                             \
		}                                                                                           \
	}

template <typename T, typename Deleter> class VulkanHandle
{
	public:
	explicit VulkanHandle (VkDevice device, T handle, Deleter deleter)
	: device (device), handle (handle), deleter (deleter)
	{
	}
	~VulkanHandle ()
	{
		if (handle != nullptr)
		{
			deleter (device, handle, nullptr);
		}
	};
	VulkanHandle (VulkanHandle const& fence) = delete;
	VulkanHandle& operator= (VulkanHandle const& fence) = delete;

	VulkanHandle (VulkanHandle&& other) noexcept
	: device (other.device), handle (other.handle), deleter (other.deleter)
	{
		other.handle = nullptr;
	}
	VulkanHandle& operator= (VulkanHandle&& other) noexcept
	{
		if (this != &other)
		{
			if (handle != nullptr)
			{
				deleter (device, handle, nullptr);
			}

			device = other.device;
			handle = other.handle;
			deleter = other.deleter;
			other.handle = nullptr;
		}
		return *this;
	}

	VkDevice device;
	T handle;
	Deleter deleter;
};