#include "Pipeline.h"

#include <filesystem>
#include <fstream>

#include "resources/Mesh.h"

#include "AsyncTask.h"
#include "Model.h"
#include "rendering/Initializers.h"

void PipelineOutline::SetShaderModuleSet (ShaderModuleSet set) { this->set = set; }
void PipelineOutline::UseModelVertexLayout (VertexLayout const& layout)
{
	for (auto& desc : layout.bindingDesc)
		vertexInputBindingDescription.push_back (desc);
	for (auto& desc : layout.attribDesc)
		vertexInputAttributeDescriptions.push_back (desc);
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
	viewport.x = x;
	viewport.y = y;
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

void PipelineOutline::AddColorBlendingAttachment (VkPipelineColorBlendAttachmentState attachment)
{
	colorBlendAttachments.push_back (attachment);
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

PipelineLayout::PipelineLayout (VkDevice device, VkPipelineLayout layout)
: device (device), layout (layout)
{
}

PipelineLayout::~PipelineLayout ()
{
	if (layout != nullptr) vkDestroyPipelineLayout (device, layout, nullptr);
}

PipelineLayout::PipelineLayout (PipelineLayout&& other) noexcept
: device (other.device), layout (other.layout)
{
	other.layout = VK_NULL_HANDLE;
}

PipelineLayout& PipelineLayout::operator= (PipelineLayout&& other) noexcept
{
	device = other.device;
	layout = other.layout;
	other.layout = nullptr;
	return *this;
}

GraphicsPipeline::GraphicsPipeline (VkDevice device, VkPipeline pipeline)
: device (device), pipeline (pipeline)
{
}
GraphicsPipeline::~GraphicsPipeline ()
{
	if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline (device, pipeline, nullptr);
}
GraphicsPipeline::GraphicsPipeline (GraphicsPipeline&& other) noexcept
: device (other.device), pipeline (other.pipeline)
{
	other.pipeline = VK_NULL_HANDLE;
}
GraphicsPipeline& GraphicsPipeline::operator= (GraphicsPipeline&& other) noexcept
{
	device = other.device;
	pipeline = other.pipeline;
	other.pipeline = VK_NULL_HANDLE;
	return *this;
}
void GraphicsPipeline::Bind (VkCommandBuffer cmdBuf)
{
	vkCmdBindPipeline (cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}


PipelineManager::PipelineManager (VkDevice device, AsyncTaskManager& async_man)
: device (device), async_man (async_man)
{
	std::vector<std::byte> cache_data;

	if (std::filesystem::exists (".cache/pipeline_cache"))
	{
		std::ifstream in (".cache/pipeline_cache", std::ios::binary | std::ios::ate);
		auto size = in.tellg ();
		std::vector<std::byte> cache_data (size); // construct string to stream size
		in.seekg (0);
		in.read (reinterpret_cast<char*> (cache_data.data ()), size);
	}

	VkPipelineCacheCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	info.initialDataSize = cache_data.size ();
	info.pInitialData = cache_data.data ();

	vkCreatePipelineCache (device, &info, nullptr, &cache);
}

PipelineManager::~PipelineManager ()
{
	std::ofstream out (".cache/pipeline_cache", std::ios::binary | std::ios::out);
	size_t cache_size = 0;
	std::vector<std::byte> cache_data;
	vkGetPipelineCacheData (device, cache, &cache_size, nullptr);
	cache_data.resize (cache_size);
	vkGetPipelineCacheData (device, cache, &cache_size, cache_data.data ());

	out.write (reinterpret_cast<char*> (cache_data.data ()), cache_data.size ());

	vkDestroyPipelineCache (device, cache, nullptr);
}

VkPipelineCache PipelineManager::GetCache () const { return cache; }

std::optional<PipelineLayout> PipelineManager::CreatePipelineLayout (
    std::vector<VkDescriptorSetLayout> desc_set_layouts, std::vector<VkPushConstantRange> push_constant_ranges) const
{
	auto layoutInfo = initializers::pipelineLayoutCreateInfo (desc_set_layouts, push_constant_ranges);
	VkPipelineLayout layout;
	VkResult res = vkCreatePipelineLayout (device, &layoutInfo, nullptr, &layout);
	if (res != VK_SUCCESS) return {};
	return PipelineLayout{ device, layout };
}

std::optional<GraphicsPipeline> PipelineManager::CreateGraphicsPipeline (
    PipelineLayout const& layout, PipelineOutline builder, VkRenderPass render_pass, uint32_t subpass) const
{
	VkPipeline pipe;

	auto info = initializers::pipelineCreateInfo (layout.get (), render_pass, subpass, 0);
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

	VkResult res = vkCreateGraphicsPipelines (device, cache, 1, &info, nullptr, &pipe);
	if (res != VK_SUCCESS)
	{
		return {};
	}
	return GraphicsPipeline{ device, pipe };
}
