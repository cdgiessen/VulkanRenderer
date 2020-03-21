#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "Shader.h"

class PipelineLayout
{
	public:
	PipelineLayout (VkDevice device, VkPipelineLayout layout);
	PipelineLayout (VkDevice device,
	    std::vector<VkDescriptorSetLayout> desc_set_layouts,
	    std::vector<VkPushConstantRange> push_constant_ranges);

	~PipelineLayout ();
	PipelineLayout (const PipelineLayout& other) = delete;
	PipelineLayout& operator= (const PipelineLayout& other) = delete;
	PipelineLayout (PipelineLayout&& other) noexcept;
	PipelineLayout& operator= (PipelineLayout&& other) noexcept;

	VkPipelineLayout get () const { return layout; }

	private:
	VkDevice device;
	VkPipelineLayout layout;
};

std::optional<PipelineLayout> CreatePipelineLayout (VkDevice device,
    std::vector<VkDescriptorSetLayout> desc_set_layouts,
    std::vector<VkPushConstantRange> push_constant_ranges);


class GraphicsPipeline
{
	public:
	GraphicsPipeline (VkDevice device, VkPipeline pipeline);

	~GraphicsPipeline ();
	GraphicsPipeline (const GraphicsPipeline& other) = delete;
	GraphicsPipeline& operator= (const GraphicsPipeline& other) = delete;
	GraphicsPipeline (GraphicsPipeline&& other) noexcept;
	GraphicsPipeline& operator= (GraphicsPipeline&& other) noexcept;

	void Bind (VkCommandBuffer buffer);

	private:
	VkDevice device = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
};

struct VertexLayout;
class PipelineBuilder
{
	public:
	PipelineBuilder (VkDevice device, VkPipelineCache cache);

	std::optional<PipelineLayout> CreateLayout () const;
	std::optional<GraphicsPipeline> CreatePipeline (
	    PipelineLayout const& layout, VkRenderPass render_pass, uint32_t subpass) const;

	PipelineBuilder& SetShaderModuleSet (ShaderModuleSet set);
	PipelineBuilder& UseModelVertexLayout (VertexLayout const& layout);
	PipelineBuilder& AddVertexLayout (
	    VkVertexInputBindingDescription bind, VkVertexInputAttributeDescription attrib);
	PipelineBuilder& AddVertexLayouts (std::vector<VkVertexInputBindingDescription> binds,
	    std::vector<VkVertexInputAttributeDescription> attribs);

	PipelineBuilder& AddDescriptorLayouts (std::vector<VkDescriptorSetLayout> layouts);
	PipelineBuilder& AddDescriptorLayout (VkDescriptorSetLayout layout);

	PipelineBuilder& SetInputAssembly (VkPrimitiveTopology topology, VkBool32 primitiveRestart);

	PipelineBuilder& AddViewport (
	    float width, float height, float minDepth, float maxDepth, float x = 0.0f, float y = 0.0f);
	PipelineBuilder& AddViewport (VkViewport viewport);

	PipelineBuilder& AddScissor (uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY);
	PipelineBuilder& AddScissor (VkRect2D scissor);

	PipelineBuilder& SetRasterizer (VkPolygonMode polygonMode,
	    VkCullModeFlagBits cullModeFlagBits,
	    VkFrontFace frontFace,
	    VkBool32 depthClampEnable,
	    VkBool32 rasterizerDiscardEnable,
	    float lineWidth,
	    VkBool32 depthBiasEnable);

	PipelineBuilder& SetMultisampling (VkSampleCountFlagBits sampleCountFlags);

	PipelineBuilder& SetDepthStencil (VkBool32 depthTestEnable,
	    VkBool32 depthWriteEnable,
	    VkCompareOp depthCompareOp,
	    VkBool32 depthBoundsTestEnable,
	    VkBool32 stencilTestEnable);

	PipelineBuilder& AddColorBlendingAttachment (VkBool32 blendEnable,
	    VkColorComponentFlags colorWriteMask,
	    VkBlendOp colorBlendOp,
	    VkBlendFactor srcColorBlendFactor,
	    VkBlendFactor dstColorBlendFactor,
	    VkBlendOp alphaBlendOp,
	    VkBlendFactor srcAlphaBlendFactor,
	    VkBlendFactor dstAlphaBlendFactor);
	PipelineBuilder& AddColorBlendingAttachment (VkPipelineColorBlendAttachmentState attachment);

	PipelineBuilder& SetDescriptorSetLayout (std::vector<VkDescriptorSetLayout>& descriptorSetlayouts);

	PipelineBuilder& AddPushConstantRange (VkPushConstantRange pushConstantRange);

	PipelineBuilder& AddDynamicStates (std::vector<VkDynamicState> states);
	PipelineBuilder& AddDynamicState (VkDynamicState dynamicStates);

	PipelineBuilder& SetComputeShader (ShaderModule shader_module);

	PipelineBuilder& SetPipelineCache (VkPipelineCache cache);

	ShaderModuleSet set;
	std::vector<VkDescriptorSetLayout> layouts;

	std::vector<VkVertexInputBindingDescription> vertexInputBindingDescription;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissors;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineDepthStencilStateCreateInfo depthStencil;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	VkPipelineColorBlendStateCreateInfo colorBlending;

	std::vector<VkPushConstantRange> pushConstantRanges;

	std::vector<VkDynamicState> dynamicStates;

	VkDevice device;
	VkPipelineCache cache;
};

class ComputePipeline
{
	public:
	ComputePipeline (VkDevice device, VkPipeline pipeline);
	~ComputePipeline ();
	ComputePipeline (const ComputePipeline& other) = delete;
	ComputePipeline& operator= (const ComputePipeline& other) = delete;
	ComputePipeline (ComputePipeline&& other) noexcept;
	ComputePipeline& operator= (ComputePipeline&& other) noexcept;

	void Bind (VkCommandBuffer buffer);

	private:
	VkDevice device = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
};

std::optional<ComputePipeline> BuildComputePipeline (
    VkDevice device, PipelineLayout const& layout, ShaderModule module, VkPipelineCache cache);


class PipelineCache
{
	public:
	PipelineCache (VkDevice device);
	~PipelineCache ();

	PipelineCache (PipelineCache const& other) = delete;
	PipelineCache& operator= (PipelineCache const& other) = delete;

	VkPipelineCache get () const;

	private:
	VkDevice device;
	VkPipelineCache cache;
};