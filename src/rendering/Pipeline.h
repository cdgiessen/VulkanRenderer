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

class PipelineBuilder {
	PipelineCreationObject pco;

	void SetShaderModuleSet(ShaderModuleSet set);

	void SetVertexInput(
		std::vector<VkVertexInputBindingDescription> bindingDescription,
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions);

	void SetInputAssembly(VkPrimitiveTopology topology,
		VkPipelineInputAssemblyStateCreateFlags flag, VkBool32 primitiveRestart);

	void SetViewport(float width, float height, float minDepth, float maxDepth, float x, float y);
	void SetScissor(uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY);

	void SetViewportState(
		uint32_t viewportCount, uint32_t scissorCount,
		VkPipelineViewportStateCreateFlags flags);

	void SetRasterizer(
		VkPolygonMode polygonMode, VkCullModeFlagBits cullModeFlagBits,
		VkFrontFace frontFace, VkBool32 depthClampEnable,
		VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable);

	void SetMultisampling(VkSampleCountFlagBits sampleCountFlags);

	void SetDepthStencil(VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable);

	void SetColorBlendingAttachment(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask,
		VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor,
		VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor);

	void SetColorBlending(
		uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState *attachments);

	void SetDescriptorSetLayout(std::vector<VkDescriptorSetLayout>& descriptorSetlayouts);

	void SetModelPushConstant(VkPushConstantRange& pushConstantRange);

	void SetDynamicState(
		std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0);

};

class VulkanPipelineManager
{
public:
	VulkanPipelineManager(VulkanDevice &device);
	~VulkanPipelineManager();

	void InitPipelineCache();
	VkPipelineCache GetPipelineCache();

	std::shared_ptr<ManagedVulkanPipeline> CreateManagedPipeline();
	void DeleteManagedPipeline(std::shared_ptr<ManagedVulkanPipeline> pipe);

	void BuildPipelineLayout(std::shared_ptr<ManagedVulkanPipeline> pco); //returns the pipeline layout of the pco (must have done SetDescriptorSetLayout
	void BuildPipeline(std::shared_ptr<ManagedVulkanPipeline> pco, VkRenderPass renderPass, VkPipelineCreateFlags flags);

private:
	VulkanDevice &device;
	VkPipelineCache pipeCache;
	std::vector<std::shared_ptr<ManagedVulkanPipeline>> pipes;
};

