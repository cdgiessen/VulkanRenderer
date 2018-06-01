#pragma once

#include <vulkan/vulkan.h>
#include "Device.h"
#include "Descriptor.h"

#include "../resources/Texture.h"

class VulkanRenderer;

class VulkanTexture {
public:
	VulkanTexture(VulkanDevice& device);

	void updateDescriptor();

	void destroy();

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


	VmaImage image;

	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
	VkImageLayout textureImageLayout;

	//VkDescriptorImageInfo descriptor;
	DescriptorResource resource;

	VulkanDevice& device;

	Signal readyToUse;

};


class VulkanTexture2D : public VulkanTexture {
public:
	VulkanTexture2D(VulkanDevice& device);


	std::shared_ptr<Texture> texture;

	void loadFromTexture(
		std::shared_ptr<Texture> texture,
		VkFormat format,
		VulkanRenderer& renderer,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool forceLinear = false,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1,
		bool wrapBorder = true);
};

class VulkanTexture2DArray : public VulkanTexture {
public:
	VulkanTexture2DArray(VulkanDevice& device);

	std::shared_ptr<TextureArray> textures;

	void loadTextureArray(
		std::shared_ptr<TextureArray> textures,
		VkFormat format,
		VulkanRenderer& renderer,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
};

class VulkanCubeMap : public VulkanTexture {
public:
	VulkanCubeMap(VulkanDevice& device);

	std::shared_ptr<CubeMap> cubeMap;

	void loadFromTexture(
		std::shared_ptr<CubeMap> cubeMap,
		VkFormat format,
		VulkanRenderer& renderer,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
};

class VulkanTextureDepthBuffer : public VulkanTexture {
public:
	VulkanTextureDepthBuffer(VulkanDevice& device);

	void CreateDepthImage(VulkanRenderer& renderer, VkFormat depthFormat, int width, int height);
};

class VulkanTextureManager {
public:
	VulkanTextureManager(VulkanDevice &device);
	~VulkanTextureManager();

	std::shared_ptr<VulkanTexture2D> CreateTexture2D(
		std::shared_ptr<Texture> texture,
		VkFormat format,
		VulkanRenderer& renderer,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool forceLinear = false,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1,
		bool wrapBorder = true);

	std::shared_ptr<VulkanTexture2DArray> CreateTexture2DArray(
		std::shared_ptr<TextureArray> textures,
		VkFormat format,
		VulkanRenderer& renderer,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);

	std::shared_ptr<VulkanCubeMap> CreateCubeMap(std::shared_ptr<CubeMap> cubeMap,
		VkFormat format,
		VulkanRenderer& renderer,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		bool genMipMaps = false,
		int mipMapLevelsToGen = 1);
	std::shared_ptr<VulkanTextureDepthBuffer> CreateDepthImage(VulkanRenderer& renderer, VkFormat depthFormat, int width, int height);


private:
	VulkanDevice & device;

	std::vector<VulkanTexture> vulkanTextures;
};
