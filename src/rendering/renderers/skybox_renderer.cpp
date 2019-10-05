#include "Skybox.h"

struct SkyboxUniformBuffer
{
	cml::mat4f proj;
	cml::mat4f view;
};

SkyboxRenderer::SkyboxRenderer (VulkanRenderer& renderer, VulkanTextureID cube_map)
: renderer (renderer), cube_map (cube_map){};

void SkyboxRenderer::InitSkybox ()
{

	skyboxUniformBuffer =
	    std::make_unique<VulkanBuffer> (renderer.device, uniform_details (sizeof (SkyboxUniformBuffer)));

	SetupCubeMapImage ();
	SetupDescriptor ();
	SetupPipeline ();
}

void SkyboxRenderer::SetupDescriptor ()
{
	descriptor = std::make_unique<VulkanDescriptor> (renderer.device);

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
	writes.push_back (DescriptorUse (0, 1, skyboxUniformBuffer->GetResource ()));
	writes.push_back (DescriptorUse (1, 1, renderer.texture_manager.GetResource (vulkanCubeMap)));
	descriptor->UpdateDescriptorSet (m_descriptorSet, writes);
}

void SkyboxRenderer::SetupPipeline ()
{
	PipelineOutline out;

	auto vert = renderer.shader_manager.get_module ("skybox", ShaderType::vertex);
	auto frag = renderer.shader_manager.get_module ("skybox", ShaderType::fragment);

	ShaderModuleSet shader_set;
	shader_set.Vertex (vert.value ()).Fragment (frag.value ());
	out.SetShaderModuleSet (shader_set);

	out.UseModelVertexLayout (*model.get ());

	out.AddViewport (1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	out.AddScissor (1, 1, 0, 0);

	out.SetInputAssembly (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

	out.SetRasterizer (
	    VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	out.SetMultisampling (VK_SAMPLE_COUNT_1_BIT);
	out.SetDepthStencil (VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_FALSE, VK_FALSE);
	out.AddColorBlendingAttachment (VK_FALSE,
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

	pipe = renderer.pipeline_manager.MakePipe (out, renderer.GetRelevantRenderpass (RenderableType::opaque));
}

void SkyboxRenderer::UpdateUniform (cml::mat4f proj, cml::mat4f view)
{
	SkyboxUniformBuffer sbo = {};
	sbo.proj = proj;
	sbo.view = view;
	sbo.view.set_col (3, cml::vec4f::w_positive);

	skyboxUniformBuffer->CopyToBuffer (sbo);
};


void SkyboxRenderer::Draw (VkCommandBuffer commandBuffer)
{

	vkCmdBindDescriptorSets (commandBuffer,
	    VK_PIPELINE_BIND_POINT_GRAPHICS,
	    renderer.pipeline_manager.GetPipeLayout (pipe),
	    2,
	    1,
	    &m_descriptorSet.set,
	    0,
	    nullptr);


	renderer.pipeline_manager.BindPipe (pipe, commandBuffer);


	model->BindModel (commandBuffer);
	vkCmdDrawIndexed (commandBuffer, static_cast<uint32_t> (model->indexCount), 1, 0, 0, 0);
}
