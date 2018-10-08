#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Shader.h"

class VulkanRenderer;
class VulkanModel;

/*
Shader,
Model,
Descriptor Set layout
Render State (rasterizer, depth, blend, etc)

*/

// class PipelineCreationData {
// public:
//
//	void WriteToFile(std::string filename);
//	void ReadFromFile(std::string filename);
//
//	//Data
//	struct ShaderString {
//
//		std::string vertShader;
//		std::string fragShader;
//		std::optional<std::string> geomShader;
//		std::optional<std::string> tessControlShader;
//		std::optional<std::string> tessEvalShader;
//
//	} shaders;
//
//	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
//	bool multiViewport;
//	std::vector<VkViewport> viewport;
//	std::vector<VkRect2D> scissor;
//	VkPipelineRasterizationStateCreateInfo rasterizer;
//	VkPipelineMultisampleStateCreateInfo multisampling;
//	VkPipelineDepthStencilStateCreateInfo depthStencil;
//	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachment;
//	VkPipelineColorBlendStateCreateInfo colorBlending;
//
//};

// class PipelineCreationObject
// {
// 	public:
// 	// PipelineCreationObject(PipelineCreationData data);
// 	// PipelineCreationObject();

// 	ShaderModuleSet shaderSet;

// 	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
// 	std::unique_ptr<std::vector<VkVertexInputBindingDescription>> vertexInputBindingDescription;
// 	std::unique_ptr<std::vector<VkVertexInputAttributeDescription>> vertexInputAttributeDescriptions;

// 	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
// 	VkViewport viewport;
// 	VkRect2D scissor;
// 	VkPipelineViewportStateCreateInfo viewportState;
// 	VkPipelineRasterizationStateCreateInfo rasterizer;
// 	VkPipelineMultisampleStateCreateInfo multisampling;
// 	VkPipelineDepthStencilStateCreateInfo depthStencil;
// 	VkPipelineColorBlendAttachmentState colorBlendAttachment;
// 	VkPipelineColorBlendStateCreateInfo colorBlending;

// 	VkPipelineDynamicStateCreateInfo dynamicState;
// 	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
// 	VkGraphicsPipelineCreateInfo pipelineInfo;
// };

// struct PipelineExternalData
// {
// 	ShaderModuleSet shaderSet;
// 	VkPrimitiveTopology topology;
// 	VkBool32 primitiveRestart;
// 	VkPolygonMode polygonMode;
// 	VkCullModeFlagBits cullModeFlagBits;
// 	VkFrontFace frontFace;
// };

// enum class PipelineType
// {
// 	opaque,
// 	transparent,
// 	post_process,
// 	overlay,
// };

// class PipelineBuilder
// {
// 	PipelineBuilder (PipelineType type, PipelineExternalData externalData) : type (type)
// 	{

// 		shaderSet = externalData.shaderSet;
// 		inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo (
// 		    externalData.topology, externalData.primitiveRestart);
// 		rasterizer = initializers::pipelineRasterizationStateCreateInfo (
// 		    externalData.polygonMode, externalData.cullModeFlagBits, externalData.frontFace);
// 	}

// 	PipelineType type;

// 	ShaderModuleSet shaderSet;

// 	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
// 	std::vector<VkVertexInputBindingDescription> vertexInputBindingDescription;
// 	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

// 	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
// 	VkViewport viewport;
// 	VkRect2D scissor;
// 	VkPipelineViewportStateCreateInfo viewportState;
// 	VkPipelineRasterizationStateCreateInfo rasterizer;
// 	VkPipelineMultisampleStateCreateInfo multisampling;
// 	VkPipelineDepthStencilStateCreateInfo depthStencil;
// 	VkPipelineColorBlendAttachmentState colorBlendAttachment;
// 	VkPipelineColorBlendStateCreateInfo colorBlending;

// 	VkPipelineDynamicStateCreateInfo dynamicState;
// 	VkPipelineLayoutCreateInfo pipelineLayoutInfo;


// 	VkGraphicsPipelineCreateInfo Get ();


// 	void SetShaderModuleSet (ShaderModuleSet set);

// 	void SetVertexInput (std::vector<VkVertexInputBindingDescription> bindingDescription,
// 	    std::vector<VkVertexInputAttributeDescription> attributeDescriptions);

// 	void SetInputAssembly (VkPrimitiveTopology topology, VkBool32 primitiveRestart);

// 	// void SetViewport(float width, float height, float minDepth, float maxDepth, float x, float
// 	// y); void SetScissor(uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY);

// 	// void SetViewportState(
// 	//	uint32_t viewportCount, uint32_t scissorCount,
// 	//	VkPipelineViewportStateCreateFlags flags);

// 	void SetRasterizer (VkPolygonMode polygonMode,
// 	    VkCullModeFlagBits cullModeFlagBits,
// 	    VkFrontFace frontFace,
// 	    VkBool32 depthClampEnable,
// 	    VkBool32 rasterizerDiscardEnable,
// 	    float lineWidth,
// 	    VkBool32 depthBiasEnable);

// 	void SetMultisampling (VkSampleCountFlagBits sampleCountFlags);

// 	void SetDepthStencil (VkBool32 depthTestEnable,
// 	    VkBool32 depthWriteEnable,
// 	    VkCompareOp depthCompareOp,
// 	    VkBool32 depthBoundsTestEnable,
// 	    VkBool32 stencilTestEnable);

// 	void SetColorBlendingAttachment (VkBool32 blendEnable,
// 	    VkColorComponentFlags colorWriteMask,
// 	    VkBlendOp colorBlendOp,
// 	    VkBlendFactor srcColorBlendFactor,
// 	    VkBlendFactor dstColorBlendFactor,
// 	    VkBlendOp alphaBlendOp,
// 	    VkBlendFactor srcAlphaBlendFactor,
// 	    VkBlendFactor dstAlphaBlendFactor);

// 	void SetColorBlending (uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* attachments);

// 	void SetDescriptorSetLayout (std::vector<VkDescriptorSetLayout>& descriptorSetlayouts);

// 	void SetModelPushConstant (VkPushConstantRange& pushConstantRange);

// 	void SetDynamicState (
// 	    std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0);
// };

// class ManagedVulkanPipeline
// {
// 	public:
// 	ManagedVulkanPipeline (PipelineType type, VkPipelineLayout layout, std::vector<VkPipeline>
// pipes) 	: type (type), layout (layout), pipelines (pipes)
// 	{
// 	}

// 	PipelineType type;

// 	VkPipelineLayout layout;
// 	std::vector<VkPipeline> pipelines;

// 	void BindPipelineAtIndex (VkCommandBuffer cmdBuf, int index);

// 	// void BindPipelineOptionalWireframe(VkCommandBuffer cmdBuf, bool wireframe);
// };

// class VulkanPipelineManager
// {
// 	public:
// 	VulkanPipelineManager (VulkanRenderer& renderer);
// 	~VulkanPipelineManager ();

// 	void InitPipelineCache ();
// 	VkPipelineCache GetPipelineCache ();

// 	ManagedVulkanPipeline* CreatePipelinesHandle (PipelineBuilder builder);
// 	// void DeleteManagedPipeline(std::shared_ptr<ManagedVulkanPipeline> pipe);

// 	// void BuildPipelineLayout(std::shared_ptr<ManagedVulkanPipeline> pco); //returns the pipeline layout of the pco (must have done SetDescriptorSetLayout
// 	// void BuildPipeline(std::shared_ptr<ManagedVulkanPipeline> pco, VkRenderPass renderPass, VkPipelineCreateFlags flags);

// 	private:
// 	VulkanRenderer& renderer;
// 	VkPipelineCache pipeCache;
// 	std::vector<std::unique_ptr<ManagedVulkanPipeline>> managed_pipes;
// };

class PipelineOutline
{
	public:
	void SetShaderModuleSet (ShaderModuleSet set);
	void UseModelVertexLayout (VulkanModel* model);
	void AddVertexLayout (VkVertexInputBindingDescription bind, VkVertexInputAttributeDescription attrib);
	void AddVertexLayouts (std::vector<VkVertexInputBindingDescription> binds,
	    std::vector<VkVertexInputAttributeDescription> attribs);


	void AddDescriptorLayouts (std::vector<VkDescriptorSetLayout> layouts);
	void AddDescriptorLayout (VkDescriptorSetLayout layout);

	void SetInputAssembly (VkPrimitiveTopology topology, VkBool32 primitiveRestart);

	void AddViewport (float width, float height, float minDepth, float maxDepth, float x, float y);
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
	Pipeline (VulkanRenderer& renderer, PipelineOutline builder, VkRenderPass renderPass, int subPass = 0);
	~Pipeline ();

	void Bind (VkCommandBuffer cmdBuf);
	VkPipelineLayout GetLayout ();

	private:
	VulkanRenderer& renderer;
	VkPipeline pipeline;
	VkPipelineLayout layout;
};
