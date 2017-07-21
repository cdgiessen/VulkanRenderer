#include "VulkanTexture.hpp"
#include "VulkanTools.h"
#include "VulkanInitializers.hpp"

void VulkanTexture::updateDescriptor()
{
	descriptor.sampler = textureSampler;
	descriptor.imageView = textureImageView;
	descriptor.imageLayout = textureImageLayout;
}

void VulkanTexture::destroy() {
	if(textureImageView != VK_NULL_HANDLE)
		vkDestroyImageView(device->device, textureImageView, nullptr);
	if (textureImage != VK_NULL_HANDLE)
		vkDestroyImage(device->device, textureImage, nullptr);
	if (textureSampler)
		vkDestroySampler(device->device, textureSampler, nullptr);
	
	vkFreeMemory(device->device, textureImageMemory, nullptr);
}

void VulkanTexture::createImageSampler(VkFilter mag, VkFilter min, VkSamplerMipmapMode mipMapMode,
	VkSamplerAddressMode textureWrapMode, float mipMapLodBias, bool useStaging, bool anisotropy , float maxAnisotropy ,
	VkBorderColor borderColor ) {

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
	samplerCreateInfo.maxLod = (useStaging) ? mipLevels : 0.0f;// Max level-of-detail should match mip level count

	// Enable anisotropic filtering
	samplerCreateInfo.maxAnisotropy = maxAnisotropy;
	samplerCreateInfo.anisotropyEnable = anisotropy;
	samplerCreateInfo.borderColor = borderColor;
	VK_CHECK_RESULT(vkCreateSampler(device->device, &samplerCreateInfo, nullptr, &textureSampler));
}


void VulkanTexture2D::loadFromTexture(
	Texture* texture,
	VkFormat format,
	VulkanDevice *device,
	VkQueue copyQueue,
	VkImageUsageFlags imageUsageFlags ,
	VkImageLayout imageLayout,
	bool forceLinear )
{
	this->texture = texture;
	this->device = device;
	mipLevels = texture->mipLevels;

	// Get device properites for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device->physical_device, format, &formatProperties);

	// Only use linear tiling if requested (and supported by the device)
	// Support for linear tiling is mostly limited, so prefer to use
	// optimal tiling instead
	// On most implementations linear tiling will only support a very
	// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
	VkBool32 useStaging = !forceLinear;

	VkMemoryAllocateInfo memAllocInfo = initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	// Use a separate command buffer for texture loading
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	if (useStaging)
	{
		// Create a host-visible staging buffer that contains the raw image data
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo();
		bufferCreateInfo.size = texture->texImageSize;
		// This buffer is used as a transfer source for the buffer copy
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK_RESULT(vkCreateBuffer(device->device, &bufferCreateInfo, nullptr, &stagingBuffer));

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		vkGetBufferMemoryRequirements(device->device, stagingBuffer, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &stagingMemory));
		VK_CHECK_RESULT(vkBindBufferMemory(device->device, stagingBuffer, stagingMemory, 0));

		// Copy texture data into staging buffer
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, texture->pixels, texture->texImageSize);
		vkUnmapMemory(device->device, stagingMemory);

		stbi_image_free(texture->pixels);

		// Setup buffer copy regions for each mip level
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;

		for (uint32_t i = 0; i < texture->mipLevels; i++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = i;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(texture->width / (i + 1));
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(texture->height / (i + 1));
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			offset += static_cast<uint32_t>(texture->texImageSize / ((i + 1)));
		}

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = texture->mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { texture->width, texture->height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		// Ensure that the TRANSFER_DST bit is set for staging
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		VK_CHECK_RESULT(vkCreateImage(device->device, &imageCreateInfo, nullptr, &textureImage));

		vkGetImageMemoryRequirements(device->device, textureImage, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;

		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &textureImageMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->device, textureImage, textureImageMemory, 0));

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = texture->mipLevels;
		subresourceRange.layerCount = 1;

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		setImageLayout(
			copyCmd,
			textureImage,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			textureImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(bufferCopyRegions.size()),
			bufferCopyRegions.data()
		);

		// Change texture image layout to shader read after all mip levels have been copied
		this->textureImageLayout = imageLayout;
		setImageLayout(
			copyCmd,
			textureImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange);

		device->flushCommandBuffer(copyCmd, copyQueue);

		// Clean up staging resources
		vkFreeMemory(device->device, stagingMemory, nullptr);
		vkDestroyBuffer(device->device, stagingBuffer, nullptr);
	}
	else
	{
		// Prefer using optimal tiling, as linear tiling 
		// may support only a small set of features 
		// depending on implementation (e.g. no mip maps, only one layer, etc.)

		// Check if this support is supported for linear tiling
		assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

		VkImage mappableImage;
		VkDeviceMemory mappableMemory;

		VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { texture->width, texture->height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateInfo.usage = imageUsageFlags;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// Load mip map level 0 to linear tiling image
		VK_CHECK_RESULT(vkCreateImage(device->device, &imageCreateInfo, nullptr, &mappableImage));

		// Get memory requirements for this image 
		// like size and alignment
		vkGetImageMemoryRequirements(device->device, mappableImage, &memReqs);
		// Set memory allocation size to required memory size
		memAllocInfo.allocationSize = memReqs.size;

		// Get memory type that can be mapped to host memory
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Allocate host memory
		VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &mappableMemory));

		// Bind allocated image for use
		VK_CHECK_RESULT(vkBindImageMemory(device->device, mappableImage, mappableMemory, 0));

		// Get sub resource layout
		// Mip map count, array layer, etc.
		VkImageSubresource subRes = {};
		subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subRes.mipLevel = 0;

		VkSubresourceLayout subResLayout;
		void *data;

		// Get sub resources layout 
		// Includes row pitch, size offsets, etc.
		vkGetImageSubresourceLayout(device->device, mappableImage, &subRes, &subResLayout);

		// Map image memory
		VK_CHECK_RESULT(vkMapMemory(device->device, mappableMemory, 0, memReqs.size, 0, &data));

		// Copy image data into memory
		memcpy(data, texture->pixels, texture->texImageSize);

		vkUnmapMemory(device->device, mappableMemory);

		stbi_image_free(texture->pixels);

		// Linear tiled images don't need to be staged
		// and can be directly used as textures
		textureImage = mappableImage;
		textureImageMemory = mappableMemory;
		imageLayout = imageLayout;

		// Setup image memory barrier
		setImageLayout(copyCmd, textureImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

		device->flushCommandBuffer(copyCmd, copyQueue);
	}

	// Create a defaultsampler
	createImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.0f, true, true, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	// Create image view
	// Textures are not directly accessed by the shaders and
	// are abstracted by image views containing additional
	// information and sub resource ranges
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	// Linear tiling usually won't support mip maps
	// Only set mip map count if optimal tiling is used
	viewCreateInfo.subresourceRange.levelCount = (useStaging) ? texture->mipLevels : 1;
	viewCreateInfo.image = textureImage;
	VK_CHECK_RESULT(vkCreateImageView(device->device, &viewCreateInfo, nullptr, &textureImageView));

	// Update descriptor image info member that can be used for setting up descriptor sets
	updateDescriptor();
}

void VulkanTexture2DArray::loadTextureArray(
	std::vector<Texture> textures,
	VkFormat format,
	VulkanDevice *device,
	VkQueue copyQueue,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout)
{

	this->device = device;
	this->textures = textures;

	VkMemoryAllocateInfo memAllocInfo = initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo();
	bufferCreateInfo.size = textures.size() * textures[0].texImageSize;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK_RESULT(vkCreateBuffer(device->device, &bufferCreateInfo, nullptr, &stagingBuffer));

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	vkGetBufferMemoryRequirements(device->device, stagingBuffer, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &stagingMemory));
	VK_CHECK_RESULT(vkBindBufferMemory(device->device, stagingBuffer, stagingMemory, 0));

	// Copy texture data into staging buffer
	uint8_t *data;
	VK_CHECK_RESULT(vkMapMemory(device->device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
	memcpy(data, textures.data(), static_cast<size_t>(textures.size() * textures[0].texImageSize));
	vkUnmapMemory(device->device, stagingMemory);

	// Setup buffer copy regions for each layer including all of it's miplevels
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t layer = 0; layer < textures.size(); layer++)
	{
		for (uint32_t level = 0; level < mipLevels; level++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(textures[0].width);
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(textures[0].height);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			// Increase offset into staging buffer for next level / face
			offset += textures[0].texImageSize;
		}
	}

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent = { textures[0].width, textures[0].height, 1 };
	imageCreateInfo.usage = imageUsageFlags;
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
	{
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	imageCreateInfo.arrayLayers = textures.size();
	imageCreateInfo.mipLevels = mipLevels;

	VK_CHECK_RESULT(vkCreateImage(device->device, &imageCreateInfo, nullptr, &textureImage));

	vkGetImageMemoryRequirements(device->device, textureImage, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &textureImageMemory));
	VK_CHECK_RESULT(vkBindImageMemory(device->device, textureImage, textureImageMemory, 0));

	// Use a separate command buffer for texture loading
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Image barrier for optimal image (target)
	// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = textures.size();

	setImageLayout(
		copyCmd,
		textureImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	// Copy the layers and mip levels from the staging buffer to the optimal tiled image
	vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer,
		textureImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());

	// Change texture image layout to shader read after all faces have been copied
	this->textureImageLayout = imageLayout;
	setImageLayout(
		copyCmd,
		textureImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		imageLayout,
		subresourceRange);

	device->flushCommandBuffer(copyCmd, copyQueue);

	// Create sampler
	createImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, true, true, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	// Create image view
	VkImageViewCreateInfo viewCreateInfo = initializers::imageViewCreateInfo();
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	viewCreateInfo.format = format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.subresourceRange.layerCount = textures.size();
	viewCreateInfo.subresourceRange.levelCount = mipLevels;
	viewCreateInfo.image = textureImage;
	VK_CHECK_RESULT(vkCreateImageView(device->device, &viewCreateInfo, nullptr, &textureImageView));

	// Clean up staging resources
	vkFreeMemory(device->device, stagingMemory, nullptr);
	vkDestroyBuffer(device->device, stagingBuffer, nullptr);

	// Update descriptor image info member that can be used for setting up descriptor sets
	updateDescriptor();
}
	

void VulkanCubeMap::loadFromTexture(
	CubeMap *cubeMap,
	VkFormat format,
	VulkanDevice *device,
	VkQueue copyQueue,
	VkImageUsageFlags imageUsageFlags,
	VkImageLayout imageLayout)
{
	this->cubeMap = cubeMap;
	this->device = device;
	mipLevels = cubeMap->mipLevels;

	VkMemoryAllocateInfo memAllocInfo = initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	// Create a host-visible staging buffer that contains the raw image data
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo();
	bufferCreateInfo.size = cubeMap->texImageSize;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK_RESULT(vkCreateBuffer(device->device, &bufferCreateInfo, nullptr, &stagingBuffer));

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	vkGetBufferMemoryRequirements(device->device, stagingBuffer, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &stagingMemory));
	VK_CHECK_RESULT(vkBindBufferMemory(device->device, stagingBuffer, stagingMemory, 0));

	// Copy texture data into staging buffer
	uint8_t *data;
	VK_CHECK_RESULT(vkMapMemory(device->device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
	memcpy(data, cubeMap->pixels, cubeMap->texImageSize);
	vkUnmapMemory(device->device, stagingMemory);

	// Setup buffer copy regions for each face including all of it's miplevels
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	size_t offset = 0;

	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0; level < 1; level++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(cubeMap->width);
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(cubeMap->height);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back(bufferCopyRegion);

			// Increase offset into staging buffer for next level / face
			offset += cubeMap->cubeImages.Front.texImageSize;
		}
	}

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = cubeMap->mipLevels;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent = { cubeMap->width, cubeMap->height, 1 };
	imageCreateInfo.usage = imageUsageFlags;
	// Ensure that the TRANSFER_DST bit is set for staging
	if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
	{
		imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	// Cube faces count as array layers in Vulkan
	imageCreateInfo.arrayLayers = 6;
	// This flag is required for cube map images
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


	VK_CHECK_RESULT(vkCreateImage(device->device, &imageCreateInfo, nullptr, &textureImage));

	vkGetImageMemoryRequirements(device->device, textureImage, &memReqs);

	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(device->device, &memAllocInfo, nullptr, &textureImageMemory));
	VK_CHECK_RESULT(vkBindImageMemory(device->device, textureImage, textureImageMemory, 0));

	// Use a separate command buffer for texture loading
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// Image barrier for optimal image (target)
	// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.layerCount = 6;

	setImageLayout(
		copyCmd,
		textureImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	// Copy the cube map faces from the staging buffer to the optimal tiled image
	vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer,
		textureImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());

	// Change texture image layout to shader read after all faces have been copied
	this->textureImageLayout = imageLayout;
	setImageLayout(
		copyCmd,
		textureImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		imageLayout,
		subresourceRange);

	device->flushCommandBuffer(copyCmd, copyQueue);

	// Create a defaultsampler
	createImageSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, true, true, 8, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	// Create image view
	VkImageViewCreateInfo viewCreateInfo = initializers::imageViewCreateInfo();
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewCreateInfo.format = format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.subresourceRange.layerCount = 6;
	viewCreateInfo.subresourceRange.levelCount = mipLevels;
	viewCreateInfo.image = textureImage;
	VK_CHECK_RESULT(vkCreateImageView(device->device, &viewCreateInfo, nullptr, &textureImageView));

	// Clean up staging resources
	vkFreeMemory(device->device, stagingMemory, nullptr);
	vkDestroyBuffer(device->device, stagingBuffer, nullptr);

	// Update descriptor image info member that can be used for setting up descriptor sets
	updateDescriptor();
}
	
