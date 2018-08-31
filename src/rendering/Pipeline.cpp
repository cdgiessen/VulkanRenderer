#include "Pipeline.h"



//#include <json.hpp>

#include "Initializers.h"
#include "Device.h"
#include "RenderTools.h"
#include "RenderStructs.h"

#include "../resources/Mesh.h"

//void PipelineCreationData::WriteToFile(std::string filename) {
//
//}
//
//void PipelineCreationData::ReadFromFile(std::string filename) {
//
//	std::ifstream inFile(filename);
//	nlohmann::json j;
//
//	if (inFile.peek() == std::ifstream::traits_type::eof()) {
//		Log::Error << "Opened graph is empty! Did something go wrong?\n";
//		return;
//	}
//
//}


//PipelineCreationObject::PipelineCreationObject(PipelineCreationData data) {
//
//}
//
//PipelineCreationObject::PipelineCreationObject() {
//
//}

void ManagedVulkanPipeline::BindPipelineAtIndex(VkCommandBuffer cmdBuf, int index) {
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines->at(0));
}

void ManagedVulkanPipeline::BindPipelineOptionalWireframe(VkCommandBuffer cmdBuf, bool wireframe) {
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines->at(1) : pipelines->at(0));
}

/////////////////////////// 
// -- PipelineBuilder -- //
///////////////////////////

void PipelineBuilder::SetShaderModuleSet(ShaderModuleSet set) {
	pco.shaderSet = set;
}


void PipelineBuilder::SetVertexInput(
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions)
{
	pco.vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo();

	pco.vertexInputBindingDescription = std::make_unique<std::vector<VkVertexInputBindingDescription>>(bindingDescription);
	pco.vertexInputAttributeDescriptions = std::make_unique<std::vector<VkVertexInputAttributeDescription>>(attributeDescriptions);

	pco.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(pco.vertexInputBindingDescription->size());
	pco.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(pco.vertexInputAttributeDescriptions->size());
	pco.vertexInputInfo.pVertexBindingDescriptions = pco.vertexInputBindingDescription->data();
	pco.vertexInputInfo.pVertexAttributeDescriptions = pco.vertexInputAttributeDescriptions->data();

}

void PipelineBuilder::SetInputAssembly(
	VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flag, VkBool32 primitiveRestart)
{
	pco.inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(topology, flag, primitiveRestart);
}

void PipelineBuilder::SetDynamicState(
	std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags)
{
	pco.dynamicState = VkPipelineDynamicStateCreateInfo();
	pco.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pco.dynamicState.flags = flags;
	pco.dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	pco.dynamicState.pDynamicStates = dynamicStates.data();
}

void PipelineBuilder::SetViewport(
	float width, float height, float minDepth, float maxDepth, float x, float y)
{
	pco.viewport = initializers::viewport(width, height, minDepth, maxDepth);
	pco.viewport.x = 0.0f;
	pco.viewport.y = 0.0f;
}

void PipelineBuilder::SetScissor(uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY)
{
	pco.scissor = initializers::rect2D(width, height, offsetX, offsetY);
}

//Currently only supports one viewport or scissor
void PipelineBuilder::SetViewportState(uint32_t viewportCount, uint32_t scissorCount,
	VkPipelineViewportStateCreateFlags flags)
{
	pco.viewportState = initializers::pipelineViewportStateCreateInfo(1, 1);
	pco.viewportState.pViewports = &pco.viewport;
	pco.viewportState.pScissors = &pco.scissor;
}

void PipelineBuilder::SetRasterizer(VkPolygonMode polygonMode,
	VkCullModeFlagBits cullModeFlagBits, VkFrontFace frontFace,
	VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable)
{
	pco.rasterizer = initializers::pipelineRasterizationStateCreateInfo(polygonMode, cullModeFlagBits, frontFace);
	pco.rasterizer.depthClampEnable = depthClampEnable;
	pco.rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
	pco.rasterizer.lineWidth = lineWidth;
	pco.rasterizer.depthBiasEnable = depthBiasEnable;
}

//No Multisampling support right now
void PipelineBuilder::SetMultisampling(VkSampleCountFlagBits sampleCountFlags)
{
	pco.multisampling = initializers::pipelineMultisampleStateCreateInfo(sampleCountFlags);
	pco.multisampling.sampleShadingEnable = VK_FALSE;
}

void PipelineBuilder::SetDepthStencil(
	VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp,
	VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable)
{
	pco.depthStencil = initializers::pipelineDepthStencilStateCreateInfo(depthTestEnable, depthWriteEnable, depthCompareOp);
	pco.depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
	pco.depthStencil.stencilTestEnable = stencilTestEnable;
}

void PipelineBuilder::SetColorBlendingAttachment(
	VkBool32 blendEnable, VkColorComponentFlags colorWriteMask,
	VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor,
	VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor)
{
	pco.colorBlendAttachment = initializers::pipelineColorBlendAttachmentState(colorWriteMask, blendEnable);
	pco.colorBlendAttachment.colorBlendOp = colorBlendOp;
	pco.colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	pco.colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	pco.colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	pco.colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	pco.colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
}

//Can't handle more than one attachment currently
void PipelineBuilder::SetColorBlending(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState * attachments)
{
	pco.colorBlending = initializers::pipelineColorBlendStateCreateInfo(1, &pco.colorBlendAttachment);
	pco.colorBlending.logicOpEnable = VK_FALSE;
	pco.colorBlending.logicOp = VK_LOGIC_OP_COPY;
	pco.colorBlending.blendConstants[0] = 0.0f;
	pco.colorBlending.blendConstants[1] = 0.0f;
	pco.colorBlending.blendConstants[2] = 0.0f;
	pco.colorBlending.blendConstants[3] = 0.0f;
}

void PipelineBuilder::SetDescriptorSetLayout(std::vector<VkDescriptorSetLayout>& descriptorSetlayouts)
{
	pco.pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(descriptorSetlayouts.data(), (uint32_t)descriptorSetlayouts.size());
}

void PipelineBuilder::SetModelPushConstant(VkPushConstantRange& pushConstantRange) {
	pco.pipelineLayoutInfo.pushConstantRangeCount = 1;
	pco.pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
}

/////////////////////////////////
// -- VulkanPipelineManager -- //
/////////////////////////////////

VulkanPipelineManager::VulkanPipelineManager(VulkanDevice &device) :
	device(device)
{
	InitPipelineCache();
}

VulkanPipelineManager::~VulkanPipelineManager()
{
	vkDestroyPipelineCache(device.device, pipeCache, nullptr);
}

void VulkanPipelineManager::InitPipelineCache() {
	VkPipelineCacheCreateInfo cacheCreateInfo;
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.pNext = NULL;
	cacheCreateInfo.flags = 0;
	cacheCreateInfo.initialDataSize = 0;

	if (vkCreatePipelineCache(device.device, &cacheCreateInfo, NULL, &pipeCache) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline cache!");
	}
}

VkPipelineCache VulkanPipelineManager::GetPipelineCache() {
	return pipeCache;
}

std::shared_ptr<ManagedVulkanPipeline> VulkanPipelineManager::CreateManagedPipeline() {
	std::shared_ptr<ManagedVulkanPipeline> mvp = std::make_shared<ManagedVulkanPipeline>();
	pipes.push_back(mvp);
	mvp->pipelines = std::make_unique<std::vector<VkPipeline>>();
	return mvp;
}

void VulkanPipelineManager::DeleteManagedPipeline(std::shared_ptr<ManagedVulkanPipeline> pipe) {
	auto mvp = std::find(pipes.begin(), pipes.end(), pipe);
	if (mvp != pipes.end()) {
		vkDestroyPipelineLayout(device.device, (*mvp)->layout, nullptr);

		for (auto pipe = (*mvp)->pipelines->begin(); pipe != (*mvp)->pipelines->end(); pipe++) {
			vkDestroyPipeline(device.device, *pipe, nullptr);
		}

		pipes.erase(mvp);
	}
}

void VulkanPipelineManager::BuildPipelineLayout(std::shared_ptr<ManagedVulkanPipeline> mvp) {
	if (vkCreatePipelineLayout(device.device, &mvp->pco.pipelineLayoutInfo, nullptr, &mvp->layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

}

void VulkanPipelineManager::BuildPipeline(std::shared_ptr<ManagedVulkanPipeline> mvp, VkRenderPass renderPass, VkPipelineCreateFlags flags)
{

	//Deals with possible geometry or tessilation shaders
	//std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	//shaderStages.push_back(mvp->pco.vertShaderStageInfo);
	//shaderStages.push_back(mvp->pco.fragShaderStageInfo);
	//if (mvp->pco.geomShader) {
	//	shaderStages.push_back(mvp->pco.geomShaderStageInfo);
	//}
	//if (mvp->pco.tessShader) {
	//	shaderStages.push_back(mvp->pco.tessShaderStageInfo);
	//}

	auto shaderStages = mvp->pco.shaderSet.ShaderStageCreateInfos();

	mvp->pco.pipelineInfo = initializers::pipelineCreateInfo(mvp->layout, renderPass, flags);
	mvp->pco.pipelineInfo.stageCount = (uint32_t)shaderStages.size();
	mvp->pco.pipelineInfo.pStages = shaderStages.data();

	mvp->pco.pipelineInfo.pVertexInputState = &mvp->pco.vertexInputInfo;
	mvp->pco.pipelineInfo.pInputAssemblyState = &mvp->pco.inputAssembly;
	mvp->pco.pipelineInfo.pViewportState = &mvp->pco.viewportState;
	mvp->pco.pipelineInfo.pRasterizationState = &mvp->pco.rasterizer;
	mvp->pco.pipelineInfo.pMultisampleState = &mvp->pco.multisampling;
	mvp->pco.pipelineInfo.pDepthStencilState = &mvp->pco.depthStencil;
	mvp->pco.pipelineInfo.pColorBlendState = &mvp->pco.colorBlending;
	mvp->pco.pipelineInfo.pDynamicState = &mvp->pco.dynamicState;


	mvp->pco.pipelineInfo.subpass = 0; //which subpass in the renderpass this pipeline gets used
	mvp->pco.pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(device.device, pipeCache, 1, &mvp->pco.pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	mvp->pipelines->push_back(pipeline);
}
