#pragma once

#include <cstddef>
#include <memory>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

#include "resources/Texture.h"

#include "AsyncTask.h"

class VulkanDevice;
class VulkanBuffer;
class AsyncTaskQueue;

struct TexCreateDetails
{
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	bool genMipMaps = false;
	uint32_t mipMapLevelsToGen = 1;
	uint32_t desiredWidth = 0;
	uint32_t desiredHeight = 0;
	VkImageUsageFlags usage = 0;

	TexCreateDetails (){};
	TexCreateDetails (VkFormat format,
	    VkImageLayout imageLayout,
	    bool genMipMaps,
	    uint32_t mipMapLevelsToGen,
	    uint32_t desiredWidth = 0,
	    uint32_t desiredHeight = 0)
	: format (format),
	  imageLayout (imageLayout),
	  genMipMaps (genMipMaps),
	  mipMapLevelsToGen (mipMapLevelsToGen),
	  desiredWidth (desiredWidth),
	  desiredHeight (desiredHeight){};
};

class VulkanTexture
{
	public:
	VulkanTexture (VulkanDevice& device,
	    AsyncTaskQueue& async_task_man,
	    std::function<void ()> const& finish_work,
	    TexCreateDetails texCreateDetails,
	    Resource::Texture::TexResource textureResource);

	VulkanTexture (VulkanDevice& device,
	    AsyncTaskQueue& async_task_man,
	    std::function<void ()> const& finish_work,
	    TexCreateDetails texCreateDetails,
	    std::unique_ptr<VulkanBuffer> buffer);

	VulkanTexture (VulkanDevice& device, TexCreateDetails texCreateDetails);

	~VulkanTexture ();

	VulkanTexture (VulkanTexture const& tex) = delete;
	VulkanTexture& operator= (VulkanTexture const& tex) = delete;

	VulkanTexture (VulkanTexture&& tex) noexcept;
	VulkanTexture& operator= (VulkanTexture&& tex) noexcept;

	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;

	VkDescriptorType get_descriptor_type () { return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; }
	VkDescriptorImageInfo get_resource ()
	{
		VkDescriptorImageInfo info{};
		info.imageLayout = data.textureImageLayout;
		info.imageView = imageView;
		info.sampler = sampler;
		return info;
	}

	uint32_t get_width () { return data.width; }
	uint32_t get_height () { return data.height; }
	uint32_t get_layers () { return data.layers; }

	private:
	struct TexData
	{
		VulkanDevice* device;

		VkImageLayout textureImageLayout;

		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo;
		VmaAllocator allocator = nullptr;

		uint32_t mipLevels;
		uint32_t layers;
		uint32_t width = 0;
		uint32_t height = 0;
	};
	TexData data;
	std::unique_ptr<VulkanBuffer> staging_buffer;

	void init_image_2d (VkImageCreateInfo imageInfo);

	VkSampler create_image_sampler (VkFilter mag = VK_FILTER_LINEAR,
	    VkFilter min = VK_FILTER_LINEAR,
	    VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    VkSamplerAddressMode textureWrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	    float mipMapLodBias = 0.0f,
	    bool useMipmaps = true,
	    int mipLevels = 1,
	    bool anisotropy = true,
	    float maxAnisotropy = 8,
	    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageView create_image_view (VkImage image,
	    VkImageViewType viewType,
	    VkFormat format,
	    VkImageAspectFlags aspectFlags,
	    VkComponentMapping components,
	    uint32_t mipLevels,
	    uint32_t layers);
};

using VulkanTextureID = int32_t;

class Textures
{
	public:
	Textures (Resource::Texture::Textures& textures, VulkanDevice& device, AsyncTaskQueue& async_task_queue);
	~Textures ();

	Textures (Textures const& buf) = delete;
	Textures& operator= (Textures const& buf) = delete;


	VulkanTextureID create_texture_2d (Resource::Texture::TexID texture_id, TexCreateDetails texCreateDetails);

	VulkanTextureID create_texture_2d_array (Resource::Texture::TexID texture_id, TexCreateDetails texCreateDetails);

	VulkanTextureID create_cube_map (Resource::Texture::TexID cubeMap, TexCreateDetails texCreateDetails);

	VulkanTextureID create_texture_from_buffer (
	    std::unique_ptr<VulkanBuffer> buffer, TexCreateDetails texCreateDetails);

	VulkanTexture create_attachment_image (TexCreateDetails texCreateDetails);

	void delete_texture (VulkanTextureID id);

	bool is_finished_transfer (VulkanTextureID id);
	VkDescriptorImageInfo get_resource (VulkanTextureID id);
	VkDescriptorType get_descriptor_type (VulkanTextureID id);

	private:
	std::function<void ()> create_finish_work (VulkanTextureID id);

	Resource::Texture::Textures& textures;
	VulkanDevice& device;
	AsyncTaskQueue& async_task_queue;

	VulkanTextureID id_counter = 0;
	std::mutex map_lock;
	std::unordered_map<VulkanTextureID, std::unique_ptr<VulkanTexture>> in_progress_map;
	std::unordered_map<VulkanTextureID, std::unique_ptr<VulkanTexture>> texture_map;

	std::vector<VulkanTextureID> expired_textures;
};
