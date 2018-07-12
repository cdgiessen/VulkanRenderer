#pragma once

#include <optional>

#include <vulkan/vulkan.h>

#include "RenderStructs.h"
#include "Descriptor.h"
#include "Buffer.h"

#include "../resources/Texture.h"
#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

class VulkanRenderer;


class VulkanTexture {
public:
	VulkanTexture(VulkanRenderer& renderer,
		std::optional<Resource::Texture::TexID> textureResource,
		VkFormat format,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool forceLinear = false,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1,
		bool wrapBorder = true,
		int desiredWidth = 0,
		int desiredHeight = 0);
	~VulkanTexture();

	void updateDescriptor();

	VkSampler CreateImageSampler(
		VkFilter mag = VK_FILTER_LINEAR,
		VkFilter min = VK_FILTER_LINEAR,
		VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VkSamplerAddressMode textureWrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		float mipMapLodBias = 0.0f,
		bool useMipmaps = true,
		int mipLevels = 1,
		bool anisotropy = true,
		float maxAnisotropy = 8,
		VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageView CreateImageView(VkImage image,
		VkImageViewType viewType,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkComponentMapping components,
		int mipLevels,
		int layers);

	struct VmaImage {
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo;
		VmaAllocator allocator = nullptr;
	} image;

	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
	VkImageLayout textureImageLayout;

	DescriptorResource resource;


	Signal readyToUse;
protected:
	VulkanRenderer& renderer;

	int mipLevels;
	int layers;

	void InitImage2D(VkImageCreateInfo imageInfo);
	void InitDepthImage(VkImageCreateInfo imageInfo);
	void InitStagingImage2D(VkImageCreateInfo imageInfo);

};


class VulkanTexture2D : public VulkanTexture {
public:
	VulkanTexture2D(VulkanRenderer& renderer,
		Resource::Texture::TexID textureResource,
		VkFormat format,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool forceLinear = false,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1,
		bool wrapBorder = true);
};

class VulkanTexture2DArray : public VulkanTexture {
public:
	VulkanTexture2DArray(VulkanRenderer& renderer,
		Resource::Texture::TexID textureResource,
		VkFormat format,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
};

class VulkanCubeMap : public VulkanTexture {
public:
	VulkanCubeMap(VulkanRenderer& renderer,
		Resource::Texture::TexID textureResource,
		VkFormat format,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);

};

class VulkanTextureDepthBuffer : public VulkanTexture {
public:
	VulkanTextureDepthBuffer(VulkanRenderer& renderer, 
		VkFormat depthFormat, int width, int height);
};

// class VulkanTextureManager {
// public:
// 	VulkanTextureManager(VulkanRenderer& renderer);
// 	~VulkanTextureManager();

// 	std::shared_ptr<VulkanTexture2D> CreateTexture2D(
// 		Resource::Texture::TexID texture,
// 		VkFormat format,
// 		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
// 		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
// 		bool forceLinear = false,
// 		bool genMipMaps = false,
// 		int mipMapLevelsToGen = 1,
// 		bool wrapBorder = true);

// 	std::shared_ptr<VulkanTexture2DArray> CreateTexture2DArray(
// 		Resource::Texture::TexID textures,
// 		VkFormat format,
// 		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
// 		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
// 		bool genMipMaps = false,
// 		int mipMapLevelsToGen = 1);

// 	std::shared_ptr<VulkanCubeMap> CreateCubeMap(
// 		Resource::Texture::TexID cubeMap,
// 		VkFormat format,
// 		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
// 		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
// 		bool genMipMaps = false,
// 		int mipMapLevelsToGen = 1);

// 	std::shared_ptr<VulkanTextureDepthBuffer> VulkanTextureDepthBuffer(
// 		VkFormat depthFormat, int width, int height);

// private:
// 	VulkanRenderer& renderer;

// 	std::vector<std::shared_ptr<VulkanTexture>> vulkanTextures;
// };

static void GenerateMipMaps(VkCommandBuffer cmdBuf, VkImage image,
	VkImageLayout finalImageLayout,
	int width, int height, int depth,
	int layers, int mipLevels);

static void SetLayoutAndTransferRegions(
	VkCommandBuffer transferCmdBuf, VkImage image, VkBuffer stagingBuffer,
	const VkImageSubresourceRange subresourceRange,
	std::vector<VkBufferImageCopy> bufferCopyRegions);

static void BeginTransferAndMipMapGenWork(
	VulkanRenderer & renderer,
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