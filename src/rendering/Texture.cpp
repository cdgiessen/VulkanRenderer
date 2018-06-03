#include "Texture.h"

#include <algorithm>

#include "../../third-party/stb_image/stb_image.h"

#include "Initializers.h"
#include "RenderTools.h"
#include "Renderer.h"

#include "../core/Logger.h"

VulkanTexture::VulkanTexture(VulkanDevice &device)
	: device(device), resource(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {

	readyToUse = std::make_shared<bool>(false);
}
VulkanTexture2D::VulkanTexture2D(VulkanDevice &device)
	: VulkanTexture(device) {}

VulkanTexture2DArray::VulkanTexture2DArray(VulkanDevice &device)
	: VulkanTexture(device) {}

VulkanCubeMap::VulkanCubeMap(VulkanDevice &device) : VulkanTexture(device) {}

VulkanTextureDepthBuffer::VulkanTextureDepthBuffer(VulkanDevice &device)
	: VulkanTexture(device) {}

void VulkanTexture::updateDescriptor() {
	resource.FillResource(textureSampler, textureImageView, textureImageLayout);
	// descriptor.sampler = textureSampler;
	// descriptor.imageView = textureImageView;
	// descriptor.imageLayout = textureImageLayout;
}

void VulkanTexture::destroy() {
	device.DestroyVmaAllocatedImage(image);

	if (textureImageView != VK_NULL_HANDLE)
		vkDestroyImageView(device.device, textureImageView, nullptr);
	if (textureSampler != VK_NULL_HANDLE)
		vkDestroySampler(device.device, textureSampler, nullptr);
}

void GenerateMipMaps(VkCommandBuffer cmdBuf, VkImage image, 
	VkImageLayout finalImageLayout,
	int width, int height, int depth,
	int layers, int mipLevels) {
	// We copy down the whole mip chain doing a blit from mip-1 to mip
	// An alternative way would be to always blit from the first mip level and
	// sample that one down

	// VkCommandBuffer blitCmd = renderer.GetSingleUseGraphicsCommandBuffer();

	// Copy down mips from n-1 to n
	for (int32_t i = 1; i < mipLevels; i++) {

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
			initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT,
				1, layers);
		mipSubRange.baseMipLevel = i;

		// Transiton current mip level to transfer dest
		setImageLayout(cmdBuf, image, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipSubRange,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Blit from previous level
		vkCmdBlitImage(cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit,
			VK_FILTER_LINEAR);

		// Transiton current mip level to transfer source for read in next iteration
		setImageLayout(cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipSubRange,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);
	}

	VkImageSubresourceRange subresourceRange =
		initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT,
			mipLevels, layers);

	// After the loop, all mip layers are in TRANSFER_SRC layout, so transition
	// all to SHADER_READ

if(finalImageLayout == VK_IMAGE_LAYOUT_UNDEFINED) Log::Debug << "final layout is undefined!\n";
	setImageLayout(cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		finalImageLayout, subresourceRange);

	// renderer.SubmitGraphicsCommandBufferAndWait(blitCmd);
}

VkSampler VulkanTexture::CreateImageSampler(
	VkFilter mag, VkFilter min, VkSamplerMipmapMode mipMapMode,
	VkSamplerAddressMode textureWrapMode, float mipMapLodBias, bool useMipMaps,
	int mipLevels, bool anisotropy, float maxAnisotropy,
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
	samplerCreateInfo.maxLod =
		(useMipMaps) ? mipLevels
		: 0.0f; // Max level-of-detail should match mip level count
	samplerCreateInfo.anisotropyEnable =
		anisotropy; // Enable anisotropic filtering
	samplerCreateInfo.maxAnisotropy =
		(anisotropy)
		? device.physical_device_properties.limits.maxSamplerAnisotropy
		: 1;
	samplerCreateInfo.borderColor = borderColor;

	VkSampler sampler;
	VK_CHECK_RESULT(
		vkCreateSampler(device.device, &samplerCreateInfo, nullptr, &sampler));

	return sampler;
}

VkImageView VulkanTexture::CreateImageView(VkImage image,
	VkImageViewType viewType,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	VkComponentMapping components,
	int mipLevels, int layers) {
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
	VK_CHECK_RESULT(
		vkCreateImageView(device.device, &viewInfo, nullptr, &imageView));

	return imageView;
}

void AlignedTextureMemcpy(int layers, int dst_layer_width, int height,
	int width, int src_row_width, int dst_row_width,
	char *src, char *dst) {

	int offset = 0;
	int texOff = 0;
	for (int i = 0; i < layers; i++) {
		for (int r = 0; r < height; r++) {
			memcpy(dst + offset, src + texOff, width * 4);
			offset += dst_row_width;
			texOff += src_row_width;
		}
		if (texOff < dst_layer_width * i) {
			texOff = dst_layer_width * i;
		}
	}
}

void SetLayoutTransferRegions(
	VkCommandBuffer transferCmdBuf, VkImage image, VkBuffer stagingBuffer,
	const VkImageSubresourceRange subresourceRange,
	std::vector<VkBufferImageCopy> bufferCopyRegions) {

	setImageLayout(transferCmdBuf, image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	vkCmdCopyBufferToImage(transferCmdBuf, stagingBuffer, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		bufferCopyRegions.size(), bufferCopyRegions.data());

	setImageLayout(transferCmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
}

void VulkanTexture2D::loadFromTexture(std::shared_ptr<Texture> texture,
	VkFormat format, VulkanRenderer &renderer,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout,
	bool forceLinear, bool genMipMaps,
	int mipMapLevelsToGen, bool wrapBorder) {
	// VmaImage stagingImage;

	this->texture = texture;
	int maxMipMapLevelsPossible =
		(int)floor(log2(std::max(texture->width, texture->height))) + 1;
	int mipLevels = genMipMaps ? mipMapLevelsToGen : 1;
	this->textureImageLayout = imageLayout;

	// Get device properites for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device.physical_device, format,
		&formatProperties);

	// Mip-chain generation requires support for blit source and destination
	assert(formatProperties.optimalTilingFeatures &
		VK_FORMAT_FEATURE_BLIT_SRC_BIT);
	assert(formatProperties.optimalTilingFeatures &
		VK_FORMAT_FEATURE_BLIT_DST_BIT);

	// Only use linear tiling if requested (and supported by the device)
	// Support for linear tiling is mostly limited, so prefer to use
	// optimal tiling instead
	// On most implementations linear tiling will only support a very
	// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
	VkBool32 useStaging = !forceLinear;

	VkExtent3D imageExtent = { texture->width, texture->height, 1 };

	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D, format, (uint32_t)mipLevels, (uint32_t)1,
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_UNDEFINED, imageExtent,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT);

	 /*VkImageCreateInfo stagingImageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D,
		format,
		(uint32_t)mipLevels,
		(uint32_t)1,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_LINEAR,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		imageExtent,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT	); */

		// VmaAllocationInfo stagingImageAllocInfo = {};
		// device.CreateStagingImage2D(stagingImageCreateInfo, stagingImage)
		// VkSubresourceLayout sub
		// const VkImageSubresource imSub =
		// initializers::imageSubresourceCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT);
		// vkGetImageSubresourceLayout(device.device, stagingImage.image, &imSub,
		// &sub);
		//
		// char* stagingPointer = (char*)stagingImage.allocationInfo.pMappedData;
		// AlignedTextureMemcpy(1, 0, texture->height, texture->width, texture->width
		// * 4, sub.rowPitch, (char*)texture->pixels.data(), stagingPointer)
		// VmaBuffer stagingBuffer;
		// device.CreateStagingImageBuffer(stagingBuffer, texture->pixels.data(),
		// texture->texImageSize)
		// device.CreateImage2D(imageCreateInfo, image)
		// VkCommandBuffer transferCmdBuf = renderer.GetTransferCommandBuffer()
		// transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
		// VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		// copyBufferToImage(stagingBuffer, textureImage,
		// static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		// transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		// VkImageSubresourceRange subresourceRange =
		//	initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT,
		// mipLevels)
		// setImageLayout(
		//	transferCmdBuf,
		//	image.image,
		//	VK_IMAGE_LAYOUT_UNDEFINED,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	subresourceRange);
		//
		// VkBufferImageCopy bufferCopyRegion = {};
		//	bufferCopyRegion.imageSubresource.aspectMask =
		// VK_IMAGE_ASPECT_COLOR_BIT; 	bufferCopyRegion.imageSubresource.mipLevel
		// = 0; 	bufferCopyRegion.imageSubresource.baseArrayLayer = 0; //layer
		// count
		//	bufferCopyRegion.imageSubresource.layerCount = 1;
		//	bufferCopyRegion.imageExtent.width =
		// static_cast<uint32_t>(texture->width);
		// bufferCopyRegion.imageExtent.height =
		// static_cast<uint32_t>(texture->height);
		// bufferCopyRegion.imageExtent.depth = 1; 	bufferCopyRegion.bufferOffset =
		// 0
		// vkCmdCopyBufferToImage(transferCmdBuf, stagingBuffer.buffer, image.image,
		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1/*copy region counts*/,
		// &bufferCopyRegion/*data*/)
		// setImageLayout(
		//	transferCmdBuf,
		//	image.image,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		//	subresourceRange)
		// SetLayoutCopyImage(transferCmdBuf, image.image, stagingImage.image,
		// mipLevels, imageExtent)
		// renderer.SubmitTransferCommandBufferAndWait(transferCmdBuf);
		
	VulkanBufferStagingResource buffer(device, texture->pixels.data(),
		texture->texImageSize);

	device.CreateImage2D(imageCreateInfo, image);

	VkImageSubresourceRange subresourceRange =
		initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT,
			mipLevels, 1);

	std::vector<VkBufferImageCopy> bufferCopyRegions;

	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(texture->width);
	bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(texture->height);
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;
	bufferCopyRegions.push_back(bufferCopyRegion);
	// Increase offset into staging buffer for next level / face

	VkSemaphore sem;

	TransferCommandWork transfer;
	VkImage rawImage = image.image;//not sure why I have to do this. Lambdas are weird
	//Log::Debug << "Tex2D handle value "<< rawImage << "\n";

	//VkEvent waitEvent;
	//VkEventCreateInfo eventInfo = initializers::eventCreateInfo();
	//VK_CHECK_RESULT(vkCreateEvent(device.device, &eventInfo, nullptr, &waitEvent));

	//vkCmdSetEvent(cmdBuf, waitEvent, VK_PIPELINE_STAGE_TRANSFER_BIT);

	// vkCmdWaitEvents(cmdBuf, 1,
	//     &waitEvent,
	//     VK_PIPELINE_STAGE_TRANSFER_BIT,
	//     VK_PIPELINE_STAGE_TRANSFER_BIT,
	//     0, nullptr,
	//     0, nullptr,
	//     0, nullptr);
	//vkDestroyEvent(device.device, waitEvent, nullptr);
		

	transfer.work = std::function<void(const VkCommandBuffer)>(
		[=](const VkCommandBuffer cmdBuf) {
		SetLayoutTransferRegions(cmdBuf, rawImage, buffer.buffer.buffer,
			subresourceRange, bufferCopyRegions);

		//setTransferBarrier(cmdBuf, buffer.buffer.buffer, buffer.Size());

		GenerateMipMaps(cmdBuf, rawImage, imageLayout, texture->width, texture->height, 1, 1, mipLevels);
		
	});
	transfer.buffersToClean.push_back(buffer);
	transfer.flags.push_back(readyToUse);

	renderer.SubmitTransferWork(std::move(transfer));

	this->textureImageLayout = imageLayout;

	// device.DestroyVmaAllocatedImage(stagingImage);
	// device.DestroyVmaAllocatedBuffer(stagingBuffer);

	// CommandBufferWork mipMapGen;
	// mipMapGen.work = [&](VkCommandBuffer cmdBuf) {
	//	GenerateMipMaps(cmdBuf, image.image, texture->width, texture->height, 1, 1, mipLevels);
	//};
	// work.flags.push_back();

	// renderer.SubmitGraphicsSetupWork(std::move(mipMapGen));

	// With mip mapping and anisotropic filtering
	textureSampler = CreateImageSampler(
		VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		wrapBorder ? VK_SAMPLER_ADDRESS_MODE_REPEAT
		: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		0.0f, true, mipLevels,
		device.physical_device_features.samplerAnisotropy ? true : false, 8,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	textureImageView = CreateImageView(
		image.image, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT,
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
						   VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		(useStaging) ? mipLevels : 1, 1);

	// Update descriptor image info member that can be used for setting up
	// descriptor sets
	updateDescriptor();

	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	// VkImageViewCreateInfo viewCreateInfo = {};
	// viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	// viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	// viewCreateInfo.format = format;
	// viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
	// VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	// viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
	// };
	//// Linear tiling usually won't support mip maps
	//// Only set mip map count if optimal tiling is used
	// viewCreateInfo.subresourceRange.levelCount = (useStaging) ? mipLevels : 1;
	// viewCreateInfo.image = image.image;
	// VK_CHECK_RESULT(vkCreateImageView(device.device, &viewCreateInfo, nullptr,
	// &textureImageView));
}

void VulkanTexture2DArray::loadTextureArray(
	std::shared_ptr<TextureArray> textures, VkFormat format,
	VulkanRenderer &renderer, VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout, bool genMipMaps, int mipMapLevelsToGen) {
	// VmaImage stagingImage;

	this->textures = textures;
	int mipLevels = genMipMaps ? mipMapLevelsToGen : 1;
	this->textureImageLayout = imageLayout;


	VkExtent3D imageExtent = { textures->width, textures->height, 1 };

	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D, format, (uint32_t)mipLevels,
		(uint32_t)textures->layerCount, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL, VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_UNDEFINED, imageExtent,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT);

	// VkImageCreateInfo stagingImageCreateInfo = initializers::imageCreateInfo(
	//	VK_IMAGE_TYPE_2D,
	//	format,
	//	(uint32_t)mipLevels,
	//	(uint32_t)textures->layerCount,
	//	VK_SAMPLE_COUNT_1_BIT,
	//	VK_IMAGE_TILING_LINEAR,
	//	VK_SHARING_MODE_EXCLUSIVE,
	//	VK_IMAGE_LAYOUT_PREINITIALIZED,
	//	imageExtent,
	//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	// VmaAllocationInfo stagingImageAllocInfo = {};
	// device.CreateStagingImage2D(stagingImageCreateInfo, stagingImage);

	// VkSubresourceLayout sub;
	//
	// const VkImageSubresource imSub =
	// initializers::imageSubresourceCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT);
	// vkGetImageSubresourceLayout(device.device, stagingImage.image, &imSub,
	// &sub);
	//
	// char* stagingPointer = (char*)stagingImage.allocationInfo.pMappedData;
	// AlignedTextureMemcpy(textures->layerCount, sub.arrayPitch,
	// textures->height, textures->width, 	textures->width * 4,
	// sub.rowPitch,
	//	(char*)textures->pixels.data(), stagingPointer);

	// VmaBuffer stagingBuffer;
	// device.CreateStagingImageBuffer(stagingBuffer, textures->pixels.data(),
	// textures->texImageSize);

	VulkanBufferStagingResource buffer(device, textures->pixels.data(),
		textures->texImageSize);

	device.CreateImage2D(imageCreateInfo, image);

	VkImageSubresourceRange subresourceRange =
		initializers::imageSubresourceRangeCreateInfo(
			VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, textures->layerCount);

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t layer = 0; layer < textures->layerCount; layer++) {
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(textures->width);
		bufferCopyRegion.imageExtent.height =
			static_cast<uint32_t>(textures->height);
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = offset;
		bufferCopyRegions.push_back(bufferCopyRegion);
		// Increase offset into staging buffer for next level / face
		offset += textures->texImageSizePerTex;
	}

	TransferCommandWork work;
	VkImage rawImage = image.image;//not sure why I have to do this. Lambdas are weird

	work.work = std::function<void(const VkCommandBuffer)>(
		[=](const VkCommandBuffer cmdBuf) {
		SetLayoutTransferRegions(cmdBuf, rawImage, buffer.buffer.buffer,
			subresourceRange, bufferCopyRegions);

		//setTransferBarrier(cmdBuf, buffer.buffer.buffer, buffer.Size());

		GenerateMipMaps(cmdBuf, rawImage, imageLayout,
			textures->width, textures->height, 1, textures->layerCount, mipLevels);
	});
	work.buffersToClean.push_back(buffer);
	work.flags.push_back(readyToUse);

	renderer.SubmitTransferWork(std::move(work));

	// VkCommandBuffer transferCmdBuf = renderer.GetTransferCommandBuffer();

	// setImageLayout(
	//	transferCmdBuf,
	//	image.image,
	//	VK_IMAGE_LAYOUT_UNDEFINED,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	subresourceRange);

	// vkCmdCopyBufferToImage(transferCmdBuf,
	//	buffer.buffer.buffer, image.image,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	bufferCopyRegions.size(), bufferCopyRegions.data());

	// setImageLayout(
	//	transferCmdBuf,
	//	image.image,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	//	subresourceRange);

	// SetLayoutCopyImageArray(transferCmdBuf, image.image, stagingImage.image,
	// mipLevels, textures->layerCount, imageExtent);

	// renderer.SubmitTransferCommandBufferAndWait(transferCmdBuf);

	// device.DestroyVmaAllocatedImage(stagingImage);

	this->textureImageLayout = imageLayout;

	// CommandBufferWork mipMapGen;
	// mipMapGen.work = [&](VkCommandBuffer cmdBuf) {
	//	GenerateMipMaps(cmdBuf, image.image, textures->width, textures->height, 1, textures->layerCount, mipLevels);
	//};

	// renderer.SubmitGraphicsSetupWork(std::move(mipMapGen));

	// GenerateMipMaps(renderer, image.image, textures->width, textures->height,
	// 1, textures->layerCount, mipLevels);

	textureSampler = CreateImageSampler(
		VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.0f, true, mipLevels, true, 8,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	textureImageView = CreateImageView(
		image.image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
						   VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		mipLevels, textures->layerCount);

	// Update descriptor image info member that can be used for setting up
	// descriptor sets
	updateDescriptor();

	//// Create image view
	// VkImageViewCreateInfo viewCreateInfo = initializers::imageViewCreateInfo();
	// viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	// viewCreateInfo.format = format;
	// viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
	// VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	// viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
	// }; viewCreateInfo.subresourceRange.layerCount = textures->layerCount;
	// viewCreateInfo.subresourceRange.levelCount = mipLevels;
	// viewCreateInfo.image = image.image;
	// VK_CHECK_RESULT(vkCreateImageView(device.device, &viewCreateInfo, nullptr,
	// &textureImageView));
}

void VulkanCubeMap::loadFromTexture(std::shared_ptr<CubeMap> cubeMap,
	VkFormat format, VulkanRenderer &renderer,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout, bool genMipMaps,
	int mipMapLevelsToGen) 
{
	this->cubeMap = cubeMap;
	int mipLevels = genMipMaps ? mipMapLevelsToGen : 1;
	this->textureImageLayout = imageLayout;

	// VmaImage stagingImage;

	VkExtent3D imageExtent = { cubeMap->width, cubeMap->height, 1 };

	// VkImageCreateInfo stagingImageCreateInfo = initializers::imageCreateInfo(
	//	VK_IMAGE_TYPE_2D,
	//	format,
	//	(uint32_t)1,
	//	(uint32_t)6,
	//	VK_SAMPLE_COUNT_1_BIT,
	//	VK_IMAGE_TILING_LINEAR,
	//	VK_SHARING_MODE_EXCLUSIVE,
	//	VK_IMAGE_LAYOUT_PREINITIALIZED,
	//	imageExtent,
	//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	// stagingImageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	// VmaAllocationInfo stagingImageAllocInfo = {};

	// device.CreateStagingImage2D(stagingImageCreateInfo, stagingImage);

	// VkSubresourceLayout sub;
	//
	// const VkImageSubresource imSub =
	// initializers::imageSubresourceCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT);
	// vkGetImageSubresourceLayout(device.device, stagingImage.image, &imSub,
	// &sub);
	//
	// char* stagingPointer = (char*)stagingImage.allocationInfo.pMappedData;
	// AlignedTextureMemcpy(6, 0, cubeMap->height, cubeMap->width, cubeMap->width
	// * 4, sub.rowPitch, (char*)cubeMap->pixels.data(), stagingPointer);
	//
	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D, format, (uint32_t)mipLevels, (uint32_t)6,
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_UNDEFINED, imageExtent,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT);
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	device.CreateImage2D(imageCreateInfo, image);

	/*VkImageSubresource stagingImageSubresource = {};
	stagingImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	stagingImageSubresource.mipLevel = 0;
	stagingImageSubresource.arrayLayer = 0;

	VkSubresourceLayout stagingImageLayout = {};
	vkGetImageSubresourceLayout(device.device, stagingImage,
	&stagingImageSubresource, &stagingImageLayout);

	char* const pMipLevelData = (char*)stagingImageAllocInfo.pMappedData +
	stagingImageLayout.offset; uint8_t* pRowData = (uint8_t*)pMipLevelData; for
	(uint32_t y = 0; y < cubeMap->height; ++y)
	{
									uint32_t* pPixelData = (uint32_t*)pRowData;
									for (uint32_t x = 0; x < cubeMap->width; ++x)
									{
																	*pPixelData =
																									((x & 0x18) == 0x08 ?
	0x000000FF : 0x00000000) |
																									((x & 0x18) == 0x10 ?
	0x0000FFFF : 0x00000000) |
																									((y & 0x18) == 0x08 ?
	0x0000FF00 : 0x00000000) |
																									((y & 0x18) == 0x10 ?
	0x00FF0000 : 0x00000000);
																	++pPixelData;
									}
									pRowData += stagingImageLayout.rowPitch;
	}
	*/

	VulkanBufferStagingResource buffer(device, cubeMap->pixels.data(),
		cubeMap->texImageSize);
	// VmaBuffer stagingBuffer;
	// device.CreateStagingImageBuffer(stagingBuffer, cubeMap->pixels.data(),
	// cubeMap->texImageSize);

	VkImageSubresourceRange subresourceRange =
		initializers::imageSubresourceRangeCreateInfo(VK_IMAGE_ASPECT_COLOR_BIT,
			mipLevels, 6);

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t layer = 0; layer < 6; layer++) {
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(cubeMap->width);
		bufferCopyRegion.imageExtent.height =
			static_cast<uint32_t>(cubeMap->height);
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = offset;
		bufferCopyRegions.push_back(bufferCopyRegion);
		// Increase offset into staging buffer for next level / face
		offset += cubeMap->texImageSizePerTex;
	}

	VkImage rawImage = image.image;//not sure why I have to do this. Lambdas are weird

	TransferCommandWork work;
	work.work = std::function<void(const VkCommandBuffer)>(
		[=](const VkCommandBuffer cmdBuf) {
		SetLayoutTransferRegions(cmdBuf, rawImage, buffer.buffer.buffer,
			subresourceRange, bufferCopyRegions);

		//setTransferBarrier(cmdBuf, buffer.buffer.buffer, buffer.Size());

		GenerateMipMaps(cmdBuf, rawImage, imageLayout,
			cubeMap->width, cubeMap->height, 1, 6, mipLevels);
	});
	work.buffersToClean.push_back(buffer);
	work.flags.push_back(readyToUse);

	renderer.SubmitTransferWork(std::move(work));

	// VkCommandBuffer transferCmdBuf = renderer.GetTransferCommandBuffer();

	// SetLayoutTransferRegions(subresourceRange, buffer.buffer.buffer,
	// bufferCopyRegions);

	// renderer.SubmitTransferCommandBufferAndWait(transferCmdBuf);

	// VkCommandBuffer transferCmdBuf = renderer.GetTransferCommandBuffer();

	// SetLayoutCopyCubeMap(transferCmdBuf, image.image, stagingImage.image,
	// mipLevels, imageExtent);

	// renderer.SubmitTransferCommandBufferAndWait(transferCmdBuf);
	this->textureImageLayout = imageLayout;

	// device.DestroyVmaAllocatedBuffer(buffer);
	// device.DestroyVmaAllocatedImage(stagingImage);

	// GenerateMipMaps(device, cubeMap->width, cubeMap->height, 6, mipLevels);

	// CommandBufferWork mipMapGen;
	// mipMapGen.work = [&](VkCommandBuffer cmdBuf) {
	//	GenerateMipMaps(cmdBuf, image.image, cubeMap->width, cubeMap->height, 1,
	// 6, mipLevels);
	//};

	// renderer.SubmitGraphicsSetupWork(std::move(mipMapGen));

	textureSampler = CreateImageSampler(
		VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, true, mipLevels, true, 8,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	textureImageView = CreateImageView(
		image.image, VK_IMAGE_VIEW_TYPE_CUBE, format, VK_IMAGE_ASPECT_COLOR_BIT,
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
						   VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		mipLevels, 6);

	updateDescriptor();

	// Create image view
	// VkImageViewCreateInfo viewCreateInfo = initializers::imageViewCreateInfo();
	// viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	// viewCreateInfo.format = format;
	// viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
	// VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	// viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
	// }; viewCreateInfo.subresourceRange.layerCount = 6;
	// viewCreateInfo.subresourceRange.levelCount = mipLevels;
	// viewCreateInfo.image = image.image;
	// VK_CHECK_RESULT(vkCreateImageView(device.device, &viewCreateInfo, nullptr,
	// &textureImageView));
}

void VulkanTextureDepthBuffer::CreateDepthImage(VulkanRenderer &renderer,
	VkFormat depthFormat, int width,
	int height) {

	VkImageCreateInfo imageInfo = initializers::imageCreateInfo(
		VK_IMAGE_TYPE_2D, depthFormat, 1, 1, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL, VK_SHARING_MODE_EXCLUSIVE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VkExtent3D{ (uint32_t)width, (uint32_t)height, (uint32_t)1 },
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	device.CreateDepthImage(imageInfo, image);

	textureImageView = VulkanTexture::CreateImageView(
		image.image, VK_IMAGE_VIEW_TYPE_2D, depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
						   VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		1, 1);

	Signal wait = std::make_shared<bool>(false);

	CommandBufferWork cmdWork;
	cmdWork.work = std::function<void(VkCommandBuffer cmdBuf)>(
		[=](VkCommandBuffer cmdBuf){
			VkImageSubresourceRange subresourceRange =
				initializers::imageSubresourceRangeCreateInfo(
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

			setImageLayout(cmdBuf, image.image, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				subresourceRange);
		}
	);
	cmdWork.flags.push_back(wait);
	renderer.SubmitGraphicsSetupWork(std::move(cmdWork));

	while(*wait == false) {};

	//VkCommandBuffer copyBuf = renderer.GetSingleUseGraphicsCommandBuffer();


	//renderer.SubmitGraphicsCommandBufferAndWait(copyBuf);
	//Log::Debug << "Depth "<< image.image << "\n";
}

VulkanTextureManager::VulkanTextureManager(VulkanDevice &device)
	: device(device) {}

VulkanTextureManager::~VulkanTextureManager() {
	for (auto& tex : vulkanTextures) {
		tex.destroy();
	}
}

std::shared_ptr<VulkanTexture2D> VulkanTextureManager::CreateTexture2D(
	std::shared_ptr<Texture> texture, VkFormat format, VulkanRenderer &renderer,
	VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout,
	bool forceLinear, bool genMipMaps, int mipMapLevelsToGen, bool wrapBorder) {
	return std::make_shared<VulkanTexture2D>(device);
}

std::shared_ptr<VulkanTexture2DArray>
VulkanTextureManager::CreateTexture2DArray(
	std::shared_ptr<TextureArray> textures, VkFormat format,
	VulkanRenderer &renderer, VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout, bool genMipMaps, int mipMapLevelsToGen) {
	return std::make_shared<VulkanTexture2DArray>(device);
}

std::shared_ptr<VulkanCubeMap> VulkanTextureManager::CreateCubeMap(
	std::shared_ptr<CubeMap> cubeMap, VkFormat format, VulkanRenderer &renderer,
	VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout,
	bool genMipMaps, int mipMapLevelsToGen) {
	return std::make_shared<VulkanCubeMap>(device);
}
std::shared_ptr<VulkanTextureDepthBuffer>
VulkanTextureManager::CreateDepthImage(VulkanRenderer &renderer,
	VkFormat depthFormat, int width,
	int height) {
	return std::make_shared<VulkanTextureDepthBuffer>(device);
}
