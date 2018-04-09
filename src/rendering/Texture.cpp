#include "Texture.hpp"

#include <algorithm>

#include "../../third-party/stb_image/stb_image.h"

#include "RenderTools.h"
#include "Initializers.hpp"

#include "../core/Logger.h"



VulkanTexture::VulkanTexture(VulkanDevice& device) : 
	device(device),
	resource(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {

}
VulkanTexture2D::VulkanTexture2D(VulkanDevice& device): VulkanTexture(device){}

VulkanTexture2DArray::VulkanTexture2DArray(VulkanDevice& device) : VulkanTexture(device) {}

VulkanCubeMap::VulkanCubeMap(VulkanDevice& device) : VulkanTexture(device) {}

VulkanTextureDepthBuffer::VulkanTextureDepthBuffer(VulkanDevice& device) : VulkanTexture(device) {}

void VulkanTexture::updateDescriptor()
{
	resource.FillResource(textureSampler, textureImageView, textureImageLayout);
	//descriptor.sampler = textureSampler;
	//descriptor.imageView = textureImageView;
	//descriptor.imageLayout = textureImageLayout;
}

void VulkanTexture::destroy() {
	device.DestroyVmaAllocatedImage(image);

	if(textureImageView != VK_NULL_HANDLE)
		vkDestroyImageView(device.device, textureImageView, nullptr);
	if (textureSampler != VK_NULL_HANDLE)
		vkDestroySampler(device.device, textureSampler, nullptr);
	
}

void VulkanTexture::GenerateMipMaps(VkImage image, int width, int height, int depth, int layers, int mipLevels) {
	// We copy down the whole mip chain doing a blit from mip-1 to mip
	// An alternative way would be to always blit from the first mip level and sample that one down
	VkCommandBuffer blitCmd = device.createCommandBuffer(device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Copy down mips from n-1 to n
	for (int32_t i = 1; i < mipLevels; i++)
	{

		VkImageBlit imageBlit{};

		// Source
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.layerCount = layers;
		imageBlit.srcSubresource.baseArrayLayer = 0;
		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
		imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
		imageBlit.srcOffsets[1].z = 1;

		// Destination
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.layerCount = layers;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.mipLevel = i;
		imageBlit.dstOffsets[1].x = int32_t(width >> i);
		imageBlit.dstOffsets[1].y = int32_t(height >> i);
		imageBlit.dstOffsets[1].z = 1;

		VkImageSubresourceRange mipSubRange =
			initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT, 1, layers);
		mipSubRange.baseMipLevel = i;


		// Transiton current mip level to transfer dest
		setImageLayout(
			blitCmd,
			image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			mipSubRange,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Blit from previous level
		vkCmdBlitImage(
			blitCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlit,
			VK_FILTER_LINEAR);

		// Transiton current mip level to transfer source for read in next iteration
		setImageLayout(
			blitCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			mipSubRange,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);
	
	}
	VkImageSubresourceRange subresourceRange = 
		initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, layers);
	
	// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
	setImageLayout(
		blitCmd,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		textureImageLayout,
		subresourceRange);

	device.flushCommandBuffer(blitCmd, device.graphics_queue);

}

VkSampler VulkanTexture::CreateImageSampler(VkFilter mag, VkFilter min, VkSamplerMipmapMode mipMapMode,
	VkSamplerAddressMode textureWrapMode, float mipMapLodBias, bool useMipMaps, int mipLevels, bool anisotropy, float maxAnisotropy,
	VkBorderColor borderColor) {

	// Create a defaultsampler
	VkSamplerCreateInfo samplerCreateInfo = initializers::samplerCreateInfo();
	samplerCreateInfo.magFilter = mag;
	samplerCreateInfo.minFilter = min;
	samplerCreateInfo.mipmapMode = mipMapMode;
	samplerCreateInfo.addressModeU = textureWrapMode;
	samplerCreateInfo.addressModeV = textureWrapMode;
	samplerCreateInfo.addressModeW = textureWrapMode;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = (useMipMaps) ? mipLevels : 0.0f;// Max level-of-detail should match mip level count
	samplerCreateInfo.anisotropyEnable = anisotropy; // Enable anisotropic filtering
	samplerCreateInfo.maxAnisotropy = (anisotropy) ? device.physical_device_properties.limits.maxSamplerAnisotropy : 1;
	samplerCreateInfo.borderColor = borderColor;
	
	VkSampler sampler;
	VK_CHECK_RESULT(vkCreateSampler(device.device, &samplerCreateInfo, nullptr, &sampler));

	return sampler;
}

VkImageView VulkanTexture::CreateImageView(VkImage image, VkImageViewType viewType, 
	VkFormat format, VkImageAspectFlags aspectFlags, VkComponentMapping components, int mipLevels, int layers) 
{
	VkImageViewCreateInfo viewInfo = initializers::imageViewCreateInfo();
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.components = components;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;
	
	VkImageView imageView;
	VK_CHECK_RESULT(vkCreateImageView(device.device, &viewInfo, nullptr, &imageView));

	return imageView;

}

void AlignedTextureMemcpy(int layers, int dst_layer_width, 
	int height, int width, 
	int src_row_width, int dst_row_width, 
	char* src, char* dst ) {
	
	int offset = 0;
	int texOff = 0;
	for (int i = 0; i < layers; i++) {
		for (int r = 0; r < height; r++) {
			memcpy(dst + offset,
				src + texOff,
				width * 4);
			offset += dst_row_width;
			texOff += src_row_width;
		}
		if(texOff < dst_layer_width * i){
			texOff = dst_layer_width * i;
		}
	}
}


void VulkanTexture2D::loadFromTexture(
	std::shared_ptr<Texture> texture,
	VkFormat format,
	VkCommandBuffer transferCmdBuf,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout,
	bool forceLinear,
	bool genMipMaps,
	int mipMapLevelsToGen,
	bool wrapBorder)
{
	VmaImage stagingImage;

	this->texture = texture;
	int maxMipMapLevelsPossible = (int)floor(log2(std::max(texture->width, texture->height))) + 1;
	int mipLevels = genMipMaps ? mipMapLevelsToGen : 1;

	// Get device properites for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.physical_device, format, &formatProperties);

	// Mip-chain generation requires support for blit source and destination
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

	// Only use linear tiling if requested (and supported by the device)
	// Support for linear tiling is mostly limited, so prefer to use
	// optimal tiling instead
	// On most implementations linear tiling will only support a very
	// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
	VkBool32 useStaging = !forceLinear;

	VkExtent3D imageExtent = { texture->width, texture->height, 1 };

	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D,
		format,
		(uint32_t)mipLevels,
		(uint32_t)1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		imageExtent,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT 
			| VK_IMAGE_USAGE_TRANSFER_SRC_BIT 
			| VK_IMAGE_USAGE_SAMPLED_BIT);

	VkImageCreateInfo stagingImageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D,
		format,
		(uint32_t)mipLevels,
		(uint32_t)1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_LINEAR,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		imageExtent,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT	);

	VmaAllocationInfo stagingImageAllocInfo = {};
	device.CreateStagingImage2D(stagingImageCreateInfo, stagingImage);
	
	VkSubresourceLayout sub;

	const VkImageSubresource imSub = initializers::imageSubresourceCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT);
	vkGetImageSubresourceLayout(device.device, stagingImage.image, &imSub, &sub);

	char* stagingPointer = (char*)stagingImage.allocationInfo.pMappedData;
	AlignedTextureMemcpy(1, 0, texture->height, texture->width, texture->width * 4, sub.rowPitch, (char*)texture->pixels.data(), stagingPointer);
	
	device.CreateImage2D(imageCreateInfo, image);

	VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT, mipLevels); {};

	setImageLayout(
		transferCmdBuf,
		image.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	setImageLayout(
		transferCmdBuf,
		stagingImage.image,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	VkImageCopy imageCopy;
	imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopy.srcSubresource.baseArrayLayer = 0;
	imageCopy.srcSubresource.mipLevel = 0;
	imageCopy.srcSubresource.layerCount = 1;

	imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopy.dstSubresource.baseArrayLayer = 0;
	imageCopy.dstSubresource.mipLevel = 0;
	imageCopy.dstSubresource.layerCount = 1;

	imageCopy.srcOffset.x = 0;
	imageCopy.srcOffset.y = 0;
	imageCopy.srcOffset.z = 0;
	imageCopy.dstOffset.x = 0;
	imageCopy.dstOffset.y = 0;
	imageCopy.dstOffset.z = 0;

	imageCopy.extent.width = texture->width;
	imageCopy.extent.height = texture->height;
	imageCopy.extent.depth = 1;

	vkCmdCopyImage(
		transferCmdBuf,
		stagingImage.image, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		image.image, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &imageCopy);

	
	setImageLayout(
		transferCmdBuf,
		image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	this->textureImageLayout = imageLayout;

	device.SubmitTransferCommandBufferAndWait(transferCmdBuf);

	device.DestroyVmaAllocatedImage(stagingImage);


	GenerateMipMaps(image.image, texture->width, texture->height, 1, 1, mipLevels);

	// With mip mapping and anisotropic filtering
	if (device.physical_device_features.samplerAnisotropy)
	{
		// Create a defaultsampler
		if (wrapBorder)
			textureSampler = CreateImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 
				VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.0f, 
				true, mipLevels, true, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
		else
			textureSampler = CreateImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 
				VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f,
				true, mipLevels, true, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
	}
	else {
		// Create a defaultsampler
		if (wrapBorder)
			textureSampler = CreateImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 
				VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.0f, 
				true, mipLevels, false, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
		else				 
			textureSampler = CreateImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 
				VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, 
				true, mipLevels, false, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
	}
	
	textureImageView = CreateImageView(image.image, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT,
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		(useStaging) ? mipLevels : 1, 1);

	// Update descriptor image info member that can be used for setting up descriptor sets
	updateDescriptor();



	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	//VkImageViewCreateInfo viewCreateInfo = {};
	//viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	//viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	//viewCreateInfo.format = format;
	//viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	//viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	//// Linear tiling usually won't support mip maps
	//// Only set mip map count if optimal tiling is used
	//viewCreateInfo.subresourceRange.levelCount = (useStaging) ? mipLevels : 1;
	//viewCreateInfo.image = image.image;
	//VK_CHECK_RESULT(vkCreateImageView(device.device, &viewCreateInfo, nullptr, &textureImageView));

}

void VulkanTexture2DArray::loadTextureArray(
	std::shared_ptr<TextureArray> textures,
	VkFormat format,
	VkCommandBuffer transferCmdBuf,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout,
	bool genMipMaps,
	int mipMapLevelsToGen)
{
	VmaImage stagingImage;

	this->textures = textures;
	int mipLevels = genMipMaps ? mipMapLevelsToGen : 1;

	VkExtent3D imageExtent = { textures->width, textures->height, 1};

	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D,
		format,
		(uint32_t)mipLevels,
		(uint32_t)textures->layerCount,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		imageExtent,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT);

	VkImageCreateInfo stagingImageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D,
		format,
		(uint32_t)mipLevels,
		(uint32_t)textures->layerCount,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_LINEAR,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		imageExtent,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	VmaAllocationInfo stagingImageAllocInfo = {};
	device.CreateStagingImage2D(stagingImageCreateInfo, stagingImage);
	
	VkSubresourceLayout sub;

	const VkImageSubresource imSub = initializers::imageSubresourceCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT);
	vkGetImageSubresourceLayout(device.device, stagingImage.image, &imSub, &sub);

	char* stagingPointer = (char*)stagingImage.allocationInfo.pMappedData;
	AlignedTextureMemcpy(textures->layerCount, sub.arrayPitch, textures->height, textures->width, 
		textures->width * 4, sub.rowPitch, 
		(char*)textures->pixels.data(), stagingPointer);

	device.CreateImage2D(imageCreateInfo, image);

	
	VkOffset3D dstCopyOffset = { 0, 0, 0 };
	VkOffset3D srcCopyOffset = { 0, 0, 0 };

	
	VkImageCopy imageCopyRegion = {};
	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.dstSubresource.baseArrayLayer = 0;
	imageCopyRegion.dstSubresource.layerCount = textures->layerCount;
	imageCopyRegion.dstSubresource.mipLevel = 0;
	imageCopyRegion.dstOffset = dstCopyOffset;

	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.srcSubresource.baseArrayLayer = 0;
	imageCopyRegion.srcSubresource.layerCount = textures->layerCount;
	imageCopyRegion.srcSubresource.mipLevel = 0;
	imageCopyRegion.srcOffset = srcCopyOffset;
	imageCopyRegion.extent = imageExtent;
	
	
	VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, textures->layerCount);

	setImageLayout(
		transferCmdBuf,
		image.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	setImageLayout(
		transferCmdBuf,
		stagingImage.image,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	vkCmdCopyImage(
		transferCmdBuf,
		stagingImage.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(1),
		&imageCopyRegion);

	setImageLayout(
		transferCmdBuf,
		image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	this->textureImageLayout = imageLayout;
	device.SubmitTransferCommandBufferAndWait(transferCmdBuf);

	device.DestroyVmaAllocatedImage(stagingImage);

	GenerateMipMaps(image.image, textures->width, textures->height, 1, textures->layerCount, mipLevels);

	textureSampler = CreateImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.0f, true, mipLevels,
		true, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	textureImageView = CreateImageView(image.image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, 
		VK_IMAGE_ASPECT_COLOR_BIT, 
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		mipLevels, textures->layerCount);

	// Update descriptor image info member that can be used for setting up descriptor sets
	updateDescriptor();

	//// Create image view
	//VkImageViewCreateInfo viewCreateInfo = initializers::imageViewCreateInfo();
	//viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	//viewCreateInfo.format = format;
	//viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	//viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	//viewCreateInfo.subresourceRange.layerCount = textures->layerCount;
	//viewCreateInfo.subresourceRange.levelCount = mipLevels;
	//viewCreateInfo.image = image.image;
	//VK_CHECK_RESULT(vkCreateImageView(device.device, &viewCreateInfo, nullptr, &textureImageView));
}
	

void VulkanCubeMap::loadFromTexture(
	std::shared_ptr<CubeMap> cubeMap,
	VkFormat format,
	VkCommandBuffer transferCmdBuf,
	
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout,
	bool genMipMaps,
	int mipMapLevelsToGen)
{
	this->cubeMap = cubeMap;
	int mipLevels = genMipMaps ? mipMapLevelsToGen : 1;
	
	VmaImage stagingImage;

	VkExtent3D imageExtent = { cubeMap->width, cubeMap->height, 1 };

	VkImageCreateInfo stagingImageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D,
		format,
		(uint32_t)1,
		(uint32_t)6,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_LINEAR,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		imageExtent,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	stagingImageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


	VmaAllocationInfo stagingImageAllocInfo = {};
	
	device.CreateStagingImage2D(stagingImageCreateInfo, stagingImage);
	
	VkSubresourceLayout sub;

	const VkImageSubresource imSub = initializers::imageSubresourceCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT);
	vkGetImageSubresourceLayout(device.device, stagingImage.image, &imSub, &sub);
	
	char* stagingPointer = (char*)stagingImage.allocationInfo.pMappedData;
	AlignedTextureMemcpy(6, 0, cubeMap->height, cubeMap->width, cubeMap->width * 4, sub.rowPitch, (char*)cubeMap->pixels.data(), stagingPointer);

	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D,
		format,
		(uint32_t)mipLevels,
		(uint32_t)6,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		imageExtent,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT);
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	device.CreateImage2D(imageCreateInfo, image);

	/*VkImageSubresource stagingImageSubresource = {};
	stagingImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	stagingImageSubresource.mipLevel = 0;
	stagingImageSubresource.arrayLayer = 0;
	
	VkSubresourceLayout stagingImageLayout = {};
	vkGetImageSubresourceLayout(device.device, stagingImage, &stagingImageSubresource, &stagingImageLayout);

	char* const pMipLevelData = (char*)stagingImageAllocInfo.pMappedData + stagingImageLayout.offset;
	uint8_t* pRowData = (uint8_t*)pMipLevelData;
	for (uint32_t y = 0; y < cubeMap->height; ++y)
	{
		uint32_t* pPixelData = (uint32_t*)pRowData;
		for (uint32_t x = 0; x < cubeMap->width; ++x)
		{
			*pPixelData =
				((x & 0x18) == 0x08 ? 0x000000FF : 0x00000000) |
				((x & 0x18) == 0x10 ? 0x0000FFFF : 0x00000000) |
				((y & 0x18) == 0x08 ? 0x0000FF00 : 0x00000000) |
				((y & 0x18) == 0x10 ? 0x00FF0000 : 0x00000000);
			++pPixelData;
		}
		pRowData += stagingImageLayout.rowPitch;
	}
	*/
	VkOffset3D copyOffset = { 0, 0, 0 };

	VkImageSubresourceRange subresourceRange = 
		initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 6);

	VkImageCopy imageCopyRegion = {};
	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.dstSubresource.baseArrayLayer = 0;
	imageCopyRegion.dstSubresource.layerCount = 6;
	imageCopyRegion.dstSubresource.mipLevel = 0;
	imageCopyRegion.dstOffset = copyOffset;// VkOffset3D{ textures->width, textures->height, 1 };

	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.srcSubresource.baseArrayLayer = 0;
	imageCopyRegion.srcSubresource.layerCount = 6;
	imageCopyRegion.srcSubresource.mipLevel = 0;
	imageCopyRegion.srcOffset = copyOffset;// VkOffset3D{ textures->width, textures->height, 1 };
	
	imageCopyRegion.extent = imageExtent;
	
	setImageLayout(
		transferCmdBuf,
		image.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	setImageLayout(
		transferCmdBuf,
		stagingImage.image,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	vkCmdCopyImage(
		transferCmdBuf,
		stagingImage.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(1),
		&imageCopyRegion);

	setImageLayout(
		transferCmdBuf,
		image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		subresourceRange);

	this->textureImageLayout = imageLayout;
	device.SubmitTransferCommandBufferAndWait(transferCmdBuf);

	device.DestroyVmaAllocatedImage(stagingImage);

	//GenerateMipMaps(device, cubeMap->width, cubeMap->height, 6, mipLevels);

	textureSampler = CreateImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 
		VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, true, mipLevels, true, 8,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	textureImageView = CreateImageView(image.image, VK_IMAGE_VIEW_TYPE_CUBE, format, VK_IMAGE_ASPECT_COLOR_BIT,
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		mipLevels, 6);

	updateDescriptor();

	// Create image view
	//VkImageViewCreateInfo viewCreateInfo = initializers::imageViewCreateInfo();
	//viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	//viewCreateInfo.format = format;
	//viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	//viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	//viewCreateInfo.subresourceRange.layerCount = 6;
	//viewCreateInfo.subresourceRange.levelCount = mipLevels;
	//viewCreateInfo.image = image.image;
	//VK_CHECK_RESULT(vkCreateImageView(device.device, &viewCreateInfo, nullptr, &textureImageView));
	
}

void VulkanTextureDepthBuffer::CreateDepthImage(VkFormat depthFormat, int width, int height) {

	VkImageCreateInfo imageInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D, depthFormat, 1, 1, 
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL, 
		VK_SHARING_MODE_EXCLUSIVE, 
		VK_IMAGE_LAYOUT_PREINITIALIZED, 
		VkExtent3D{ (uint32_t)width, (uint32_t)height, (uint32_t)1 }, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	device.CreateDepthImage(imageInfo, image);

	textureImageView = VulkanTexture::CreateImageView(image.image, VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, 1, 1);

	VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

	VkCommandBuffer copyBuf = device.createCommandBuffer(device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	setImageLayout(copyBuf, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, subresourceRange);

	device.flushCommandBuffer(copyBuf, device.graphics_queue, true);
}
	
VulkanTextureManager::VulkanTextureManager(VulkanDevice& device) : device(device)
{
}

VulkanTextureManager::~VulkanTextureManager()
{
	for (auto tex : vulkanTextures) {
		tex.destroy();
	}
}

std::shared_ptr<VulkanTexture2D> VulkanTextureManager::CreateTexture2D(
	std::shared_ptr<Texture> texture,
	VkFormat format,
	VkCommandBuffer transferBuf,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout,
	bool forceLinear,
	bool genMipMaps ,
	int mipMapLevelsToGen ,
	bool wrapBorder) {
	return std::make_shared<VulkanTexture2D>(device);
}

std::shared_ptr<VulkanTexture2DArray> VulkanTextureManager::CreateTexture2DArray(
	std::shared_ptr<TextureArray> textures,
	VkFormat format,
	VkCommandBuffer transferBuf,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout,
	bool genMipMaps,
	int mipMapLevelsToGen) {
	return std::make_shared<VulkanTexture2DArray>(device);
}

std::shared_ptr<VulkanCubeMap> VulkanTextureManager::CreateCubeMap(
	std::shared_ptr<CubeMap> cubeMap,
	VkFormat format,
	VkCommandBuffer transferBuf,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout,
	bool genMipMaps,
	int mipMapLevelsToGen) {
	return std::make_shared<VulkanCubeMap>(device);
}
std::shared_ptr<VulkanTextureDepthBuffer> VulkanTextureManager::CreateDepthImage(
	VkFormat depthFormat, int width, int height) {
	return std::make_shared<VulkanTextureDepthBuffer>(device);
}
