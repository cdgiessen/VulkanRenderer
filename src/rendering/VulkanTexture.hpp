#pragma once

#include <vulkan/vulkan.h>
#include "VulkanDevice.hpp"

#include "../resources/Texture.h"

#include "../../third-party/stb_image/stb_image.h"

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

class VulkanTexture {
public:
	VkImage vmaImage = VK_NULL_HANDLE;
	VmaAllocation vmaImageAlloc = VK_NULL_HANDLE;

	VkImageLayout textureImageLayout;
	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;

	VkDescriptorImageInfo descriptor;

	int mipLevels;

	void updateDescriptor();

	void destroy(VulkanDevice &device);

	void GenerateMipMaps(VulkanDevice& device, VkImage image, int width, int height, int depth, int layers, int mipLevels);

	static VkSampler CreateImageSampler(VulkanDevice &device, VkFilter mag = VK_FILTER_LINEAR, VkFilter min = VK_FILTER_LINEAR, VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				VkSamplerAddressMode textureWrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, float mipMapLodBias = 0.0f, bool useMipmaps = 1, int mipLevels = 0, bool anisotropy = 1, float maxAnisotropy = 8,
				VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
				
	static VkImageView CreateImageView(VulkanDevice& device, VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags, VkComponentMapping components, int mipLevels, int layers);
};


/**
* Load a 2D texture including all mip levels
*
* @param filename File to load (supports .ktx and .dds)
* @param format Vulkan format of the image data stored in the file
* @param device Vulkan device to create the texture on
* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
*
*/
class VulkanTexture2D : public VulkanTexture {
public:
	std::shared_ptr<Texture> texture;

	void loadFromTexture(
		VulkanDevice &device,
		std::shared_ptr<Texture> texture,
		VkFormat format,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool forceLinear = false,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1,
		bool wrapBorder = true);
};

class VulkanTexture2DArray : public VulkanTexture {
public:
	std::shared_ptr<TextureArray> textures;

	/**
	* Load a 2D texture array including all mip levels
	*
	* @param filename File to load (supports .ktx and .dds)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*
	*/
	void loadTextureArray(
		VulkanDevice &device,
		std::shared_ptr<TextureArray> textures,
		VkFormat format,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
};

class VulkanCubeMap : public VulkanTexture {
public:
	std::shared_ptr<CubeMap> cubeMap;

	/**
	* Load a cubemap texture including all mip levels from a single file
	*
	* @param filename File to load (supports .ktx and .dds)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*
	*/
	void loadFromTexture(
		VulkanDevice &device,
		std::shared_ptr<CubeMap> cubeMap,
		VkFormat format,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
};

class VulkanTextureDepthBuffer : public VulkanTexture {
public:
	void CreateDepthImage(VulkanDevice &device, VkFormat depthFormat, int width, int height);
};

class VulkanTextureManager {
public:
	VulkanTextureManager(VulkanDevice &device);
	~VulkanTextureManager();



private:
	VulkanDevice& device;

	std::vector<VulkanTexture> vulkanTextures;
};
