#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <optional>

#include <vulkan/vulkan.h>

#include "Device.h"
#include "Shader.h"

//class PipelineCreationData {
//public:
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

class VulkanDevice;

class PipelineCreationObject {
public:

	//PipelineCreationObject(PipelineCreationData data);
	//PipelineCreationObject();

	ShaderModuleSet shaderSet;

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

class ManagedVulkanPipeline {
public:
	PipelineCreationObject pco;
	VkPipelineLayout layout;
	std::unique_ptr<std::vector<VkPipeline>> pipelines;

	void BindPipelineAtIndex(VkCommandBuffer cmdBuf, int index);

	void BindPipelineOptionalWireframe(VkCommandBuffer cmdBuf, bool wireframe);
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

	void SetShaderModuleSet(std::shared_ptr<ManagedVulkanPipeline> pco, ShaderModuleSet set);

	void SetVertexInput(std::shared_ptr<ManagedVulkanPipeline> pco,
		std::vector<VkVertexInputBindingDescription> bindingDescription,
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions);

	void SetInputAssembly(std::shared_ptr<ManagedVulkanPipeline> pco, VkPrimitiveTopology topology,
		VkPipelineInputAssemblyStateCreateFlags flag, VkBool32 primitiveRestart);

	void SetViewport(std::shared_ptr<ManagedVulkanPipeline> pco, float width, float height, float minDepth, float maxDepth, float x, float y);
	void SetScissor(std::shared_ptr<ManagedVulkanPipeline> pco, uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY);

	void SetViewportState(std::shared_ptr<ManagedVulkanPipeline> pco,
		uint32_t viewportCount, uint32_t scissorCount,
		VkPipelineViewportStateCreateFlags flags);

	void SetRasterizer(std::shared_ptr<ManagedVulkanPipeline> pco,
		VkPolygonMode polygonMode, VkCullModeFlagBits cullModeFlagBits,
		VkFrontFace frontFace, VkBool32 depthClampEnable,
		VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable);

	void SetMultisampling(std::shared_ptr<ManagedVulkanPipeline> pco, VkSampleCountFlagBits sampleCountFlags);

	void SetDepthStencil(std::shared_ptr<ManagedVulkanPipeline> pco, VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable);

	void SetColorBlendingAttachment(std::shared_ptr<ManagedVulkanPipeline> pco, VkBool32 blendEnable, VkColorComponentFlags colorWriteMask,
		VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor,
		VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor);

	void SetColorBlending(std::shared_ptr<ManagedVulkanPipeline> pco,
		uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState *attachments);

	void SetDescriptorSetLayout(std::shared_ptr<ManagedVulkanPipeline> pco, std::vector<VkDescriptorSetLayout>& descriptorSetlayouts);

	void SetModelPushConstant(std::shared_ptr<ManagedVulkanPipeline> pco, VkPushConstantRange& pushConstantRange);

	void SetDynamicState(std::shared_ptr<ManagedVulkanPipeline> pco,
		std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0);

private:
	const VulkanDevice &device;
	VkPipelineCache pipeCache;
	std::vector<std::shared_ptr<ManagedVulkanPipeline>> pipes;
};

