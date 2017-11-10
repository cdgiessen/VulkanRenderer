#pragma once

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  

#include "VulkanDevice.hpp"
#include <vulkan\vulkan.h>
#include "VulkanInitializers.hpp"
#include "VulkanTools.h"
#include <vector>

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
	std::vector<VkVertexInputBindingDescription> vertexInputBindingDescription;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

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
	VkPipelineLayout pipelineLayout;
	VkGraphicsPipelineCreateInfo pipelineInfo;

	VkRenderPass renderPass;
};

class VulkanPipeline
{
public:
	VulkanPipeline(VulkanDevice* device);
	~VulkanPipeline();

	PipelineCreationObject* CreatePipelineOutline(); //returns a pipeline ready to build
	VkPipelineLayout VulkanPipeline::BuildPipelineLayout(PipelineCreationObject *pco); //returns the pipeline layout of the pco (must have done SetDescriptorSetLayout
	VkPipeline BuildPipeline(PipelineCreationObject *pco, VkRenderPass renderPass, VkPipelineCreateFlags flags);

	void SetVertexShader(PipelineCreationObject *pco, VkShaderModule vert);
	void SetFragmentShader(PipelineCreationObject *pco, VkShaderModule frag);
	void SetGeometryShader(PipelineCreationObject *pco, VkShaderModule geom);
	void SetTesselationShader(PipelineCreationObject *pco, VkShaderModule tess);
	void CleanShaderResources(PipelineCreationObject *pco); //destroys the shader modules after pipeline creation is finished

	void SetVertexInput(PipelineCreationObject *pco, std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions);

	void SetInputAssembly(PipelineCreationObject *pco, VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flag, VkBool32 primitiveRestart);

	void SetViewport(PipelineCreationObject *pco, float width, float height, float minDepth, float maxDepth, float x, float y);
	void SetScissor(PipelineCreationObject *pco, uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY);

	void SetViewportState(PipelineCreationObject *pco, uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags);

	void SetRasterizer(PipelineCreationObject *pco, VkPolygonMode polygonMode, VkCullModeFlagBits cullModeFlagBits, VkFrontFace frontFace, VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable);

	void SetMultisampling(PipelineCreationObject *pco, VkSampleCountFlagBits sampleCountFlags);

	void SetDepthStencil(PipelineCreationObject *pco, VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable);

	void SetColorBlendingAttachment(PipelineCreationObject *pco, VkBool32 blendEnable, VkColorComponentFlags colorWriteMask, VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor);

	void SetColorBlending(PipelineCreationObject *pco, uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState *attachments); 

	void SetDescriptorSetLayout(PipelineCreationObject *pco, VkDescriptorSetLayout* descriptorSetlayouts, uint32_t descritorSetLayoutCount);

	void SetDynamicState(PipelineCreationObject *pco, uint32_t dynamicStateCount, VkDynamicState* pDynamicStates, VkPipelineDynamicStateCreateFlags flags);
private:
	VulkanDevice* device; 
};

