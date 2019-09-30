#include "Texture.h"

#include <algorithm>

#include "stb/stb_image.h"

#include "Device.h"
#include "Initializers.h"
#include "RenderTools.h"

#include "core/Logger.h"

void SetImageLayout (VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = initializers::imageMemoryBarrier ();
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldImageLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newImageLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask =
			    imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier (cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

// Fixed sub resource on first mip level and layer
void SetImageLayout (VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	SetImageLayout (cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}

void GenerateMipMaps (
    VkCommandBuffer cmdBuf, VkImage image, VkImageLayout finalImageLayout, int width, int height, int depth, int layers, int mipLevels)
{
	// We copy down the whole mip chain doing a blit from mip-1 to mip
	// An alternative way would be to always blit from the first mip level and
	// sample that one down

	// Copy down mips from n-1 to n
	for (int32_t i = 1; i < mipLevels; i++)
	{

		VkImageBlit imageBlit{};

		// Source
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.layerCount = layers;
		imageBlit.srcSubresource.baseArrayLayer = 0;
		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[1].x = int32_t (width >> (i - 1));
		imageBlit.srcOffsets[1].y = int32_t (height >> (i - 1));
		imageBlit.srcOffsets[1].z = 1;

		// Destination
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.layerCount = layers;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.mipLevel = i;
		imageBlit.dstOffsets[1].x = int32_t (width >> i);
		imageBlit.dstOffsets[1].y = int32_t (height >> i);
		imageBlit.dstOffsets[1].z = 1;

		VkImageSubresourceRange mipSubRange =
		    initializers::imageSubresourceRangeCreateInfo (VK_IMAGE_ASPECT_COLOR_BIT, 1, layers);
		mipSubRange.baseMipLevel = i;

		// Transiston current mip level to transfer dest
		SetImageLayout (cmdBuf,
		    image,
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    mipSubRange,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Blit from previous level
		vkCmdBlitImage (
		    cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

		// Transiston current mip level to transfer source for read in next iteration
		SetImageLayout (cmdBuf,
		    image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    mipSubRange,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT);
	}

	VkImageSubresourceRange subresourceRange =
	    initializers::imageSubresourceRangeCreateInfo (VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, layers);

	// After the loop, all mip layers are in TRANSFER_SRC layout, so transition
	// all to SHADER_READ

	if (finalImageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		Log.Error (fmt::format ("Final image layout Undefined!\n"));
	SetImageLayout (cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, finalImageLayout, subresourceRange);
}

void SetLayoutAndTransferRegions (VkCommandBuffer transferCmdBuf,
    VkImage image,
    VkBuffer stagingBuffer,
    const VkImageSubresourceRange subresourceRange,
    std::vector<VkBufferImageCopy> bufferCopyRegions)
{

	SetImageLayout (transferCmdBuf, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	vkCmdCopyBufferToImage (transferCmdBuf,
	    stagingBuffer,
	    image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    bufferCopyRegions.size (),
	    bufferCopyRegions.data ());

	SetImageLayout (
	    transferCmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
}

void BeginTransferAndMipMapGenWork (VulkanDevice& device,
    AsyncTaskManager& async_task_man,
    BufferManager& buf_man,
    BufferID buffer,
    const VkImageSubresourceRange subresourceRange,
    const std::vector<VkBufferImageCopy> bufferCopyRegions,
    VkImageLayout imageLayout,
    VkImage image,
    int width,
    int height,
    int depth,
    int layers,
    int mipLevels)
{
	if (device.singleQueueDevice)
	{
		std::function<void(const VkCommandBuffer)> work = [&](const VkCommandBuffer cmdBuf) {
			SetLayoutAndTransferRegions (
			    cmdBuf, image, buf_man.GetBuffer (buffer).buffer, subresourceRange, bufferCopyRegions);

			GenerateMipMaps (cmdBuf, image, imageLayout, width, height, depth, layers, mipLevels);
		};

		AsyncTask task;
		task.work = work;
		task.finish_work = [&] { buf_man.FreeBuffer (buffer); };

		async_task_man.SubmitTask (TaskType::graphics, std::move (task));
	}
	else
	{
		auto sem = std::make_shared<VulkanSemaphore> (device);

		std::function<void(const VkCommandBuffer)> transferWork = [=, &buf_man](const VkCommandBuffer cmdBuf) {
			SetLayoutAndTransferRegions (
			    cmdBuf, image, buf_man.GetBuffer (buffer).buffer, subresourceRange, bufferCopyRegions);
		};

		std::function<void(const VkCommandBuffer)> mipMapGenWork = [=](const VkCommandBuffer cmdBuf) {
			GenerateMipMaps (cmdBuf, image, imageLayout, width, height, depth, layers, mipLevels);
		};

		AsyncTask task_transfer;
		task_transfer.work = transferWork;
		task_transfer.finish_work = [=, &buf_man] { buf_man.FreeBuffer (buffer); };
		task_transfer.signal_sems = { sem };

		AsyncTask task_gen_mips;
		task_gen_mips.work = mipMapGenWork;
		task_gen_mips.wait_sems = { sem };

		async_task_man.SubmitTask (TaskType::transfer, std::move (task_transfer));
		async_task_man.SubmitTask (TaskType::graphics, std::move (task_gen_mips));
	}
}

VulkanTexture::VulkanTexture (VulkanDevice& device,
    AsyncTaskManager& async_task_man,
    BufferManager& buf_man,
    TexCreateDetails texCreateDetails,
    Resource::Texture::TexResource textureResource)
: device (device), resource (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
{
	this->mipLevels = texCreateDetails.genMipMaps ? texCreateDetails.mipMapLevelsToGen : 1;
	this->textureImageLayout = texCreateDetails.imageLayout;
	this->layers = textureResource.description.layers;

	VkExtent3D imageExtent = { textureResource.description.width,
		textureResource.description.height,
		textureResource.description.depth };

	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo (VK_IMAGE_TYPE_2D,
	    texCreateDetails.format,
	    (uint32_t)mipLevels,
	    (uint32_t)layers,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_SHARING_MODE_EXCLUSIVE,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    imageExtent,
	    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	if (textureResource.layout == Resource::Texture::LayoutType::cubemap2D)
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


	auto buffer = buf_man.CreateBuffer (
	    staging_details (BufferType::staging, textureResource.description.pixelCount * 4));

	buf_man.GetBuffer (buffer).CopyToBuffer (textureResource.data);

	InitImage2D (imageCreateInfo);

	VkImageSubresourceRange subresourceRange =
	    initializers::imageSubresourceRangeCreateInfo (VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, layers);

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t layer = 0; layer < layers; layer++)
	{
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = static_cast<uint32_t> (textureResource.description.width);
		bufferCopyRegion.imageExtent.height = static_cast<uint32_t> (textureResource.description.height);
		bufferCopyRegion.imageExtent.depth = static_cast<uint32_t> (textureResource.description.depth);
		bufferCopyRegion.bufferOffset = offset;
		bufferCopyRegions.push_back (bufferCopyRegion);
		// Increase offset into staging buffer for next level / face
		offset += textureResource.description.width * textureResource.description.height *
		          textureResource.description.depth * 4;
	}

	BeginTransferAndMipMapGenWork (device,
	    async_task_man,
	    buf_man,
	    buffer,
	    subresourceRange,
	    bufferCopyRegions,
	    texCreateDetails.imageLayout,
	    image.image,
	    textureResource.description.width,
	    textureResource.description.height,
	    textureResource.description.depth,
	    layers,
	    mipLevels);

	textureSampler = CreateImageSampler (VK_FILTER_LINEAR,
	    VK_FILTER_LINEAR,
	    VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    texCreateDetails.addressMode,
	    0.0f,
	    true,
	    mipLevels,
	    true,
	    8,
	    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageViewType viewType;
	if (textureResource.layout == Resource::Texture::LayoutType::array1D ||
	    textureResource.layout == Resource::Texture::LayoutType::array2D ||
	    textureResource.layout == Resource::Texture::LayoutType::array3D)
	{
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	else if (textureResource.layout == Resource::Texture::LayoutType::cubemap2D)
	{
		viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else
	{
		viewType = VK_IMAGE_VIEW_TYPE_2D;
	}
	textureImageView = CreateImageView (image.image,
	    viewType,
	    texCreateDetails.format,
	    VK_IMAGE_ASPECT_COLOR_BIT,
	    VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
	    mipLevels,
	    layers);

	resource.FillResource (textureSampler, textureImageView, textureImageLayout);
}

VulkanTexture::VulkanTexture (VulkanDevice& device,
    AsyncTaskManager& async_task_man,
    BufferManager& buf_man,
    TexCreateDetails texCreateDetails,
    BufferID buffer)
: device (device), resource (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
{
	this->mipLevels = texCreateDetails.genMipMaps ? texCreateDetails.mipMapLevelsToGen : 1;
	this->textureImageLayout = texCreateDetails.imageLayout;
	this->layers = 1;

	VkExtent3D imageExtent = { (uint32_t)texCreateDetails.desiredWidth, (uint32_t)texCreateDetails.desiredHeight, 1 };

	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo (VK_IMAGE_TYPE_2D,
	    texCreateDetails.format,
	    (uint32_t)mipLevels,
	    (uint32_t)layers,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_SHARING_MODE_EXCLUSIVE,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    imageExtent,
	    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	InitImage2D (imageCreateInfo);

	VkImageSubresourceRange subresourceRange =
	    initializers::imageSubresourceRangeCreateInfo (VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, layers);

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (int32_t layer = 0; layer < layers; layer++)
	{
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = (uint32_t)layer;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = static_cast<uint32_t> (texCreateDetails.desiredWidth);
		bufferCopyRegion.imageExtent.height = static_cast<uint32_t> (texCreateDetails.desiredHeight);
		bufferCopyRegion.imageExtent.depth = static_cast<uint32_t> (1);
		bufferCopyRegion.bufferOffset = offset;
		bufferCopyRegions.push_back (bufferCopyRegion);
		// Increase offset into staging buffer for next level / face
		offset += texCreateDetails.desiredWidth * texCreateDetails.desiredHeight * 4;
	}

	BeginTransferAndMipMapGenWork (device,
	    async_task_man,
	    buf_man,
	    buffer,
	    subresourceRange,
	    bufferCopyRegions,
	    texCreateDetails.imageLayout,
	    image.image,
	    texCreateDetails.desiredWidth,
	    texCreateDetails.desiredHeight,
	    1,
	    layers,
	    mipLevels);

	textureSampler = CreateImageSampler (VK_FILTER_LINEAR,
	    VK_FILTER_LINEAR,
	    VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    texCreateDetails.addressMode,
	    0.0f,
	    true,
	    mipLevels,
	    true,
	    8,
	    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	VkImageViewType viewType;
	// if(texCreateDetails.layout == Resource::Texture::LayoutType::array1D
	//	|| texCreateDetails.layout == Resource::Texture::LayoutType::array1D
	//	|| texCreateDetails.layout == Resource::Texture::LayoutType::array1D){
	//	viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	//}
	// else if(texCreateDetails.layout == Resource::Texture::LayoutType::cubemap2D){
	//	viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	//}
	// else {
	viewType = VK_IMAGE_VIEW_TYPE_2D;
	//}


	textureImageView = CreateImageView (image.image,
	    viewType,
	    texCreateDetails.format,
	    VK_IMAGE_ASPECT_COLOR_BIT,
	    VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
	    mipLevels,
	    layers);

	resource.FillResource (textureSampler, textureImageView, textureImageLayout);
}

VulkanTexture::VulkanTexture (VulkanDevice& device, TexCreateDetails texCreateDetails)
: device (device), resource (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
{
	VkImageCreateInfo imageInfo = initializers::imageCreateInfo (VK_IMAGE_TYPE_2D,
	    texCreateDetails.format,
	    1,
	    1,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_SHARING_MODE_EXCLUSIVE,
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VkExtent3D{ (uint32_t)texCreateDetails.desiredWidth, (uint32_t)texCreateDetails.desiredHeight, (uint32_t)1 },
	    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	image.allocator = device.GetImageOptimalAllocator ();
	VK_CHECK_RESULT (vmaCreateImage (
	    image.allocator, &imageInfo, &imageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));


	textureImageView = VulkanTexture::CreateImageView (image.image,
	    VK_IMAGE_VIEW_TYPE_2D,
	    texCreateDetails.format,
	    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
	    VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
	    1,
	    1);


	VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRangeCreateInfo (
	    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

	CommandPool pool (device, device.GraphicsQueue ());

	VkCommandBuffer cmdBuf = pool.GetCommandBuffer ();

	SetImageLayout (
	    cmdBuf, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, subresourceRange);

	VulkanFence fence = VulkanFence (device);

	pool.ReturnCommandBuffer (cmdBuf, fence);
	fence.Wait ();
}


VulkanTexture::~VulkanTexture ()
{
	vmaDestroyImage (image.allocator, image.image, image.allocation);

	if (textureImageView != VK_NULL_HANDLE)
		vkDestroyImageView (device.device, textureImageView, nullptr);
	if (textureSampler != VK_NULL_HANDLE) vkDestroySampler (device.device, textureSampler, nullptr);
}



VkSampler VulkanTexture::CreateImageSampler (VkFilter mag,
    VkFilter min,
    VkSamplerMipmapMode mipMapMode,
    VkSamplerAddressMode textureWrapMode,
    float mipMapLodBias,
    bool useMipMaps,
    int mipLevels,
    bool anisotropy,
    float maxAnisotropy,
    VkBorderColor borderColor)
{

	// Create a defaultsampler
	VkSamplerCreateInfo samplerCreateInfo = initializers::samplerCreateInfo ();
	samplerCreateInfo.magFilter = mag;
	samplerCreateInfo.minFilter = min;
	samplerCreateInfo.mipmapMode = mipMapMode;
	samplerCreateInfo.addressModeU = textureWrapMode;
	samplerCreateInfo.addressModeV = textureWrapMode;
	samplerCreateInfo.addressModeW = textureWrapMode;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = (useMipMaps) ? mipLevels : 0.0f; // Max level-of-detail should match mip level count
	samplerCreateInfo.anisotropyEnable = anisotropy; // Enable anisotropic filtering
	samplerCreateInfo.maxAnisotropy =
	    (anisotropy) ? device.physical_device.physical_device_properties.limits.maxSamplerAnisotropy : 1;
	samplerCreateInfo.borderColor = borderColor;

	VkSampler sampler;
	VK_CHECK_RESULT (vkCreateSampler (device.device, &samplerCreateInfo, nullptr, &sampler));

	return sampler;
}

VkImageView VulkanTexture::CreateImageView (VkImage image,
    VkImageViewType viewType,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkComponentMapping components,
    int mipLevels,
    int layers)
{
	VkImageViewCreateInfo viewInfo = initializers::imageViewCreateInfo ();
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
	VK_CHECK_RESULT (vkCreateImageView (device.device, &viewInfo, nullptr, &imageView));

	return imageView;
}

void VulkanTexture::InitImage2D (VkImageCreateInfo imageInfo)
{

	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	if (imageInfo.tiling == VK_IMAGE_TILING_OPTIMAL)
	{
		image.allocator = device.GetImageOptimalAllocator ();
	}
	else if (imageInfo.tiling == VK_IMAGE_TILING_LINEAR)
	{
		image.allocator = device.GetImageLinearAllocator ();
	}
	VK_CHECK_RESULT (vmaCreateImage (
	    image.allocator, &imageInfo, &imageAllocCreateInfo, &image.image, &image.allocation, &image.allocationInfo));
}

VulkanTextureManager::VulkanTextureManager (Resource::Texture::Manager& texture_manager,
    VulkanDevice& device,
    AsyncTaskManager& async_task_manager,
    BufferManager& buffer_manager)
: device (device),
  texture_manager (texture_manager),
  async_task_manager (async_task_manager),
  buffer_manager (buffer_manager)
{
}

VulkanTextureManager::~VulkanTextureManager ()
{
	Log.Debug (fmt::format ("Textures left over {}\n", texture_map.size ()));
}

VulkanTextureID VulkanTextureManager::CreateTexture2D (Resource::Texture::TexID texture, TexCreateDetails texCreateDetails)
{
	auto& resource = texture_manager.GetTexResourceByID (texture);
	auto tex = std::make_unique<VulkanTexture> (
	    device, async_task_manager, buffer_manager, texCreateDetails, resource);
	std::lock_guard guard (map_lock);
	texture_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTextureID VulkanTextureManager::CreateTexture2DArray (
    Resource::Texture::TexID textures, TexCreateDetails texCreateDetails)
{
	auto& resource = texture_manager.GetTexResourceByID (textures);
	auto tex = std::make_unique<VulkanTexture> (
	    device, async_task_manager, buffer_manager, texCreateDetails, resource);
	std::lock_guard guard (map_lock);
	texture_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTextureID VulkanTextureManager::CreateCubeMap (Resource::Texture::TexID cubeMap, TexCreateDetails texCreateDetails)
{
	auto& resource = texture_manager.GetTexResourceByID (cubeMap);
	auto tex = std::make_unique<VulkanTexture> (
	    device, async_task_manager, buffer_manager, texCreateDetails, resource);
	std::lock_guard guard (map_lock);
	texture_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTextureID VulkanTextureManager::CreateTextureFromBuffer (BufferID buffer, TexCreateDetails texCreateDetails)
{
	auto tex = std::make_unique<VulkanTexture> (
	    device, async_task_manager, buffer_manager, texCreateDetails, buffer);
	std::lock_guard guard (map_lock);
	texture_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTextureID VulkanTextureManager::CreateDepthImage (VkFormat depthFormat, int width, int height)
{
	TexCreateDetails texCreateDetails;
	texCreateDetails.format = depthFormat;
	texCreateDetails.desiredWidth = width;
	texCreateDetails.desiredHeight = height;
	auto tex = std::make_unique<VulkanTexture> (device, texCreateDetails);
	std::lock_guard guard (map_lock);
	texture_map[id_counter] = std::move (tex);
	return id_counter++;
}

VulkanTexture const& VulkanTextureManager::get_texture (VulkanTextureID id)
{
	std::lock_guard guard (map_lock);
	return *texture_map.at (id);
}


void VulkanTextureManager::delete_texture (VulkanTextureID id)
{
	std::lock_guard guard (map_lock);
	texture_map.erase (id);
}