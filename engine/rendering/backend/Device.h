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
	bool Create (VkPhysicalDevice physical_device, VkDevice device, VkAllocationCallbacks* custom_allocator = nullptr);

	void Free ();

	void LogVMA (bool detailedOutput = false) const;

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

	CommandQueue& GraphicsQueue () const;
	CommandQueue& ComputeQueue () const;
	CommandQueue& TransferQueue () const;
	CommandQueue& PresentQueue () const;

	VmaAllocator GetGeneralAllocator () const;
	VmaAllocator GetImageLinearAllocator () const;
	VmaAllocator GetImageOptimalAllocator () const;

	VkSurfaceKHR GetSurface () const;
	Window& GetWindow () const;

	VkFormat FindSupportedFormat (
	    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

	private:
	std::unique_ptr<CommandQueue> graphics_queue;
	std::unique_ptr<CommandQueue> compute_queue;
	std::unique_ptr<CommandQueue> transfer_queue;
	std::unique_ptr<CommandQueue> present_queue;

	VMA_MemoryResource allocator_general;
	VMA_MemoryResource allocator_linear_tiling;
	VMA_MemoryResource allocator_optimal_tiling;

	bool CreateSurface (VkInstance instance, Window const& window);
	void DestroySurface ();
	void CreateQueues ();
	void CreateVulkanAllocator ();
};
