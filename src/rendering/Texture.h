#pragma once

#include <cstddef>
#include <memory>
#include <thread>
#include <vulkan/vulkan.h>

#include "AsyncTask.h"
#include "Buffer.h"
#include "Descriptor.h"
#include "Wrappers.h"

#include "resources/Texture.h"
#include "vk_mem_alloc.h"

class VulkanDevice;

void BeginTransferAndMipMapGenWork (VulkanDevice& device,
    BufferID buffer,
    const VkImageSubresourceRange subresourceRange,
    const std::vector<VkBufferImageCopy> bufferCopyRegions,
    VkImageLayout imageLayout,
    VkImage image,
    VkBuffer vk_buffer,
    int width,
    int height,
    int depth,
    int layers,
    int mipLevels);

struct TexCreateDetails
{
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	bool genMipMaps = false;
	int mipMapLevelsToGen = 1;
	int desiredWidth = 0;
	int desiredHeight = 0;

	TexCreateDetails (){};
	TexCreateDetails (VkFormat format,
	    VkImageLayout imageLayout,
	    bool genMipMaps,
	    int mipMapLevelsToGen,
	    int desiredWidth = 0,
	    int desiredHeight = 0)
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
	    AsyncTaskManager& async_task_man,
	    BufferManager& buf_man,
	    TexCreateDetails texCreateDetails,
	    Resource::Texture::TexResource textureResource);

	VulkanTexture (VulkanDevice& device,
	    AsyncTaskManager& async_task_man,
	    BufferManager& buf_man,
	    TexCreateDetails texCreateDetails,
	    BufferID buffer);

	VulkanTexture (VulkanDevice& device, TexCreateDetails texCreateDetails);

	~VulkanTexture ();

	struct VmaImage
	{
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo;
		VmaAllocator allocator = nullptr;
	} image;

	protected:
	VulkanDevice& device;

	public:
	DescriptorResource resource;

	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
	VkImageLayout textureImageLayout;

	protected:
	int mipLevels;
	int layers;

	void InitImage2D (VkImageCreateInfo imageInfo);

	VkSampler CreateImageSampler (VkFilter mag = VK_FILTER_LINEAR,
	    VkFilter min = VK_FILTER_LINEAR,
	    VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    VkSamplerAddressMode textureWrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	    float mipMapLodBias = 0.0f,
	    bool useMipmaps = true,
	    int mipLevels = 1,
	    bool anisotropy = true,
	    float maxAnisotropy = 8,
	    VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageView CreateImageView (VkImage image,
	    VkImageViewType viewType,
	    VkFormat format,
	    VkImageAspectFlags aspectFlags,
	    VkComponentMapping components,
	    int mipLevels,
	    int layers);
};

using VulkanTextureID = int;

class TextureManager
{
	public:
	TextureManager (Resource::Texture::Manager& texture_manager,
	    VulkanDevice& device,
	    AsyncTaskManager& async_task_manager,
	    BufferManager& buf_man);
	~TextureManager ();

	TextureManager (TextureManager const& buf) = delete;
	TextureManager& operator= (TextureManager const& buf) = delete;


	VulkanTextureID CreateTexture2D (Resource::Texture::TexID texture, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateTexture2DArray (Resource::Texture::TexID textures, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateCubeMap (Resource::Texture::TexID cubeMap, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateTextureFromBuffer (BufferID buffer, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateDepthImage (VkFormat depthFormat, int width, int height);

	void delete_texture (VulkanTextureID id);

	VulkanTexture const& get_texture (VulkanTextureID id);

	private:
	VulkanDevice& device;
	Resource::Texture::Manager& texture_manager;
	AsyncTaskManager& async_task_manager;
	BufferManager& buffer_manager;

	VulkanTextureID id_counter = 0;
	std::mutex map_lock;
	std::unordered_map<VulkanTextureID, std::unique_ptr<VulkanTexture>> texture_map;
};
