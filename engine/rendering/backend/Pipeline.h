#pragma once

#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>

#include "SG14/flat_map.h"

#include "Shader.h"

class VulkanDevice;
class AsyncTaskManager;
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

using PipeID = int;

struct SpecificPass
{
	VkRenderPass render_pass;
	uint32_t subpass;
};

bool operator== (SpecificPass const& a, SpecificPass const& b);

namespace std
{
template <> struct hash<SpecificPass>
{
	std::size_t operator() (SpecificPass const& s) const noexcept
	{
		std::size_t h1 = std::hash<VkRenderPass>{}(s.render_pass);
		std::size_t h2 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.subpass));
		return h1 ^ (h2 << 16);
	}
};
} // namespace std

class PipelineGroup
{
	public:
	PipelineGroup (
	    VkDevice device, VkPipelineCache cache, PipelineOutline builder, std::vector<SpecificPass> const& initial_passes);
	~PipelineGroup ();
	PipelineGroup (const PipelineGroup& group) = delete;
	PipelineGroup& operator= (const PipelineGroup& group) = delete;
	PipelineGroup (PipelineGroup&& group);
	PipelineGroup& operator= (PipelineGroup&& group) noexcept;

	void CreatePipeline (SpecificPass pass);

	void Bind (VkCommandBuffer cmdBuf, SpecificPass pass);

	VkPipelineLayout GetLayout () { return layout; }

	private:
	VkDevice device;
	PipelineOutline builder;
	VkPipelineCache cache;
	VkPipelineLayout layout;
	std::unordered_map<SpecificPass, VkPipeline> pipelines;
};

class PipelineManager
{
	public:
	PipelineManager (VulkanDevice& device, AsyncTaskManager& async_man);
	~PipelineManager ();

	PipeID MakePipeGroup (PipelineOutline builder);
	void MakePipe (PipeID ID, SpecificPass pass);

	void BindPipe (VkCommandBuffer cmdBuf, SpecificPass pass, PipeID ID);
	VkPipelineLayout GetPipeLayout (PipeID ID);

	VkPipelineCache GetCache () const;

	private:
	VulkanDevice& device;
	AsyncTaskManager& async_man;
	VkPipelineCache cache;

	std::mutex pipe_lock;
	std::unordered_map<int, PipelineGroup> pipelines;
	PipeID cur_id = 0;
};