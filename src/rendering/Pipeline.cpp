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

VulkanPipeline::VulkanPipeline(const VulkanDevice &device) :device(device) {
}

VulkanPipeline::~VulkanPipeline()
{
}

void VulkanPipeline::InitPipelineCache() {
	VkPipelineCacheCreateInfo cacheCreateInfo;
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.pNext = NULL;
	cacheCreateInfo.flags = 0;
	cacheCreateInfo.initialDataSize = 0;

	if (vkCreatePipelineCache(device.device, &cacheCreateInfo, NULL, &pipeCache) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline cache!");
	}
}

VkPipelineCache VulkanPipeline::GetPipelineCache() {
	return pipeCache;
}

void VulkanPipeline::CleanUp() {
	for (auto& manPipe : pipes){
		vkDestroyPipelineLayout(device.device, manPipe->layout, nullptr);

		for (auto& pipe : *manPipe->pipelines){
			vkDestroyPipeline(device.device, pipe, nullptr);
		}

	}

	vkDestroyPipelineCache(device.device, pipeCache, nullptr);
}

std::shared_ptr<ManagedVulkanPipeline> VulkanPipeline::CreateManagedPipeline() {
	std::shared_ptr<ManagedVulkanPipeline> mvp = std::make_shared<ManagedVulkanPipeline>();
	pipes.push_back(mvp);
	mvp->pipelines = std::make_unique<std::vector<VkPipeline>>();
	return mvp;
}

void VulkanPipeline::DeleteManagedPipeline(std::shared_ptr<ManagedVulkanPipeline> pipe) {
	auto mvp = std::find(pipes.begin(), pipes.end(), pipe);
	if (mvp != pipes.end()) {
		vkDestroyPipelineLayout(device.device, (*mvp)->layout, nullptr);

		for (auto pipe = (*mvp)->pipelines->begin(); pipe != (*mvp)->pipelines->end(); pipe++) {
			vkDestroyPipeline(device.device, *pipe, nullptr);
		}

		pipes.erase(mvp);
	}
}

void VulkanPipeline::BuildPipelineLayout(std::shared_ptr<ManagedVulkanPipeline> mvp) {
	if (vkCreatePipelineLayout(device.device, &mvp->pco.pipelineLayoutInfo, nullptr, &mvp->layout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
	
}

void VulkanPipeline::BuildPipeline(std::shared_ptr<ManagedVulkanPipeline> mvp, VkRenderPass renderPass, VkPipelineCreateFlags flags)
{

	//Deals with possible geometry or tessilation shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back(mvp->pco.vertShaderStageInfo);
	shaderStages.push_back(mvp->pco.fragShaderStageInfo);
	if (mvp->pco.geomShader) {
		shaderStages.push_back(mvp->pco.geomShaderStageInfo);
	}
	if (mvp->pco.tessShader) {
		shaderStages.push_back(mvp->pco.tessShaderStageInfo);
	}

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



void VulkanPipeline::SetVertexShader(std::shared_ptr<ManagedVulkanPipeline> mvp, VkShaderModule vert)
{
	mvp->pco.vertShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vert);
	mvp->pco.vertShaderStageInfo.pName = "main";
	mvp->pco.vertShaderModule = vert;
}

void VulkanPipeline::SetFragmentShader(std::shared_ptr<ManagedVulkanPipeline> mvp, VkShaderModule frag)
{
	mvp->pco.fragShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, frag);
	mvp->pco.fragShaderStageInfo.pName = "main";
	mvp->pco.fragShaderModule = frag;
}

void VulkanPipeline::SetGeometryShader(std::shared_ptr<ManagedVulkanPipeline> mvp, VkShaderModule geom)
{
	mvp->pco.geomShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, geom);
	mvp->pco.geomShaderStageInfo.pName = "main";
	mvp->pco.geomShader = true;
	mvp->pco.geomShaderModule = geom;
}

//this shouldn't work (cause I have no clue how tess shaders work...)
void VulkanPipeline::SetTesselationShader(std::shared_ptr<ManagedVulkanPipeline> mvp, VkShaderModule tess)
{
	mvp->pco.tessShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, tess);
	mvp->pco.tessShaderStageInfo.pName = "main";
	mvp->pco.tessShader = true;
	mvp->pco.tessShaderModule = tess;
}

void VulkanPipeline::CleanShaderResources(std::shared_ptr<ManagedVulkanPipeline> mvp) {
	vkDestroyShaderModule(device.device, mvp->pco.vertShaderModule, nullptr);
	vkDestroyShaderModule(device.device, mvp->pco.fragShaderModule, nullptr);
	vkDestroyShaderModule(device.device, mvp->pco.geomShaderModule, nullptr);
	vkDestroyShaderModule(device.device, mvp->pco.tessShaderModule, nullptr);
}

void VulkanPipeline::SetVertexInput(std::shared_ptr<ManagedVulkanPipeline> mvp, 
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions)
{
	mvp->pco.vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo();
	
	mvp->pco.vertexInputBindingDescription = std::make_unique<std::vector<VkVertexInputBindingDescription>>(bindingDescription);
	mvp->pco.vertexInputAttributeDescriptions = std::make_unique<std::vector<VkVertexInputAttributeDescription>>(attributeDescriptions);

	mvp->pco.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(mvp->pco.vertexInputBindingDescription->size());
	mvp->pco.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(mvp->pco.vertexInputAttributeDescriptions->size());
	mvp->pco.vertexInputInfo.pVertexBindingDescriptions = mvp->pco.vertexInputBindingDescription->data();
	mvp->pco.vertexInputInfo.pVertexAttributeDescriptions = mvp->pco.vertexInputAttributeDescriptions->data();
 
}

void VulkanPipeline::SetInputAssembly(std::shared_ptr<ManagedVulkanPipeline> mvp, 
	VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flag, VkBool32 primitiveRestart)
{
	mvp->pco.inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(topology, flag, primitiveRestart);
}

void VulkanPipeline::SetDynamicState(std::shared_ptr<ManagedVulkanPipeline> mvp, 
	std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags) 
{
	mvp->pco.dynamicState = VkPipelineDynamicStateCreateInfo();
	mvp->pco.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	mvp->pco.dynamicState.flags = flags;
	mvp->pco.dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	mvp->pco.dynamicState.pDynamicStates = dynamicStates.data();
}

void VulkanPipeline::SetViewport(std::shared_ptr<ManagedVulkanPipeline> mvp, 
	float width, float height, float minDepth, float maxDepth, float x, float y)
{
	mvp->pco.viewport = initializers::viewport(width, height, minDepth, maxDepth);
	mvp->pco.viewport.x = 0.0f;
	mvp->pco.viewport.y = 0.0f;
}

void VulkanPipeline::SetScissor(std::shared_ptr<ManagedVulkanPipeline> mvp, uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY)
{
	mvp->pco.scissor = initializers::rect2D(width, height, offsetX, offsetY);
}

//Currently only supports one viewport or scissor
void VulkanPipeline::SetViewportState(std::shared_ptr<ManagedVulkanPipeline> mvp, uint32_t viewportCount, uint32_t scissorCount, 
	VkPipelineViewportStateCreateFlags flags)
{ 
	mvp->pco.viewportState = initializers::pipelineViewportStateCreateInfo(1, 1);
	mvp->pco.viewportState.pViewports = &mvp->pco.viewport;
	mvp->pco.viewportState.pScissors = &mvp->pco.scissor;
}

void VulkanPipeline::SetRasterizer(std::shared_ptr<ManagedVulkanPipeline> mvp, VkPolygonMode polygonMode, 
	VkCullModeFlagBits cullModeFlagBits, VkFrontFace frontFace, 
	VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, float lineWidth, VkBool32 depthBiasEnable)
{
	mvp->pco.rasterizer = initializers::pipelineRasterizationStateCreateInfo( polygonMode, cullModeFlagBits, frontFace);
	mvp->pco.rasterizer.depthClampEnable = depthClampEnable;
	mvp->pco.rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
	mvp->pco.rasterizer.lineWidth = lineWidth;
	mvp->pco.rasterizer.depthBiasEnable = depthBiasEnable;
}

//No Multisampling support right now
void VulkanPipeline::SetMultisampling(std::shared_ptr<ManagedVulkanPipeline> mvp, VkSampleCountFlagBits sampleCountFlags)
{
	mvp->pco.multisampling = initializers::pipelineMultisampleStateCreateInfo(sampleCountFlags);
	mvp->pco.multisampling.sampleShadingEnable = VK_FALSE;
}

void VulkanPipeline::SetDepthStencil(std::shared_ptr<ManagedVulkanPipeline> mvp, 
	VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, 
	VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable)
{
	mvp->pco.depthStencil = initializers::pipelineDepthStencilStateCreateInfo(depthTestEnable, depthWriteEnable, depthCompareOp);
	mvp->pco.depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
	mvp->pco.depthStencil.stencilTestEnable = stencilTestEnable;
}

void VulkanPipeline::SetColorBlendingAttachment(std::shared_ptr<ManagedVulkanPipeline> mvp, 
		VkBool32 blendEnable, VkColorComponentFlags colorWriteMask, 
	VkBlendOp colorBlendOp, VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, 
	VkBlendOp alphaBlendOp, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor)
{
	mvp->pco.colorBlendAttachment = initializers::pipelineColorBlendAttachmentState(colorWriteMask, blendEnable);
	mvp->pco.colorBlendAttachment.colorBlendOp = colorBlendOp;
	mvp->pco.colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	mvp->pco.colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	mvp->pco.colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	mvp->pco.colorBlendAttachment.srcAlphaBlendFactor= srcAlphaBlendFactor;
	mvp->pco.colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
}

//Can't handle more than one attachment currently
void VulkanPipeline::SetColorBlending(std::shared_ptr<ManagedVulkanPipeline> mvp, uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState * attachments)
{
	mvp->pco.colorBlending = initializers::pipelineColorBlendStateCreateInfo(1, &mvp->pco.colorBlendAttachment);
	mvp->pco.colorBlending.logicOpEnable = VK_FALSE;
	mvp->pco.colorBlending.logicOp = VK_LOGIC_OP_COPY;
	mvp->pco.colorBlending.blendConstants[0] = 0.0f;
	mvp->pco.colorBlending.blendConstants[1] = 0.0f;
	mvp->pco.colorBlending.blendConstants[2] = 0.0f;
	mvp->pco.colorBlending.blendConstants[3] = 0.0f;
}

void VulkanPipeline::SetDescriptorSetLayout(std::shared_ptr<ManagedVulkanPipeline> mvp, std::vector<VkDescriptorSetLayout>& descriptorSetlayouts)
{
	mvp->pco.pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(descriptorSetlayouts.data(), (uint32_t)descriptorSetlayouts.size());
}

void VulkanPipeline::SetModelPushConstant(std::shared_ptr<ManagedVulkanPipeline> mvp, VkPushConstantRange& pushConstantRange) {
	mvp->pco.pipelineLayoutInfo.pushConstantRangeCount = 1;
	mvp->pco.pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
}