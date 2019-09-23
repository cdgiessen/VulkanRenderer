#pragma once

#include <cstddef>
#include <memory>
#include <thread>
#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "Descriptor.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "resources/Texture.h"

class VulkanRenderer;
using Signal = std::shared_ptr<bool>;

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
	VulkanTexture (VulkanRenderer& renderer, TexCreateDetails texCreateDetails, Resource::Texture::TexResource textureResource);

	VulkanTexture (VulkanRenderer& renderer, TexCreateDetails texCreateDetails, std::shared_ptr<VulkanBuffer> buffer);

	VulkanTexture (VulkanRenderer& renderer, TexCreateDetails texCreateDetails);

	~VulkanTexture ();

	struct VmaImage
	{
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo;
		VmaAllocator allocator = nullptr;
	} image;

	protected:
	VulkanRenderer& renderer;

	public:
	DescriptorResource resource;

	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
	VkImageLayout textureImageLayout;

	Signal readyToUse;

	protected:
	int mipLevels;
	int layers;

	void InitImage2D (VkImageCreateInfo imageInfo);
	void InitDepthImage (VkImageCreateInfo imageInfo);

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

class VulkanTextureManager
{
	public:
	VulkanTextureManager (VulkanRenderer& renderer, Resource::Texture::Manager& texManager);
	~VulkanTextureManager ();

	VulkanTextureID CreateTexture2D (Resource::Texture::TexID texture, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateTexture2DArray (Resource::Texture::TexID textures, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateCubeMap (Resource::Texture::TexID cubeMap, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateTextureFromBuffer (std::shared_ptr<VulkanBuffer> buffer, TexCreateDetails texCreateDetails);

	VulkanTextureID CreateDepthImage (VkFormat depthFormat, int width, int height);

	void delete_texture (VulkanTextureID id);

	VulkanTexture const& get_texture (VulkanTextureID id);

	private:
	VulkanRenderer& renderer;
	Resource::Texture::Manager& texManager;

	VulkanTextureID id_counter = 0;
	std::mutex map_lock;
	std::unordered_map<VulkanTextureID, std::unique_ptr<VulkanTexture>> texture_map;
};

void BeginTransferAndMipMapGenWork (VulkanRenderer& renderer,
    std::shared_ptr<VulkanBuffer> buffer,
    const VkImageSubresourceRange subresourceRange,
    const std::vector<VkBufferImageCopy> bufferCopyRegions,
    VkImageLayout imageLayout,
    VkImage image,
    VkBuffer vk_buffer,
    int width,
    int height,
    int depth,
    Signal signal,
    int layers,
    int mipLevels);
