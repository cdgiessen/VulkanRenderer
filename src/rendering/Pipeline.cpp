#include "Pipeline.h"

#include "Device.h"
#include "Initializers.h"
#include "Model.h"
#include "RenderStructs.h"
#include "RenderTools.h"
#include "Renderer.h"

////// BASIC PIPELINE //////


void PipelineOutline::SetShaderModuleSet (ShaderModuleSet set) { this->set = set; }
void PipelineOutline::UseModelVertexLayout (VulkanModel* model)
{
	auto vl = model->GetVertexLayout ();
	vertexInputBindingDescription.insert (
	    std::end (vertexInputBindingDescription), std::begin (vl.bindingDesc), std::end (vl.bindingDesc));
	vertexInputAttributeDescriptions.insert (
	    std::end (vertexInputAttributeDescriptions), std::begin (vl.attribDesc), std::end (vl.attribDesc));
}


void PipelineOutline::AddVertexLayout (VkVertexInputBindingDescription bind, VkVertexInputAttributeDescription attrib)
{
	vertexInputBindingDescription.push_back (bind);
	vertexInputAttributeDescriptions.push_back (attrib);
}

void PipelineOutline::AddVertexLayouts (std::vector<VkVertexInputBindingDescription> binds,
    std::vector<VkVertexInputAttributeDescription> attribs)
{
	vertexInputBindingDescription.insert (
	    std::end (vertexInputBindingDescription), std::begin (binds), std::end (binds));
	vertexInputAttributeDescriptions.insert (
	    std::end (vertexInputAttributeDescriptions), std::begin (attribs), std::end (attribs));
}

void PipelineOutline::SetInputAssembly (VkPrimitiveTopology topology, VkBool32 primitiveRestart)
{
	inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo (topology, primitiveRestart);
}

void PipelineOutline::AddDescriptorLayouts (std::vector<VkDescriptorSetLayout> layouts)
{
	this->layouts.insert (std::end (this->layouts), std::begin (layouts), std::end (layouts));
}
void PipelineOutline::AddDescriptorLayout (VkDescriptorSetLayout layout)
{
	layouts.push_back (layout);
}

void PipelineOutline::AddViewport (VkViewport viewport) { viewports.push_back (viewport); }

void PipelineOutline::AddViewport (float width, float height, float minDepth, float maxDepth, float x, float y)
{
	auto viewport = initializers::viewport (width, height, minDepth, maxDepth);
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewports.push_back (viewport);
}

void PipelineOutline::AddScissor (VkRect2D scissor) { scissors.push_back (scissor); }
void PipelineOutline::AddScissor (uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY)
{
	scissors.push_back (initializers::rect2D (width, height, offsetX, offsetY));
}


void PipelineOutline::SetRasterizer (VkPolygonMode polygonMode,
    VkCullModeFlagBits cullModeFlagBits,
    VkFrontFace frontFace,
    VkBool32 depthClampEnable,
    VkBool32 rasterizerDiscardEnable,
    float lineWidth,
    VkBool32 depthBiasEnable)
{
	rasterizer = initializers::pipelineRasterizationStateCreateInfo (polygonMode, cullModeFlagBits, frontFace);
	rasterizer.depthClampEnable = depthClampEnable;
	rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
	rasterizer.lineWidth = lineWidth;
	rasterizer.depthBiasEnable = depthBiasEnable;
}

// No Multisampling support right now
void PipelineOutline::SetMultisampling (VkSampleCountFlagBits sampleCountFlags)
{
	multisampling = initializers::pipelineMultisampleStateCreateInfo (sampleCountFlags);
	multisampling.sampleShadingEnable = VK_FALSE;
}

void PipelineOutline::SetDepthStencil (VkBool32 depthTestEnable,
    VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp,
    VkBool32 depthBoundsTestEnable,
    VkBool32 stencilTestEnable)
{
	depthStencil = initializers::pipelineDepthStencilStateCreateInfo (
	    depthTestEnable, depthWriteEnable, depthCompareOp);
	depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
	depthStencil.stencilTestEnable = stencilTestEnable;
}

void PipelineOutline::AddColorBlendingAttachment (VkBool32 blendEnable,
    VkColorComponentFlags colorWriteMask,
    VkBlendOp colorBlendOp,
    VkBlendFactor srcColorBlendFactor,
    VkBlendFactor dstColorBlendFactor,
    VkBlendOp alphaBlendOp,
    VkBlendFactor srcAlphaBlendFactor,
    VkBlendFactor dstAlphaBlendFactor)
{
	auto colorBlendAttachment = initializers::pipelineColorBlendAttachmentState (colorWriteMask, blendEnable);
	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
	colorBlendAttachments.push_back (colorBlendAttachment);
}

void PipelineOutline::AddDynamicStates (std::vector<VkDynamicState> states)
{
	dynamicStates.insert (std::end (dynamicStates), std::begin (states), std::end (states));
}
void PipelineOutline::AddDynamicState (VkDynamicState state) { dynamicStates.push_back (state); }


void PipelineOutline::AddPushConstantRange (VkPushConstantRange pushConstantRange)
{
	pushConstantRanges.push_back (pushConstantRange);
}

Pipeline::Pipeline (VulkanRenderer& renderer, PipelineOutline builder, VkRenderPass renderPass, int subPass)
: renderer (renderer)
{
	auto layoutInfo = initializers::pipelineLayoutCreateInfo (builder.layouts, builder.pushConstantRanges);

	if (vkCreatePipelineLayout (renderer.device.device, &layoutInfo, nullptr, &this->layout) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create pipeline layout!");
	}



	auto info = initializers::pipelineCreateInfo (layout, renderPass, 0);
	info.layout = layout;
	info.subpass = subPass; // which subpass in the renderpass this pipeline gets used
	info.basePipelineHandle = VK_NULL_HANDLE;

	auto shaderStages = builder.set.ShaderStageCreateInfos ();
	info.stageCount = (uint32_t)shaderStages.size ();
	info.pStages = shaderStages.data ();

	info.pInputAssemblyState = &builder.inputAssembly;

	auto vertexInput = initializers::pipelineVertexInputStateCreateInfo (
	    builder.vertexInputBindingDescription, builder.vertexInputAttributeDescriptions);

	info.pVertexInputState = &vertexInput;

	auto viewportInfo =
	    initializers::pipelineViewportStateCreateInfo (builder.viewports, builder.scissors, 0);
	info.pViewportState = &viewportInfo;

	info.pRasterizationState = &builder.rasterizer;
	info.pMultisampleState = &builder.multisampling;
	info.pDepthStencilState = &builder.depthStencil;

	auto colorBlending = initializers::pipelineColorBlendStateCreateInfo (builder.colorBlendAttachments);
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	info.pColorBlendState = &colorBlending;

	VkPipelineDynamicStateCreateInfo dynamicInfo =
	    initializers::pipelineDynamicStateCreateInfo (builder.dynamicStates, 0);
	info.pDynamicState = &dynamicInfo;

	if (vkCreateGraphicsPipelines (renderer.device.device, nullptr, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create graphics pipeline!");
	}
}

Pipeline::~Pipeline ()
{
	vkDestroyPipeline (renderer.device.device, pipeline, nullptr);
	vkDestroyPipelineLayout (renderer.device.device, layout, nullptr);
}

void Pipeline::Bind (VkCommandBuffer cmdBuf)
{
	vkCmdBindPipeline (cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

VkPipelineLayout Pipeline::GetLayout () { return layout; }