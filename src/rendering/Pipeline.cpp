#include "Pipeline.h"



//#include <json.hpp>

#include "Device.h"
#include "Initializers.h"
#include "RenderStructs.h"
#include "RenderTools.h"

#include "../resources/Mesh.h"

// void PipelineCreationData::WriteToFile(std::string filename) {
//
//}
//
// void PipelineCreationData::ReadFromFile(std::string filename) {
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


// PipelineCreationObject::PipelineCreationObject(PipelineCreationData data) {
//
//}
//
// PipelineCreationObject::PipelineCreationObject() {
//
//}

void ManagedVulkanPipeline::BindPipelineAtIndex (VkCommandBuffer cmdBuf, int index)
{
	vkCmdBindPipeline (cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.at (0));
}

// void ManagedVulkanPipeline::BindPipelineOptionalWireframe(VkCommandBuffer cmdBuf, bool wireframe) {
//	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines->at(1) : pipelines->at(0));
//}

///////////////////////////
// -- PipelineBuilder -- //
///////////////////////////

void PipelineBuilder::SetShaderModuleSet (ShaderModuleSet set) { shaderSet = set; }


void PipelineBuilder::SetVertexInput (std::vector<VkVertexInputBindingDescription> bindingDescription,
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions)
{
	vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo ();

	vertexInputBindingDescription = std::vector<VkVertexInputBindingDescription> (bindingDescription);
	vertexInputAttributeDescriptions = std::vector<VkVertexInputAttributeDescription> (attributeDescriptions);

	vertexInputInfo.vertexBindingDescriptionCount =
	    static_cast<uint32_t> (vertexInputBindingDescription.size ());
	vertexInputInfo.vertexAttributeDescriptionCount =
	    static_cast<uint32_t> (vertexInputAttributeDescriptions.size ());
	vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescription.data ();
	vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data ();
}

void PipelineBuilder::SetInputAssembly (VkPrimitiveTopology topology, VkBool32 primitiveRestart)
{
	inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo (topology, primitiveRestart);
}

void PipelineBuilder::SetDynamicState (
    std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags)
{
	dynamicState = VkPipelineDynamicStateCreateInfo ();
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.flags = flags;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size ();
	dynamicState.pDynamicStates = dynamicStates.data ();
}

// void PipelineBuilder::SetViewport(
//	float width, float height, float minDepth, float maxDepth, float x, float y)
//{
//	pco.viewport = initializers::viewport(width, height, minDepth, maxDepth);
//	pco.viewport.x = 0.0f;
//	pco.viewport.y = 0.0f;
//}
//
// void PipelineBuilder::SetScissor(uint32_t width, uint32_t height, uint32_t offsetX, uint32_t offsetY)
//{
//	pco.scissor = initializers::rect2D(width, height, offsetX, offsetY);
//}

// Currently only supports one viewport or scissor
// void PipelineBuilder::SetViewportState(uint32_t viewportCount, uint32_t scissorCount,
//	VkPipelineViewportStateCreateFlags flags)
//{
//	pco.viewportState = initializers::pipelineViewportStateCreateInfo(1, 1);
//	pco.viewportState.pViewports = &pco.viewport;
//	pco.viewportState.pScissors = &pco.scissor;
//}

VkGraphicsPipelineCreateInfo PipelineBuilder::Get ()
{
	auto shaderStages = shaderSet.ShaderStageCreateInfos ();



	pipelineInfo = initializers::pipelineCreateInfo (mvp->layout, renderPass, flags);


	pipelineInfo.stageCount = (uint32_t)shaderStages.size ();
	pipelineInfo.pStages = shaderStages.data ();

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;


	pipelineInfo.subpass = 0; // which subpass in the renderpass this pipeline gets used
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
}

void PipelineBuilder::SetRasterizer (VkPolygonMode polygonMode,
    VkCullModeFlagBits cullModeFlagBits,
    VkFrontFace frontFace,
    VkBool32 depthClampEnable,
    VkBool32 rasterizerDiscardEnable,
    float lineWidth,
    VkBool32 depthBiasEnable)
{
	rasterizer = initializers::pipelineRasterizationStateCreateInfo (polygonMode, cullModeFlagBits, frontFace);
	rasterizer.depthClampEnable = depthClampEnable;
	rasterizer.rasterizerDiscardEnable = rasterizerDiscardEnable;
	rasterizer.lineWidth = lineWidth;
	rasterizer.depthBiasEnable = depthBiasEnable;
}

// No Multisampling support right now
void PipelineBuilder::SetMultisampling (VkSampleCountFlagBits sampleCountFlags)
{
	multisampling = initializers::pipelineMultisampleStateCreateInfo (sampleCountFlags);
	multisampling.sampleShadingEnable = VK_FALSE;
}

void PipelineBuilder::SetDepthStencil (VkBool32 depthTestEnable,
    VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp,
    VkBool32 depthBoundsTestEnable,
    VkBool32 stencilTestEnable)
{
	depthStencil = initializers::pipelineDepthStencilStateCreateInfo (
	    depthTestEnable, depthWriteEnable, depthCompareOp);
	depthStencil.depthBoundsTestEnable = depthBoundsTestEnable;
	depthStencil.stencilTestEnable = stencilTestEnable;
}

void PipelineBuilder::SetColorBlendingAttachment (VkBool32 blendEnable,
    VkColorComponentFlags colorWriteMask,
    VkBlendOp colorBlendOp,
    VkBlendFactor srcColorBlendFactor,
    VkBlendFactor dstColorBlendFactor,
    VkBlendOp alphaBlendOp,
    VkBlendFactor srcAlphaBlendFactor,
    VkBlendFactor dstAlphaBlendFactor)
{
	colorBlendAttachment = initializers::pipelineColorBlendAttachmentState (colorWriteMask, blendEnable);
	colorBlendAttachment.colorBlendOp = colorBlendOp;
	colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
	colorBlendAttachment.alphaBlendOp = alphaBlendOp;
	colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
	colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
}

// Can't handle more than one attachment currently
void PipelineBuilder::SetColorBlending (
    uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* attachments)
{
	colorBlending = initializers::pipelineColorBlendStateCreateInfo (1, &colorBlendAttachment);
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;
}

void PipelineBuilder::SetDescriptorSetLayout (std::vector<VkDescriptorSetLayout>& descriptorSetlayouts)
{
	pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo (
	    descriptorSetlayouts.data (), (uint32_t)descriptorSetlayouts.size ());
}

void PipelineBuilder::SetModelPushConstant (VkPushConstantRange& pushConstantRange)
{
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
}

/////////////////////////////////
// -- VulkanPipelineManager -- //
/////////////////////////////////

VulkanPipelineManager::VulkanPipelineManager (VulkanRenderer& renderer) : renderer (renderer)
{
	InitPipelineCache ();
}

VulkanPipelineManager::~VulkanPipelineManager ()
{
	vkDestroyPipelineCache (renderer.device.device, pipeCache, nullptr);
}

void VulkanPipelineManager::InitPipelineCache ()
{
	VkPipelineCacheCreateInfo cacheCreateInfo;
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.pNext = NULL;
	cacheCreateInfo.flags = 0;
	cacheCreateInfo.initialDataSize = 0;

	if (vkCreatePipelineCache (renderer.device.device, &cacheCreateInfo, NULL, &pipeCache) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create pipeline cache!");
	}
}

VkPipelineCache VulkanPipelineManager::GetPipelineCache () { return pipeCache; }

std::shared_ptr<ManagedVulkanPipeline> VulkanPipelineManager::CreatePipelinesHandle (PipelineBuilder builder)
{
	/*std::shared_ptr<ManagedVulkanPipeline> mvp = std::make_shared<ManagedVulkanPipeline>();
	pipes.push_back(mvp);
	mvp->pipelines = std::make_unique<std::vector<VkPipeline>>();
	return mvp;*/
}

void VulkanPipelineManager::DeleteManagedPipeline (std::shared_ptr<ManagedVulkanPipeline> pipe)
{
	auto mvp = std::find (pipes.begin (), pipes.end (), pipe);
	if (mvp != pipes.end ())
	{
		vkDestroyPipelineLayout (renderer.device.device, (*mvp)->layout, nullptr);

		for (auto pipe = (*mvp)->pipelines->begin (); pipe != (*mvp)->pipelines->end (); pipe++)
		{
			vkDestroyPipeline (renderer.device.device, *pipe, nullptr);
		}

		pipes.erase (mvp);
	}
}

void VulkanPipelineManager::BuildPipelineLayout (std::shared_ptr<ManagedVulkanPipeline> mvp)
{
	if (vkCreatePipelineLayout (renderer.device.device, &mvp->pipelineLayoutInfo, nullptr, &mvp->layout) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create pipeline layout!");
	}
}

void VulkanPipelineManager::BuildPipeline (
    std::shared_ptr<ManagedVulkanPipeline> mvp, VkRenderPass renderPass, VkPipelineCreateFlags flags)
{

	// Deals with possible geometry or tessilation shaders
	// std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	// shaderStages.push_back(mvp->pco.vertShaderStageInfo);
	// shaderStages.push_back(mvp->pco.fragShaderStageInfo);
	// if (mvp->pco.geomShader) {
	//	shaderStages.push_back(mvp->pco.geomShaderStageInfo);
	//}
	// if (mvp->pco.tessShader) {
	//	shaderStages.push_back(mvp->pco.tessShaderStageInfo);
	//}

	auto shaderStages = mvp->pco.shaderSet.ShaderStageCreateInfos ();

	mvp->pco.pipelineInfo = initializers::pipelineCreateInfo (mvp->layout, renderPass, flags);
	mvp->pco.pipelineInfo.stageCount = (uint32_t)shaderStages.size ();
	mvp->pco.pipelineInfo.pStages = shaderStages.data ();

	mvp->pco.pipelineInfo.pVertexInputState = &mvp->pco.vertexInputInfo;
	mvp->pco.pipelineInfo.pInputAssemblyState = &mvp->pco.inputAssembly;
	mvp->pco.pipelineInfo.pViewportState = &mvp->pco.viewportState;
	mvp->pco.pipelineInfo.pRasterizationState = &mvp->pco.rasterizer;
	mvp->pco.pipelineInfo.pMultisampleState = &mvp->pco.multisampling;
	mvp->pco.pipelineInfo.pDepthStencilState = &mvp->pco.depthStencil;
	mvp->pco.pipelineInfo.pColorBlendState = &mvp->pco.colorBlending;
	mvp->pco.pipelineInfo.pDynamicState = &mvp->pco.dynamicState;


	mvp->pco.pipelineInfo.subpass = 0; // which subpass in the renderpass this pipeline gets used
	mvp->pco.pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines (
	        renderer.device.device, pipeCache, 1, &mvp->pco.pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create graphics pipeline!");
	}

	mvp->pipelines->push_back (pipeline);
}


void pipeline_game_object ()
{
	VulkanPipeline& pipeMan = renderer.pipelineManager;
	mvp = pipeMan.CreateManagedPipeline ();

	auto pbr_vert =
	    renderer.shaderManager.loadShaderModule ("assets/shaders/pbr.vert.spv", ShaderModuleType::vertex);
	auto pbr_frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/pbr.frag.spv", ShaderModuleType::fragment);

	auto go_vert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/gameObject_shader.vert.spv", ShaderModuleType::vertex);
	auto go_frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/gameObject_shader.frag.spv", ShaderModuleType::fragment);

	auto geom = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/normalVecDebug.geom.spv", ShaderModuleType::geometry);

	ShaderModuleSet pbr_set (pbr_vert, pbr_frag);
	ShaderModuleSet go_set (go_vert, go_frag);

	ShaderModuleSet depthpre (pbr_vert);

	if (usePBR)
		pipeMan.SetShaderModuleSet (mvp, pbr_set);
	else
		pipeMan.SetShaderModuleSet (mvp, go_set);

	pipeMan.SetVertexInput (
	    mvp, Vertex_PosNormTex::getBindingDescription (), Vertex_PosNormTex::getAttributeDescriptions ());
	pipeMan.SetInputAssembly (mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	pipeMan.SetViewport (mvp,
	    (float)renderer.vulkanSwapChain.swapChainExtent.width,
	    (float)renderer.vulkanSwapChain.swapChainExtent.height,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f);

	pipeMan.SetScissor (mvp,
	    renderer.vulkanSwapChain.swapChainExtent.width,
	    renderer.vulkanSwapChain.swapChainExtent.height,
	    0,
	    0);

	pipeMan.SetViewportState (mvp, 1, 1, 0);
	pipeMan.SetRasterizer (
	    mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling (mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil (mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment (mvp,
	    VK_FALSE,
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_ONE,
	    VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending (mvp, 1, &mvp->pco.colorBlendAttachment);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	pipeMan.SetDynamicState (mvp, dynamicStateEnables);


	std::vector<VkDescriptorSetLayout> layouts;
	renderer.AddGlobalLayouts (layouts);
	layouts.push_back (mat->GetDescriptorSetLayout ());
	// layouts.push_back(materialDescriptor->GetLayout());
	pipeMan.SetDescriptorSetLayout (mvp, layouts);

	// VkPushConstantRange pushConstantRange = {};
	// pushConstantRange.offset = 0;
	// pushConstantRange.size = sizeof(ModelPushConstant);
	// pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//
	// pipeMan.SetModelPushConstant(mvp, pushConstantRange);

	pipeMan.BuildPipelineLayout (mvp);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);

	pipeMan.SetRasterizer (
	    mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);

	ShaderModuleSet pbr_geom_set (pbr_vert, pbr_frag, geom);
	ShaderModuleSet go_geom_set (go_vert, go_frag, geom);

	if (usePBR)
		pipeMan.SetShaderModuleSet (mvp, pbr_geom_set);
	else
		pipeMan.SetShaderModuleSet (mvp, go_geom_set);

	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);
	// pipeMan.CleanShaderResources(mvp);
}

void pipeline_instanced_object ()
{

	VulkanPipeline& pipeMan = renderer.pipelineManager;
	mvp = pipeMan.CreateManagedPipeline ();

	// pipeMan.SetVertexShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/instancedSceneObject.vert.spv"));
	// pipeMan.SetFragmentShader(mvp, fragShaderModule);

	auto vert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/instancedSceneObject.vert.spv", ShaderModuleType::vertex);
	auto frag = renderer.shaderManager.loadShaderModule (fragShaderPath, ShaderModuleType::fragment);

	ShaderModuleSet set (vert, frag, {}, {}, {});
	pipeMan.SetShaderModuleSet (mvp, set);

	// pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	pipeMan.SetInputAssembly (mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport (mvp,
	    (float)renderer.vulkanSwapChain.swapChainExtent.width,
	    (float)renderer.vulkanSwapChain.swapChainExtent.height,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f);
	pipeMan.SetScissor (mvp,
	    renderer.vulkanSwapChain.swapChainExtent.width,
	    renderer.vulkanSwapChain.swapChainExtent.height,
	    0,
	    0);
	pipeMan.SetViewportState (mvp, 1, 1, 0);
	pipeMan.SetRasterizer (
	    mvp, VK_POLYGON_MODE_FILL, cullModeFlagBits, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling (mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil (mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment (mvp,
	    enableBlending,
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_ALPHA,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	pipeMan.SetColorBlending (mvp, 1, &mvp->pco.colorBlendAttachment);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	pipeMan.SetDynamicState (mvp, dynamicStateEnables);

	std::vector<VkDescriptorSetLayout> layouts;
	renderer.AddGlobalLayouts (layouts);
	layouts.push_back (descriptor->GetLayout ());
	pipeMan.SetDescriptorSetLayout (mvp, layouts);



	std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
		// Binding point 0: Mesh vertex layout description at per-vertex rate
		initializers::vertexInputBindingDescription (
		    VERTEX_BUFFER_BIND_ID, sizeof (Vertex_PosNormTexColor), VK_VERTEX_INPUT_RATE_VERTEX),
		// Binding point 1: Instanced data at per-instance rate
		initializers::vertexInputBindingDescription (INSTANCE_BUFFER_BIND_ID, sizeof (InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
	};

	// Vertex attribute bindings
	// Note that the shader declaration for per-vertex and per-instance attributes is the same, the
	// different input rates are only stored in the bindings: instanced.vert:
	//	layout (location = 0) in vec3 inPos;			Per-Vertex
	//	...
	//	layout (location = 4) in vec3 instancePos;	Per-Instance
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
		// Per-vertex attributees
		// These are advanced for each vertex fetched by the vertex shader
		initializers::vertexInputAttributeDescription (
		    VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, 0), // Location 0: Position
		initializers::vertexInputAttributeDescription (
		    VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3), // Location 1: Normal
		initializers::vertexInputAttributeDescription (
		    VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, sizeof (float) * 6), // Location 2: Texture coordinates
		initializers::vertexInputAttributeDescription (
		    VERTEX_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 8), // Location 3: Color

		// Per-Instance attributes
		// These are fetched for each instance rendered
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, 0), // Location 4: Position
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3), // Location 5: Rotation
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32_SFLOAT, sizeof (float) * 6), // Location 6: Scale
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 7, VK_FORMAT_R32_SINT, sizeof (float) * 7), // Location 7: Texture array layer index
	};

	pipeMan.SetVertexInput (mvp, bindingDescriptions, attributeDescriptions);

	pipeMan.BuildPipelineLayout (mvp);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);

	pipeMan.SetRasterizer (
	    mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);

	// pipeMan.CleanShaderResources(mvp);
	// pipeMan.SetVertexShader(mvp, loadShaderModule(renderer.device.device,
	// "assets/shaders/normalVecDebug.vert.spv")); pipeMan.SetFragmentShader(mvp,
	// loadShaderModule(renderer.device.device, "assets/shaders/normalVecDebug.frag.spv")); pipeMan.SetGeometryShader(mvp,
	// loadShaderModule(renderer.device.device, "assets/shaders/normalVecDebug.geom.spv"));
	//
	// pipeMan.BuildPipeline(mvp, renderer.renderPass->Get(), 0);
	// pipeMan.CleanShaderResources(mvp);
}

void pipeline_terrain ()
{
	VulkanPipeline& pipeMan = renderer.pipelineManager;
	mvp = pipeMan.CreateManagedPipeline ();

	// pipeMan.SetVertexShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/terrain.vert.spv"));
	// pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/terrain.frag.spv"));

	auto vert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/terrain.vert.spv", ShaderModuleType::vertex);
	auto frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/terrain.frag.spv", ShaderModuleType::fragment);

	ShaderModuleSet set (vert, frag, {}, {}, {});
	pipeMan.SetShaderModuleSet (mvp, set);

	pipeMan.SetVertexInput (
	    mvp, Vertex_PosNormTex::getBindingDescription (), Vertex_PosNormTex::getAttributeDescriptions ());
	pipeMan.SetInputAssembly (mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport (mvp,
	    (float)renderer.vulkanSwapChain.swapChainExtent.width,
	    (float)renderer.vulkanSwapChain.swapChainExtent.height,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f);
	pipeMan.SetScissor (mvp,
	    renderer.vulkanSwapChain.swapChainExtent.width,
	    renderer.vulkanSwapChain.swapChainExtent.height,
	    0,
	    0);
	pipeMan.SetViewportState (mvp, 1, 1, 0);
	pipeMan.SetRasterizer (
	    mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling (mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil (mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment (mvp,
	    VK_FALSE,
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_ONE,
	    VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending (mvp, 1, &mvp->pco.colorBlendAttachment);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	pipeMan.SetDynamicState (mvp, dynamicStateEnables);

	std::vector<VkDescriptorSetLayout> layouts;
	renderer.AddGlobalLayouts (layouts);
	layouts.push_back (descriptor->GetLayout ());
	pipeMan.SetDescriptorSetLayout (mvp, layouts);

	// VkPushConstantRange pushConstantRange = {};
	// pushConstantRange.offset = 0;
	// pushConstantRange.size = sizeof(TerrainPushConstant);
	// pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// pipeMan.SetModelPushConstant(mvp, pushConstantRange);

	pipeMan.BuildPipelineLayout (mvp);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);

	pipeMan.SetRasterizer (
	    mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);

	auto normalVert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/normalVecDebug.vert.spv", ShaderModuleType::vertex);
	auto normalFrag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/normalVecDebug.frag.spv", ShaderModuleType::fragment);
	auto normalGeom = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/normalVecDebug.geom.spv", ShaderModuleType::geometry);

	ShaderModuleSet normalSset (normalVert, normalFrag, normalGeom, {}, {});
	pipeMan.SetShaderModuleSet (mvp, normalSset);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);
}

void pipeline_skybox ()
{
	VulkanPipeline& pipeMan = renderer.pipelineManager;
	mvp = pipeMan.CreateManagedPipeline ();

	auto vert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/skybox.vert.spv", ShaderModuleType::vertex);
	auto frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/skybox.frag.spv", ShaderModuleType::fragment);

	ShaderModuleSet set (vert, frag, {}, {}, {});
	pipeMan.SetShaderModuleSet (mvp, set);
	// pipeMan.SetVertexShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/skybox.vert.spv"));
	// pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/skybox.frag.spv"));

	pipeMan.SetVertexInput (mvp,
	    Vertex_PosNormTexColor::getBindingDescription (),
	    Vertex_PosNormTexColor::getAttributeDescriptions ());
	pipeMan.SetInputAssembly (mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport (mvp,
	    (float)renderer.vulkanSwapChain.swapChainExtent.width,
	    (float)renderer.vulkanSwapChain.swapChainExtent.height,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f);
	pipeMan.SetScissor (mvp,
	    renderer.vulkanSwapChain.swapChainExtent.width,
	    renderer.vulkanSwapChain.swapChainExtent.height,
	    0,
	    0);
	pipeMan.SetViewportState (mvp, 1, 1, 0);
	pipeMan.SetRasterizer (
	    mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling (mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil (mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment (mvp,
	    VK_FALSE,
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_ONE,
	    VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending (mvp, 1, &mvp->pco.colorBlendAttachment);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	pipeMan.SetDynamicState (mvp, dynamicStateEnables);

	std::vector<VkDescriptorSetLayout> layouts;
	renderer.AddGlobalLayouts (layouts);
	layouts.push_back (descriptor->GetLayout ());
	pipeMan.SetDescriptorSetLayout (mvp, layouts);

	pipeMan.BuildPipelineLayout (mvp);
	pipeMan.BuildPipeline (mvp, renderer.renderPass->Get (), 0);

	// pipeMan.CleanShaderResources(mvp);
}
