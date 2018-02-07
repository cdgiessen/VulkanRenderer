#pragma once

#include <vector>
#include <functional>
#include <memory>

#include <vulkan/vulkan.h>

class VulkanDevice;

struct PipelineCreationObject {
	bool geomShader;
	bool tessShader;
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	VkShaderModule geomShaderModule;
	VkShaderModule tessShaderModule;
	VkPipelineShaderStageCreateInfo vertShaderStageInfo;
	VkPipelineShaderStageCreateInfo fragShaderStageInfo;
	VkPipelineShaderStageCreateInfo geomShaderStageInfo;
	VkPipelineShaderStageCreateInfo tessShaderStageInfo;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	std::unique_ptr<std::vector<VkVertexInputBindingDescription>> vertexInputBindingDescription;
	std::unique_ptr<std::vector<VkVertexInputAttributeDescription>> vertexInputAttributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlending;

	VkPipelineDynamicStateCreateInfo dynamicState;
	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
	VkGraphicsPipelineCreateInfo pipelineInfo;
};

struct ManagedVulkanPipeline {
	PipelineCreationObject pco;
	VkPipelineLayout layout;
	std::unique_ptr<std::vector<VkPipeline>> pipelines;
};

class VulkanPipeline
{
public:
	VulkanPipeline(const VulkanDevice &device);
	~VulkanPipeline();
	void CleanUp();

	void InitPipelineCache();
	VkPipelineCache GetPipelineCache();

	std::shared_ptr<ManagedVulkanPipeline> CreateManagedPipeline();
	void DeleteManagedPipeline(std::shared_ptr<ManagedVulkanPipeline> pipe);

	void BuildPipelineLayout(std::shared_ptr<ManagedVulkanPipeline> pco); //returns the pipeline layout of the pco (must have done SetDescriptorSetLayout
	void BuildPipeline(std::shared_ptr<ManagedVulkanPipeline> pco, VkRenderPass renderPass, VkPipelineCreateFlags flags);

	void SetVertexShader(std::shared_ptr<ManagedVulkanPipeline> pco, VkShaderModule vert);
	void SetFragmentShader(std::shared_ptr<ManagedVulkanPipeline> pco, VkShaderModule frag);
	void SetGeometryShader(std::shared_ptr<ManagedVulkanPipeline> pco, VkShaderModule geom);
	void SetTesselationShader(std::shared_ptr<ManagedVulkanPipeline> pco, VkShaderModule tess);
	void CleanShaderResources(std::shared_ptr<ManagedVulkanPipeline> pco); //destroys the shader modules after pipeline creation is finished

	void SetVertexInput(std::shared_ptr<ManagedVulkanPipeline> pco, std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions);

	void SetInputAssembly(std::shared_ptr<ManagedVulkanPipeline> pco, VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flag, VkBool32 primitiveRestart);

	void SetViewport(std::shared_ptr<ManagedVulkanPipeline> pco, float width, float height, float minDepth, float maxDepth, float x, float y);
	void SetScissor(std::shared_ptr<ManagedVulkanPipeline> pco, uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY);

	void SetViewportState(std::shared_ptr<ManagedVulkanPipeline> pco, uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags);

	void SetRasterizer(std::shared_ptr<ManagedVulkanPipeline> pco, VkPolygonMode polygonMode, VkCullModeFlagBits cullModeFlagBits, VkFrontFace frontFace, VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable);

	void SetMultisampling(std::shared_ptr<ManagedVulkanPipeline> pco, VkSampleCountFlagBits sampleCountFlags);

	void SetDepthStencil(std::shared_ptr<ManagedVulkanPipeline> pco, VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable);

	void SetColorBlendingAttachment(std::shared_ptr<ManagedVulkanPipeline> pco, VkBool32 blendEnable, VkColorComponentFlags colorWriteMask, VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor);

	void SetColorBlending(std::shared_ptr<ManagedVulkanPipeline> pco, uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState *attachments); 

	void SetDescriptorSetLayout(std::shared_ptr<ManagedVulkanPipeline> pco, std::vector<VkDescriptorSetLayout>& descriptorSetlayouts);

	void SetModelPushConstant(std::shared_ptr<ManagedVulkanPipeline> pco, VkPushConstantRange& pushConstantRange);

	void SetDynamicState(std::shared_ptr<ManagedVulkanPipeline> pco, std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0);
private:
	const VulkanDevice &device;
	VkPipelineCache pipeCache;
	std::vector<std::shared_ptr<ManagedVulkanPipeline>> pipes;
};

