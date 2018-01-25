#pragma once

#include <vulkan/vulkan.h>
#include "VulkanDevice.hpp"
#include "VulkanDescriptor.hpp"

#include "../resources/Texture.h"

class VulkanTexture {
public:
	VulkanTexture();

	void updateDescriptor();

	void destroy(VulkanDevice &device);

	void GenerateMipMaps(VulkanDevice& device, VkImage image, int width, int height, 
		int depth, int layers, int mipLevels);

	static VkSampler CreateImageSampler(VulkanDevice &device, VkFilter mag = VK_FILTER_LINEAR, 
		VkFilter min = VK_FILTER_LINEAR, VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VkSamplerAddressMode textureWrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, 
		float mipMapLodBias = 0.0f, bool useMipmaps = true, int mipLevels = 1, 
		bool anisotropy = true, float maxAnisotropy = 8,
		VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
				
	static VkImageView CreateImageView(VulkanDevice& device, VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags, VkComponentMapping components, int mipLevels, int layers);
	
	
	VmaImage image;

	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
	VkImageLayout textureImageLayout;

	//VkDescriptorImageInfo descriptor;
	DescriptorResource resource;

};


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
