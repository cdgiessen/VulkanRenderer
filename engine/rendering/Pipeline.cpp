#include "Pipeline.h"

#include <filesystem>
#include <fstream>

#include "resources/Mesh.h"

#include "AsyncTask.h"
#include "Device.h"
#include "Initializers.h"
#include "Model.h"

bool operator== (SpecificPass const& a, SpecificPass const& b)
{
	return a.render_pass == b.render_pass && a.subpass == b.subpass;
}

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

PipelineGroup::PipelineGroup (
    VkDevice device, VkPipelineCache cache, PipelineOutline builder, std::vector<SpecificPass> const& initial_passes)
: device (device), cache (cache), builder (builder)
{
	auto layoutInfo = initializers::pipelineLayoutCreateInfo (builder.layouts, builder.pushConstantRanges);

	if (vkCreatePipelineLayout (device, &layoutInfo, nullptr, &this->layout) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create pipeline layout!");
	}

	for (auto& pass : initial_passes)
	{
		CreatePipeline (pass);
	}
}

PipelineGroup::~PipelineGroup ()
{
	for (auto const& [pass, pipe] : pipelines)
	{
		vkDestroyPipeline (device, pipe, nullptr);
	}
	if (layout != nullptr) vkDestroyPipelineLayout (device, layout, nullptr);
}

PipelineGroup::PipelineGroup (PipelineGroup&& group)
: device (group.device),
  cache (group.cache),
  builder (group.builder),
  layout (group.layout),
  pipelines (group.pipelines)
{
}

PipelineGroup& PipelineGroup::operator= (PipelineGroup&& group) noexcept
{
	device = group.device;
	cache = group.cache;
	builder = group.builder;
	layout = group.layout;
	group.layout = nullptr;
	pipelines = std::move (group.pipelines);
	return *this;
}

void PipelineGroup::CreatePipeline (SpecificPass pass)
{
	VkPipeline pipe;

	auto info = initializers::pipelineCreateInfo (layout, pass.render_pass, pass.subpass, 0);
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

	if (vkCreateGraphicsPipelines (device, cache, 1, &info, nullptr, &pipe) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create graphics pipeline!");
	}
	pipelines.emplace (pass, pipe);
}

void PipelineGroup::Bind (VkCommandBuffer cmdBuf, SpecificPass pass)
{
	if (pipelines.count (pass) == 0)
	{
		CreatePipeline (pass);
	}
	vkCmdBindPipeline (cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.at (pass));
}

PipelineManager::PipelineManager (VulkanDevice& device, AsyncTaskManager& async_man)
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

	vkCreatePipelineCache (device.device, &info, nullptr, &cache);
}

PipelineManager::~PipelineManager ()
{
	std::ofstream out (".cache/pipeline_cache", std::ios::binary | std::ios::out);
	size_t cache_size = 0;
	std::vector<std::byte> cache_data;
	vkGetPipelineCacheData (device.device, cache, &cache_size, nullptr);
	cache_data.resize (cache_size);
	vkGetPipelineCacheData (device.device, cache, &cache_size, cache_data.data ());

	out.write (reinterpret_cast<char*> (cache_data.data ()), cache_data.size ());

	vkDestroyPipelineCache (device.device, cache, nullptr);
}

PipeID PipelineManager::MakePipeGroup (PipelineOutline builder)
{
	auto pipe = PipelineGroup (device.device, cache, builder, {});
	std::lock_guard guard (pipe_lock);
	pipelines.emplace (cur_id, std::move (pipe));
	return cur_id++;
}
void PipelineManager::MakePipe (PipeID ID, SpecificPass pass)
{
	std::lock_guard guard (pipe_lock);
	pipelines.at (ID).CreatePipeline (pass);
}

void PipelineManager::BindPipe (VkCommandBuffer cmdBuf, SpecificPass pass, PipeID ID)
{
	std::lock_guard guard (pipe_lock);
	pipelines.at (ID).Bind (cmdBuf, pass);
}
VkPipelineLayout PipelineManager::GetPipeLayout (PipeID ID)
{
	std::lock_guard guard (pipe_lock);
	return pipelines.at (ID).GetLayout ();
}

VkPipelineCache PipelineManager::GetCache () const { return cache; }