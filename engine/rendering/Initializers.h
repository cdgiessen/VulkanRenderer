#pragma once

#include <array>
#include <vector>
#include <vulkan/vulkan.h>


namespace initializers
{

inline VkMemoryAllocateInfo memory_allocate_info ()
{
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	return memAllocInfo;
}

inline VkMappedMemoryRange mapped_memory_range ()
{
	VkMappedMemoryRange mappedMemoryRange{};
	mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	return mappedMemoryRange;
}

inline VkCommandBufferAllocateInfo command_buffer_allocate_info (
    VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = bufferCount;
	return commandBufferAllocateInfo;
}

inline VkCommandPoolCreateInfo command_pool_create_info ()
{
	VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	return cmdPoolCreateInfo;
}

inline VkCommandBufferBeginInfo command_buffer_begin_info ()
{
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	return cmdBufferBeginInfo;
}

inline VkCommandBufferInheritanceInfo command_buffer_inheritance_info ()
{
	VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
	cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	return cmdBufferInheritanceInfo;
}

inline VkRenderPassBeginInfo render_pass_begin_info ()
{
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	return renderPassBeginInfo;
}

inline VkRenderPassBeginInfo render_pass_begin_info (VkRenderPass renderPass,
    VkFramebuffer frameBuffer,
    VkOffset2D offset,
    VkExtent2D extent,
    std::vector<VkClearValue> const& clearValues)
{
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = frameBuffer;
	renderPassBeginInfo.renderArea.offset = offset;
	renderPassBeginInfo.renderArea.extent = extent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t> (clearValues.size ());
	renderPassBeginInfo.pClearValues = clearValues.data ();
	return renderPassBeginInfo;
}

inline VkRenderPassCreateInfo render_pass_create_info ()
{
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	return renderPassCreateInfo;
}

/** @brief Initialize an image memory barrier with no image transfer ownership */
inline VkImageMemoryBarrier image_memory_barrier ()
{
	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return imageMemoryBarrier;
}

/** @brief Initialize a buffer memory barrier with no image transfer ownership */
inline VkBufferMemoryBarrier buffer_memory_barrier ()
{
	VkBufferMemoryBarrier bufferMemoryBarrier{};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return bufferMemoryBarrier;
}

inline VkMemoryBarrier memory_barrier ()
{
	VkMemoryBarrier memoryBarrier{};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	return memoryBarrier;
}

inline VkImageCreateInfo image_create_info ()
{
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	return imageCreateInfo;
}

inline VkImageCreateInfo image_create_info (VkImageType imageType,
    VkFormat format,
    uint32_t mipLevels,
    uint32_t layers,
    VkSampleCountFlagBits sampleCounts,
    VkImageTiling tiling,
    VkSharingMode sharingMode,
    VkImageLayout initialLayout,
    VkExtent3D extent,
    VkImageUsageFlags usageFlags)
{
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = imageType;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = layers;
	imageCreateInfo.samples = sampleCounts;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.sharingMode = sharingMode;
	imageCreateInfo.initialLayout = initialLayout;
	imageCreateInfo.extent = extent;
	imageCreateInfo.usage = usageFlags;
	imageCreateInfo.flags = 0;

	return imageCreateInfo;
}

inline VkImageSubresourceRange image_subresource_range_create_info (
    VkImageAspectFlags aspectMask, int mipLevels = 1, int layerCount = 1)
{
	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipLevels;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = layerCount;

	return subresourceRange;
}

inline VkImageSubresource image_subresource_create_info (
    VkImageAspectFlags aspectMask, int mipLevel = 0, int arrayLayer = 0)
{
	VkImageSubresource subresource{};
	subresource.aspectMask = aspectMask;
	subresource.mipLevel = mipLevel;
	subresource.arrayLayer = arrayLayer;

	return subresource;
}

inline VkImageSubresourceLayers image_subresource_layers (
    VkImageAspectFlags aspectMask, uint32_t mipLevel = 0, uint32_t layerCount = 0, uint32_t baseArrayLayer = 0)
{
	VkImageSubresourceLayers sub{};
	sub.aspectMask = aspectMask;
	sub.mipLevel = mipLevel;
	sub.layerCount = layerCount;
	sub.baseArrayLayer = baseArrayLayer;
	return sub;
}

inline VkImageBlit image_blit (VkImageSubresourceLayers srcSubresource,
    VkOffset3D srcOffsets,
    VkImageSubresourceLayers dstSubresource,
    VkOffset3D dstOffsets)
{
	VkImageBlit blit{};
	blit.srcSubresource = srcSubresource;
	blit.srcOffsets[1] = srcOffsets;
	blit.dstSubresource = dstSubresource;
	blit.dstOffsets[1] = dstOffsets;
	return blit;
}

inline VkSamplerCreateInfo sampler_create_info ()
{
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	return samplerCreateInfo;
}

inline VkImageViewCreateInfo image_view_create_info ()
{
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	return imageViewCreateInfo;
}

inline VkFramebufferCreateInfo framebuffer_create_info ()
{
	VkFramebufferCreateInfo framebufferCreateInfo{};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	return framebufferCreateInfo;
}

inline VkSemaphoreCreateInfo semaphore_create_info ()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	return semaphoreCreateInfo;
}

inline VkFenceCreateInfo fence_create_info (VkFenceCreateFlags flags = 0)
{
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = flags;
	return fenceCreateInfo;
}

inline VkEventCreateInfo event_create_info ()
{
	VkEventCreateInfo eventCreateInfo{};
	eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	return eventCreateInfo;
}

inline VkSubmitInfo submit_info ()
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	return submitInfo;
}

inline VkViewport viewport (float width, float height, float minDepth, float maxDepth)
{
	VkViewport viewport{};
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

inline VkRect2D rect2D (int32_t width, int32_t height, int32_t offsetX, int32_t offsetY)
{
	VkRect2D rect2D{};
	rect2D.extent.width = width;
	rect2D.extent.height = height;
	rect2D.offset.x = offsetX;
	rect2D.offset.y = offsetY;
	return rect2D;
}

inline VkBufferImageCopy buffer_image_copy_create (VkImageSubresourceLayers imageSubresource,
    VkExtent3D imageExtent,
    VkDeviceSize bufferOffset,
    VkOffset3D imageOffset = { 0, 0, 0 },
    uint32_t bufferRowLength = 0,
    uint32_t bufferImageHeight = 0)
{
	VkBufferImageCopy region{};
	region.imageSubresource = imageSubresource;
	region.imageExtent = imageExtent;
	region.imageOffset = imageOffset;
	region.bufferRowLength = bufferRowLength;
	region.bufferImageHeight = bufferImageHeight;
	region.bufferOffset = bufferOffset;
	return region;
}

inline VkBufferCopy buffer_copy_create (VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
{
	VkBufferCopy bufferRegion{};
	bufferRegion.size = size;
	bufferRegion.srcOffset = srcOffset;
	bufferRegion.dstOffset = dstOffset;
	return bufferRegion;
}

inline VkBufferCreateInfo buffer_create_info ()
{
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	return bufCreateInfo;
}

inline VkBufferCreateInfo buffer_create_info (VkBufferUsageFlags usage, VkDeviceSize size)
{
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.usage = usage;
	bufCreateInfo.size = size;
	return bufCreateInfo;
}

inline VkDescriptorBufferInfo descriptor_buffer_create_info (VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	VkDescriptorBufferInfo info{};
	info.buffer = buffer;
	info.offset = offset;
	info.range = range;
	return info;
}

inline VkDescriptorPoolCreateInfo descriptor_pool_create_info (
    std::vector<VkDescriptorPoolSize> const& poolSizes, uint32_t maxSets)
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t> (poolSizes.size ());
	descriptorPoolInfo.pPoolSizes = poolSizes.data ();
	descriptorPoolInfo.maxSets = maxSets;
	return descriptorPoolInfo;
}

inline VkDescriptorPoolSize descriptor_pool_size (VkDescriptorType type, uint32_t descriptorCount)
{
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = type;
	descriptorPoolSize.descriptorCount = descriptorCount;
	return descriptorPoolSize;
}

inline VkDescriptorSetLayoutBinding descriptor_set_layout_binding (
    VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding, uint32_t descriptorCount = 1)
{
	VkDescriptorSetLayoutBinding setLayoutBinding{};
	setLayoutBinding.descriptorType = type;
	setLayoutBinding.stageFlags = stageFlags;
	setLayoutBinding.binding = binding;
	setLayoutBinding.descriptorCount = descriptorCount;
	return setLayoutBinding;
}

inline VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info (
    std::vector<VkDescriptorSetLayoutBinding> const& bindings)
{
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pBindings = bindings.data ();
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t> (bindings.size ());
	return descriptorSetLayoutCreateInfo;
}

inline VkPipelineLayoutCreateInfo pipeline_set_layout_create_info (
    std::vector<VkDescriptorSetLayout> const& layouts, std::vector<VkPushConstantRange> const& ranges)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t> (layouts.size ());
	pipelineLayoutCreateInfo.pSetLayouts = layouts.data ();
	pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t> (ranges.size ());
	pipelineLayoutCreateInfo.pPushConstantRanges = ranges.data ();
	return pipelineLayoutCreateInfo;
}

inline VkDescriptorSetAllocateInfo descriptor_set_allocate_info (
    VkDescriptorPool descriptorPool, std::vector<VkDescriptorSetLayout> const& setLayouts)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = setLayouts.data ();
	descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t> (setLayouts.size ());
	return descriptorSetAllocateInfo;
}

inline VkWriteDescriptorSet write_descriptor_set (VkDescriptorSet dstSet,
    VkDescriptorType type,
    uint32_t binding,
    VkDescriptorBufferInfo* bufferInfo,
    uint32_t descriptorCount = 1)
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pBufferInfo = bufferInfo;
	writeDescriptorSet.descriptorCount = descriptorCount;
	return writeDescriptorSet;
}

inline VkWriteDescriptorSet write_descriptor_set (VkDescriptorSet dstSet,
    VkDescriptorType type,
    uint32_t binding,
    VkDescriptorImageInfo* imageInfo,
    uint32_t descriptorCount = 1)
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pImageInfo = imageInfo;
	writeDescriptorSet.descriptorCount = descriptorCount;
	return writeDescriptorSet;
}

inline VkVertexInputBindingDescription vertex_input_binding_description (
    uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
{
	VkVertexInputBindingDescription vInputBindDescription{};
	vInputBindDescription.binding = binding;
	vInputBindDescription.stride = stride;
	vInputBindDescription.inputRate = inputRate;
	return vInputBindDescription;
}

inline VkVertexInputAttributeDescription vertex_input_attribute_description (
    uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
{
	VkVertexInputAttributeDescription vInputAttribDescription{};
	vInputAttribDescription.location = location;
	vInputAttribDescription.binding = binding;
	vInputAttribDescription.format = format;
	vInputAttribDescription.offset = offset;
	return vInputAttribDescription;
}

inline VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info (
    VkShaderStageFlagBits shaderStage, VkShaderModule module)
{
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = shaderStage;
	shaderStageCreateInfo.module = module;
	shaderStageCreateInfo.pName = "main";
	return shaderStageCreateInfo;
}

inline VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info ()
{
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	return pipelineVertexInputStateCreateInfo;
}

inline VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info (
    std::vector<VkVertexInputBindingDescription> const& bindings,
    std::vector<VkVertexInputAttributeDescription> const& attribs)
{
	// int index = 0;
	// for (auto& attrib : attribs)
	// {
	// 	attrib.location = index++;
	// }

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount =
	    static_cast<uint32_t> (bindings.size ());
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = bindings.data ();
	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount =
	    static_cast<uint32_t> (attribs.size ());
	pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = attribs.data ();

	return pipelineVertexInputStateCreateInfo;
}

inline VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info (
    VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable, VkPipelineInputAssemblyStateCreateFlags flags = 0)
{
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.topology = topology;
	pipelineInputAssemblyStateCreateInfo.flags = flags;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
	return pipelineInputAssemblyStateCreateInfo;
}

inline VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info (VkPolygonMode polygonMode,
    VkCullModeFlags cullMode,
    VkFrontFace frontFace,
    VkPipelineRasterizationStateCreateFlags flags = 0)
{
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
	pipelineRasterizationStateCreateInfo.cullMode = cullMode;
	pipelineRasterizationStateCreateInfo.frontFace = frontFace;
	pipelineRasterizationStateCreateInfo.flags = flags;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	return pipelineRasterizationStateCreateInfo;
}

inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState (
    VkColorComponentFlags colorWriteMask, VkBool32 blendEnable)
{
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
	pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
	pipelineColorBlendAttachmentState.blendEnable = blendEnable;
	return pipelineColorBlendAttachmentState;
}

inline VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info (
    std::vector<VkPipelineColorBlendAttachmentState> const& attachments)
{
	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
	pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t> (attachments.size ());
	pipelineColorBlendStateCreateInfo.pAttachments = attachments.data ();
	return pipelineColorBlendStateCreateInfo;
}

inline VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info (
    VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp)
{
	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
	pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;
	pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	return pipelineDepthStencilStateCreateInfo;
}

inline VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info (std::vector<VkViewport> const& viewports,
    std::vector<VkRect2D> const& scissors,
    VkPipelineViewportStateCreateFlags flags = 0)
{
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.viewportCount = static_cast<uint32_t> (viewports.size ());
	pipelineViewportStateCreateInfo.scissorCount = static_cast<uint32_t> (scissors.size ());
	pipelineViewportStateCreateInfo.pViewports = viewports.data ();
	pipelineViewportStateCreateInfo.pScissors = scissors.data ();

	pipelineViewportStateCreateInfo.flags = flags;
	return pipelineViewportStateCreateInfo;
}

inline VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info (
    VkSampleCountFlagBits rasterizationSamples, VkPipelineMultisampleStateCreateFlags flags = 0)
{
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
	pipelineMultisampleStateCreateInfo.flags = flags;
	return pipelineMultisampleStateCreateInfo;
}

inline VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info (
    std::vector<VkDynamicState> const& pDynamicStates, VkPipelineDynamicStateCreateFlags flags = 0)
{
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data ();
	pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t> (pDynamicStates.size ());
	pipelineDynamicStateCreateInfo.flags = flags;
	return pipelineDynamicStateCreateInfo;
}

inline VkPipelineTessellationStateCreateInfo pipeline_tesselllation_state_create_info (uint32_t patchControlPoints)
{
	VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
	pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
	return pipelineTessellationStateCreateInfo;
}

inline VkGraphicsPipelineCreateInfo graphics_pipeline_create_info (
    VkPipelineLayout layout, VkRenderPass renderPass, uint32_t subpass, VkPipelineCreateFlags flags = 0)
{
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = layout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = subpass;
	pipelineCreateInfo.flags = flags;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	return pipelineCreateInfo;
}

inline VkComputePipelineCreateInfo compute_pipeline_create_info (
    VkPipelineLayout layout, VkPipelineCreateFlags flags = 0)
{
	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.flags = flags;
	return computePipelineCreateInfo;
}

inline VkPushConstantRange push_constant_range (VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = stageFlags;
	pushConstantRange.offset = offset;
	pushConstantRange.size = size;
	return pushConstantRange;
}

inline VkBindSparseInfo bind_sparse_info ()
{
	VkBindSparseInfo bindSparseInfo{};
	bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
	return bindSparseInfo;
}

/** @brief Initialize a map entry for a shader specialization constant */
inline VkSpecializationMapEntry specialization_map_entry (uint32_t constantID, uint32_t offset, size_t size)
{
	VkSpecializationMapEntry specializationMapEntry{};
	specializationMapEntry.constantID = constantID;
	specializationMapEntry.offset = offset;
	specializationMapEntry.size = size;
	return specializationMapEntry;
}

/** @brief Initialize a specialization constant info structure to pass to a shader stage */
inline VkSpecializationInfo specialization_info (
    uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
{
	VkSpecializationInfo specializationInfo{};
	specializationInfo.mapEntryCount = mapEntryCount;
	specializationInfo.pMapEntries = mapEntries;
	specializationInfo.dataSize = dataSize;
	specializationInfo.pData = data;
	return specializationInfo;
}
} // namespace initializers