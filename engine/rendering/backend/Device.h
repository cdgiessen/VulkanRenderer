#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"
#include "VkBootstrap.h"

#include "Wrappers.h"



class Window;

struct VMA_MemoryResource
{
	public:
	bool create (VkPhysicalDevice physical_device, VkDevice device, VkAllocationCallbacks* custom_allocator = nullptr);

	void free ();

	void log (bool detailedOutput = false) const;

	VmaAllocator allocator;
};

class VulkanDevice
{
	private:
	bool enableValidationLayers = false;
	Window* window;

	public:
	vkb::Instance vkb_instance;
	VkSurfaceKHR surface;
	vkb::PhysicalDevice phys_device;

	vkb::Device vkb_device;
	VkDevice device;

	VulkanDevice (Window& window, bool validationLayers = false);

	~VulkanDevice ();
	bool has_dedicated_compute () const;
	bool has_dedicated_transfer () const;

	void LogMemory () const;

	CommandQueue& graphics_queue () const;
	CommandQueue& compute_queue () const;
	CommandQueue& transfer_queue () const;
	CommandQueue& present_queue () const;

	VmaAllocator get_general_allocator () const;
	VmaAllocator get_image_allocator () const;
	VmaAllocator get_image_optimal_allocator () const;

	VkSurfaceKHR get_surface () const;
	Window& get_window () const;

	VkFormat find_supported_format (
	    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

	private:
	std::unique_ptr<CommandQueue> m_graphics_queue;
	std::unique_ptr<CommandQueue> m_compute_queue;
	std::unique_ptr<CommandQueue> m_transfer_queue;
	std::unique_ptr<CommandQueue> m_present_queue;

	VMA_MemoryResource allocator_general;
	VMA_MemoryResource allocator_linear_tiling;
	VMA_MemoryResource allocator_optimal_tiling;

	bool create_surface (VkInstance instance, Window const& window);
	void destroy_surface ();
	void create_queues ();
	void create_vulkan_allocator ();
};
