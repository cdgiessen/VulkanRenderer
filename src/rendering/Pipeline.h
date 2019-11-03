#pragma once

#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>

#include "SG14/flat_map.h"

#include "Shader.h"

class VulkanDevice;
class VulkanLayout;
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

	void SetColorBlending (uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* attachments);

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


class Pipeline
{
	public:
	Pipeline (VulkanDevice& device, VkPipelineCache cache, PipelineOutline builder, VkRenderPass renderPass, int subPass = 0);
	~Pipeline ();
	Pipeline (Pipeline const& pipe) = delete;
	Pipeline& operator= (Pipeline const& pipe) = delete;
	Pipeline (Pipeline& pipe);
	Pipeline& operator= (Pipeline&& pipe);


	VkPipeline pipeline;
};

using PipeID = int;



struct Specific
{
	VkRenderPass render_pass;
	uint32_t subpass;
}

struct PipelineGroup
{
	PipelineGroup (VulkanDevice& device, VkPipelineCache cache, PipelineOutline builder);

	void AddPipeline (VkRenderPass render_pass, uint32_t subpass);

	void Bind (VkRenderPass render_pass, )

	    PipelineOutline builder;
	VkPipelineCache cache;
	VkPipelineLayout layout;
	stdext::flat_map<VkRenderPass, Pipeline> pipelines;
};

class PipelineManager
{
	public:
	PipelineManager (VulkanDevice& device);
	~PipelineManager ();

	PipeID MakePipe (PipelineOutline builder, VkRenderPass renderPass, int subPass = 0);

	void BindPipe (PipeID ID, VkCommandBuffer cmdBuf);
	VkPipelineLayout GetPipeLayout (PipeID ID);

	private:
	VulkanDevice& device;
	VkPipelineCache cache;

	std::mutex pipe_lock;
	std::unordered_map<int, Pipeline> pipelines;
	PipeID cur_id = 0;
};