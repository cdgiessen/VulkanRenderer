#include "Water.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Water::Water (Resource::AssetManager& resourceMan, VulkanRenderer& renderer) : renderer (renderer)
{
	mesh = create_water_plane_subdiv (15, 50);
	model = std::make_unique<VulkanModel> (renderer, mesh);

	texture = resourceMan.texManager.GetTexIDByName ("TileableWaterTexture");
	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8);
	vulkanTexture = renderer.textureManager.CreateTexture2D (texture, details);

	uniformBuffer = std::make_shared<VulkanBufferUniform> (renderer.device, sizeof (ModelBufferObject));

	ubo.model = glm::mat4 (1.0f);
	ubo.model = glm::translate (ubo.model, glm::vec3 (0, 0, 0));
	ubo.normal = glm::transpose (glm::inverse (glm::mat3 (ubo.model)));

	uniformBuffer->CopyToBuffer (&ubo, sizeof (ModelBufferObject));

	descriptor = renderer.GetVulkanDescriptor ();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	descriptor->SetupLayout (m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor->SetupPool (poolSizes);

	m_descriptorSet = descriptor->CreateDescriptorSet ();

	std::vector<DescriptorUse> writes;
	writes.push_back (DescriptorUse (0, 1, uniformBuffer->resource));
	writes.push_back (DescriptorUse (1, 1, vulkanTexture->resource));
	descriptor->UpdateDescriptorSet (m_descriptorSet, writes);

	PipelineOutline out;

	auto water_vert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/water.vert.spv", ShaderModuleType::vertex);
	auto water_frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/water.frag.spv", ShaderModuleType::fragment);
	out.SetShaderModuleSet (ShaderModuleSet (water_vert, water_frag));

	out.UseModelVertexLayout (*model.get ());

	out.SetInputAssembly (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

	out.AddViewport (1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	out.AddScissor (1, 1, 0, 0);

	out.SetRasterizer (
	    VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	out.SetMultisampling (VK_SAMPLE_COUNT_1_BIT);
	out.SetDepthStencil (VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	out.AddColorBlendingAttachment (VK_TRUE,
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_ONE,
	    VK_BLEND_FACTOR_ZERO);

	out.AddDescriptorLayouts (renderer.GetGlobalLayouts ());
	out.AddDescriptorLayout (descriptor->GetLayout ());

	out.AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

	pipe = std::make_unique<Pipeline> (renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));

	out.SetRasterizer (
	    VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	wireframe = std::make_unique<Pipeline> (
	    renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));
}

void Water::UpdateUniform (glm::vec3 camera_pos)
{
	auto pos = camera_pos;
	pos.y = 0;
	ubo.model = glm::translate (glm::mat4 (1.0f), pos);
	uniformBuffer->CopyToBuffer (&ubo, sizeof (ModelBufferObject));
}

void Water::Draw (VkCommandBuffer cmdBuf, bool wireframe)
{
	model->BindModel (cmdBuf);
	if (wireframe)
		this->wireframe->Bind (cmdBuf);
	else
		pipe->Bind (cmdBuf);
	vkCmdBindDescriptorSets (
	    cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->GetLayout (), 2, 1, &m_descriptorSet.set, 0, nullptr);

	vkCmdDrawIndexed (cmdBuf, model->indexCount, 1, 0, 0, 0);
}
