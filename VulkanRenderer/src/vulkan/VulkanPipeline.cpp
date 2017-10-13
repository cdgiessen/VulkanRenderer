#include "VulkanPipeline.hpp"

#include "..\core\Mesh.h"

VulkanPipeline::VulkanPipeline(VulkanDevice* device) : device(device)
{
}


VulkanPipeline::~VulkanPipeline()
{
}

PipelineCreationObject* VulkanPipeline::CreatePipelineOutline()
{
	PipelineCreationObject *pco = new PipelineCreationObject();
	
	return pco;
}

VkPipelineLayout VulkanPipeline::BuildPipelineLayout(PipelineCreationObject *pco) {
	if (vkCreatePipelineLayout(device->device, &pco->pipelineLayoutInfo, nullptr, &pco->pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
	return pco->pipelineLayout;
}

VkPipeline VulkanPipeline::BuildPipeline(PipelineCreationObject *pco, VkRenderPass renderPass, VkPipelineCreateFlags flags)
{
	//Deals with possible geometry or tessilation shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back(pco->vertShaderStageInfo);
	shaderStages.push_back(pco->fragShaderStageInfo);
	if (pco->geomShader) {
		shaderStages.push_back(pco->geomShaderStageInfo);
	}
	if (pco->tessShader) {
		shaderStages.push_back(pco->tessShaderStageInfo);
	}

	pco->pipelineInfo = initializers::pipelineCreateInfo(pco->pipelineLayout, renderPass, flags);
	pco->pipelineInfo.stageCount = shaderStages.size();
	pco->pipelineInfo.pStages = shaderStages.data();

	pco->pipelineInfo.pVertexInputState = &pco->vertexInputInfo;
	pco->pipelineInfo.pInputAssemblyState = &pco->inputAssembly;
	pco->pipelineInfo.pViewportState = &pco->viewportState;
	pco->pipelineInfo.pRasterizationState = &pco->rasterizer;
	pco->pipelineInfo.pMultisampleState = &pco->multisampling;
	pco->pipelineInfo.pDepthStencilState = &pco->depthStencil;
	pco->pipelineInfo.pColorBlendState = &pco->colorBlending;

	pco->pipelineInfo.subpass = 0; //which subpass in the renderpass this pipeline gets used
	pco->pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	
	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pco->pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	return pipeline;
}



void VulkanPipeline::SetVertexShader(PipelineCreationObject *pco, VkShaderModule vert)
{
	pco->vertShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vert);
	pco->vertShaderStageInfo.pName = "main";
	pco->vertShaderModule = vert;
}

void VulkanPipeline::SetFragmentShader(PipelineCreationObject *pco, VkShaderModule frag)
{
	pco->fragShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, frag);
	pco->fragShaderStageInfo.pName = "main";
	pco->fragShaderModule = frag;
}

void VulkanPipeline::SetGeometryShader(PipelineCreationObject *pco, VkShaderModule geom)
{
	pco->geomShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, geom);
	pco->geomShaderStageInfo.pName = "main";
	pco->geomShader = true;
	pco->geomShaderModule = geom;
}

//this shouldn't work (cause I have no clue how tess shaders work...)
void VulkanPipeline::SetTesselationShader(PipelineCreationObject *pco, VkShaderModule tess)
{
	pco->tessShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, tess);
	pco->tessShaderStageInfo.pName = "main";
	pco->tessShader = true;
	pco->tessShaderModule = tess;
}

void VulkanPipeline::CleanShaderResources(PipelineCreationObject *pco) {
	vkDestroyShaderModule(device->device, pco->vertShaderModule, nullptr);
	vkDestroyShaderModule(device->device, pco->fragShaderModule, nullptr);
	vkDestroyShaderModule(device->device, pco->geomShaderModule, nullptr);
	vkDestroyShaderModule(device->device, pco->tessShaderModule, nullptr);
}

void VulkanPipeline::SetVertexInput(PipelineCreationObject *pco, std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions)
{
	pco->vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo();
	
	pco->vertexInputBindingDescription = bindingDescription;
	pco->vertexInputAttributeDescriptions = attributeDescriptions;

	pco->vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(pco->vertexInputBindingDescription.size());
	pco->vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(pco->vertexInputAttributeDescriptions.size());
	pco->vertexInputInfo.pVertexBindingDescriptions = pco->vertexInputBindingDescription.data();
	pco->vertexInputInfo.pVertexAttributeDescriptions = pco->vertexInputAttributeDescriptions.data();
 
}

void VulkanPipeline::SetInputAssembly(PipelineCreationObject *pco, VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flag, VkBool32 primitiveRestart)
{
	pco->inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(topology, flag, primitiveRestart);
}

void VulkanPipeline::SetDynamicState(PipelineCreationObject *pco, uint32_t dynamicStateCount, VkDynamicState* pDynamicStates, VkPipelineDynamicStateCreateFlags flags) {
	pco->dynamicState = VkPipelineDynamicStateCreateInfo();
	pco->dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pco->dynamicState.flags = flags;
	pco->dynamicState.dynamicStateCount = dynamicStateCount;
	pco->dynamicState.pDynamicStates = pDynamicStates;
}

void VulkanPipeline::SetViewport(PipelineCreationObject *pco, float width, float height, float minDepth, float maxDepth, float x, float y)
{
	pco->viewport = initializers::viewport(width, height, minDepth, maxDepth);
	pco->viewport.x = 0.0f;
	pco->viewport.y = 0.0f;
}

void VulkanPipeline::SetScissor(PipelineCreationObject *pco, uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY)
{
	pco->scissor = initializers::rect2D(width, height, offsetX, offsetY);
}

//Currently only supports one viewport or scissor
void VulkanPipeline::SetViewportState(PipelineCreationObject *pco, uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags)
{ 
	pco->viewportState = initializers::pipelineViewportStateCreateInfo(1, 1);
	pco->viewportState.pViewports = &pco->viewport;
	pco->viewportState.pScissors = &pco->scissor;
}

void VulkanPipeline::SetRasterizer(PipelineCreationObject *pco, VkPolygonMode polygonMode, VkCullModeFlagBits cullModeFlagBits, VkFrontFace frontFace, 
		VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable)
{
	pco->rasterizer = initializers::pipelineRasterizationStateCreateInfo( polygonMode, cullModeFlagBits, frontFace);
	pco->rasterizer.depthClampEnable = depthClampEnable;
	pco->rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
	pco->rasterizer.lineWidth = lineWidth;
	pco->rasterizer.depthBiasEnable = depthBiasEnable;
}

//No Multisampling support right now
void VulkanPipeline::SetMultisampling(PipelineCreationObject *pco, VkSampleCountFlagBits sampleCountFlags)
{
	pco->multisampling = initializers::pipelineMultisampleStateCreateInfo(sampleCountFlags);
	pco->multisampling.sampleShadingEnable = VK_FALSE;
}

void VulkanPipeline::SetDepthStencil(PipelineCreationObject *pco, VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable)
{
	pco->depthStencil = initializers::pipelineDepthStencilStateCreateInfo(depthTestEnable, depthWriteEnable, depthCompareOp);
	pco->depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
	pco->depthStencil.stencilTestEnable = stencilTestEnable;
}

void VulkanPipeline::SetColorBlendingAttachment(PipelineCreationObject *pco, VkBool32 blendEnable, VkColorComponentFlags colorWriteMask, 
	VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor)
{
	pco->colorBlendAttachment = initializers::pipelineColorBlendAttachmentState(colorWriteMask, blendEnable);
	pco->colorBlendAttachment.colorBlendOp = colorBlendOp;
	pco->colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	pco->colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	pco->colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	pco->colorBlendAttachment.srcAlphaBlendFactor= srcAlphaBlendFactor;
	pco->colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
}

//Can't handle more than one attachment currently
void VulkanPipeline::SetColorBlending(PipelineCreationObject *pco, uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState * attachments)
{
	pco->colorBlending = initializers::pipelineColorBlendStateCreateInfo(1, &pco->colorBlendAttachment);
	pco->colorBlending.logicOpEnable = VK_FALSE;
	pco->colorBlending.logicOp = VK_LOGIC_OP_COPY;
	pco->colorBlending.blendConstants[0] = 0.0f;
	pco->colorBlending.blendConstants[1] = 0.0f;
	pco->colorBlending.blendConstants[2] = 0.0f;
	pco->colorBlending.blendConstants[3] = 0.0f;
}

void VulkanPipeline::SetDescriptorSetLayout(PipelineCreationObject *pco, VkDescriptorSetLayout* descriptorSetlayouts, uint32_t descritorSetLayoutCount)
{
	pco->pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(descriptorSetlayouts, descritorSetLayoutCount);
}
