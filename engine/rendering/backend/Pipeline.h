#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "Shader.h"

struct VertexLayout;
class PipelineOutline
{
	public:
	void SetShaderModuleSet (ShaderModuleSet set);
	void UseModelVertexLayout (VertexLayout const& layout);
	void AddVertexLayout (VkVertexInputBindingDescription bind, VkVertexInputAttributeDescription attrib);
	void AddVertexLayouts (std::vector<VkVertexInputBindingDescription> binds,
	    std::vector<VkVertexInputAttributeDescription> attribs);


	void AddDescriptorLayouts (std::vector<VkDescriptorSetLayout> layouts);
	void AddDescriptorLayout (VkDescriptorSetLayout layout);

	void SetInputAssembly (VkPrimitiveTopology topology, VkBool32 primitiveRestart);

	void AddViewport (float width, float height, float minDepth, float maxDepth, float x = 0.0f, float y = 0.0f);
	void AddViewport (VkViewport viewport);

	void AddScissor (uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY);
	void AddScissor (VkRect2D scissor);

	void SetRasterizer (VkPolygonMode polygonMode,
	    VkCullModeFlagBits cullModeFlagBits,
	    VkFrontFace frontFace,
	    VkBool32 depthClampEnable,
	    VkBool32 rasterizerDiscardEnable,
	    float lineWidth,
	    VkBool32 depthBiasEnable);

	void SetMultisampling (VkSampleCountFlagBits sampleCountFlags);

	void SetDepthStencil (VkBool32 depthTestEnable,
	    VkBool32 depthWriteEnable,
	    VkCompareOp depthCompareOp,
	    VkBool32 depthBoundsTestEnable,
	    VkBool32 stencilTestEnable);

	void AddColorBlendingAttachment (VkBool32 blendEnable,
	    VkColorComponentFlags colorWriteMask,
	    VkBlendOp colorBlendOp,
	    VkBlendFactor srcColorBlendFactor,
	    VkBlendFactor dstColorBlendFactor,
	    VkBlendOp alphaBlendOp,
	    VkBlendFactor srcAlphaBlendFactor,
	    VkBlendFactor dstAlphaBlendFactor);
	void AddColorBlendingAttachment (VkPipelineColorBlendAttachmentState attachment);


	void SetDescriptorSetLayout (std::vector<VkDescriptorSetLayout>& descriptorSetlayouts);

	void AddPushConstantRange (VkPushConstantRange pushConstantRange);

	void AddDynamicStates (std::vector<VkDynamicState> states);
	void AddDynamicState (VkDynamicState dynamicStates);

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
};

class PipelineLayout
{
	public:
	PipelineLayout (VkDevice device, VkPipelineLayout layout);
	~PipelineLayout ();
	PipelineLayout (const PipelineLayout& other) = delete;
	PipelineLayout& operator= (const PipelineLayout& other) = delete;
	PipelineLayout (PipelineLayout&& other);
	PipelineLayout& operator= (PipelineLayout&& other) noexcept;

	VkPipelineLayout get () const { return layout; }

	private:
	VkDevice device;
	VkPipelineLayout layout;
};
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

class AsyncTaskManager;

class PipelineManager
{
	public:
	PipelineManager (VkDevice device, AsyncTaskManager& async_man);
	~PipelineManager ();

	std::optional<PipelineLayout> CreatePipelineLayout (std::vector<VkDescriptorSetLayout> desc_set_layouts,
	    std::vector<VkPushConstantRange> push_constant_ranges) const;

	std::optional<GraphicsPipeline> CreateGraphicsPipeline (
	    PipelineLayout const& layout, PipelineOutline builder, VkRenderPass render_pass, uint32_t subpass) const;

	VkPipelineCache GetCache () const;

	private:
	VkDevice device;
	AsyncTaskManager& async_man;
	VkPipelineCache cache;
};