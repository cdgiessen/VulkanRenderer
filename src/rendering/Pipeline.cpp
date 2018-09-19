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
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.at(0));
}

//void ManagedVulkanPipeline::BindPipelineOptionalWireframe(VkCommandBuffer cmdBuf, bool wireframe) {
//	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines->at(1) : pipelines->at(0));
//}

/////////////////////////// 
// -- PipelineBuilder -- //
///////////////////////////

void PipelineBuilder::SetShaderModuleSet(ShaderModuleSet set) {
	shaderSet = set;
}


void PipelineBuilder::SetVertexInput(
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions)
{
	vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo();

	vertexInputBindingDescription = std::vector<VkVertexInputBindingDescription>(bindingDescription);
	vertexInputAttributeDescriptions = std::vector<VkVertexInputAttributeDescription>(attributeDescriptions);

	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindingDescription.size());
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescription.data();
	vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

}

void PipelineBuilder::SetInputAssembly(
	VkPrimitiveTopology topology, VkBool32 primitiveRestart)
{
	inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(topology, primitiveRestart);
}

void PipelineBuilder::SetDynamicState(
	std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags)
{
	dynamicState = VkPipelineDynamicStateCreateInfo();
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.flags = flags;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();
}

//void PipelineBuilder::SetViewport(
//	float width, float height, float minDepth, float maxDepth, float x, float y)
//{
//	pco.viewport = initializers::viewport(width, height, minDepth, maxDepth);
//	pco.viewport.x = 0.0f;
//	pco.viewport.y = 0.0f;
//}
//
//void PipelineBuilder::SetScissor(uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY)
//{
//	pco.scissor = initializers::rect2D(width, height, offsetX, offsetY);
//}

//Currently only supports one viewport or scissor
//void PipelineBuilder::SetViewportState(uint32_t viewportCount, uint32_t scissorCount,
//	VkPipelineViewportStateCreateFlags flags)
//{
//	pco.viewportState = initializers::pipelineViewportStateCreateInfo(1, 1);
//	pco.viewportState.pViewports = &pco.viewport;
//	pco.viewportState.pScissors = &pco.scissor;
//}

VkGraphicsPipelineCreateInfo PipelineBuilder::Get() {
	auto shaderStages = shaderSet.ShaderStageCreateInfos();

	

	pipelineInfo = initializers::pipelineCreateInfo(mvp->layout, renderPass, flags);
	
	
	pipelineInfo.stageCount = (uint32_t)shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;


	pipelineInfo.subpass = 0; //which subpass in the renderpass this pipeline gets used
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

}

void PipelineBuilder::SetRasterizer(VkPolygonMode polygonMode,
	VkCullModeFlagBits cullModeFlagBits, VkFrontFace frontFace,
	VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable)
{
	rasterizer = initializers::pipelineRasterizationStateCreateInfo(polygonMode, cullModeFlagBits, frontFace);
	rasterizer.depthClampEnable = depthClampEnable;
	rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
	rasterizer.lineWidth = lineWidth;
	rasterizer.depthBiasEnable = depthBiasEnable;
}

//No Multisampling support right now
void PipelineBuilder::SetMultisampling(VkSampleCountFlagBits sampleCountFlags)
{
	multisampling = initializers::pipelineMultisampleStateCreateInfo(sampleCountFlags);
	multisampling.sampleShadingEnable = VK_FALSE;
}

void PipelineBuilder::SetDepthStencil(
	VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp,
	VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable)
{
	depthStencil = initializers::pipelineDepthStencilStateCreateInfo(depthTestEnable, depthWriteEnable, depthCompareOp);
	depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
	depthStencil.stencilTestEnable = stencilTestEnable;
}

void PipelineBuilder::SetColorBlendingAttachment(
	VkBool32 blendEnable, VkColorComponentFlags colorWriteMask,
	VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor,
	VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor)
{
	colorBlendAttachment = initializers::pipelineColorBlendAttachmentState(colorWriteMask, blendEnable);
	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
}

//Can't handle more than one attachment currently
void PipelineBuilder::SetColorBlending(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState * attachments)
{
	colorBlending = initializers::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;
}

void PipelineBuilder::SetDescriptorSetLayout(std::vector<VkDescriptorSetLayout>& descriptorSetlayouts)
{
	pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(descriptorSetlayouts.data(), (uint32_t)descriptorSetlayouts.size());
}

void PipelineBuilder::SetModelPushConstant(VkPushConstantRange& pushConstantRange) {
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
}

/////////////////////////////////
// -- VulkanPipelineManager -- //
/////////////////////////////////

VulkanPipelineManager::VulkanPipelineManager(VulkanRenderer &renderer) :
	renderer(renderer)
{
	InitPipelineCache();
}

VulkanPipelineManager::~VulkanPipelineManager()
{
	vkDestroyPipelineCache(renderer.device.device, pipeCache, nullptr);
}

void VulkanPipelineManager::InitPipelineCache() {
	VkPipelineCacheCreateInfo cacheCreateInfo;
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.pNext = NULL;
	cacheCreateInfo.flags = 0;
	cacheCreateInfo.initialDataSize = 0;

	if (vkCreatePipelineCache(renderer.device.device, &cacheCreateInfo, NULL, &pipeCache) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline cache!");
	}
}

VkPipelineCache VulkanPipelineManager::GetPipelineCache() {
	return pipeCache;
}

std::shared_ptr<ManagedVulkanPipeline> VulkanPipelineManager::CreatePipelinesHandle(PipelineBuilder builder) {
	/*std::shared_ptr<ManagedVulkanPipeline> mvp = std::make_shared<ManagedVulkanPipeline>();
	pipes.push_back(mvp);
	mvp->pipelines = std::make_unique<std::vector<VkPipeline>>();
	return mvp;*/
}

void VulkanPipelineManager::DeleteManagedPipeline(std::shared_ptr<ManagedVulkanPipeline> pipe) {
	auto mvp = std::find(pipes.begin(), pipes.end(), pipe);
	if (mvp != pipes.end()) {
		vkDestroyPipelineLayout(renderer.device.device, (*mvp)->layout, nullptr);

		for (auto pipe = (*mvp)->pipelines->begin(); pipe != (*mvp)->pipelines->end(); pipe++) {
			vkDestroyPipeline(renderer.device.device, *pipe, nullptr);
		}

		pipes.erase(mvp);
	}
}

void VulkanPipelineManager::BuildPipelineLayout(std::shared_ptr<ManagedVulkanPipeline> mvp) {
	if (vkCreatePipelineLayout(renderer.device.device, &mvp->pipelineLayoutInfo, nullptr, &mvp->layout) != VK_SUCCESS) {
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
	if (vkCreateGraphicsPipelines(renderer.device.device, pipeCache, 1, &mvp->pco.pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	mvp->pipelines->push_back(pipeline);
}
