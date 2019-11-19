#include "SkyboxRenderer.h"

#include <utility>

#include "rendering/backend/BackEnd.h"

struct SkyboxUniformBuffer
{
	cml::mat4f proj;
	cml::mat4f view;
};

SkyboxManager::SkyboxManager (BackEnd& back_end, ViewCameraManager& camera_manager)
: back_end (back_end), camera_manager (camera_manager)
{
	std::vector<DescriptorSetLayoutBinding> m_bindings = {
		{ DescriptorType::uniform_buffer, ShaderStage::vertex, 0, 1 },
		{ DescriptorType::combined_image_sampler, ShaderStage::fragment, 1, 1 }
	};
	descriptor_layout = back_end.descriptor_manager.CreateDescriptorSetLayout (m_bindings);
	skybox_cube_model = back_end.model_manager.CreateModel (Resource::Mesh::CreateCube ());

	CreatePipeline ();
}

SkyboxID SkyboxManager::CreateSkybox (Resource::Texture::TexID skyboxCubeMap)
{
	auto uniform_buffer = VulkanBuffer (back_end.device, uniform_details (sizeof (SkyboxUniformBuffer)));

	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 3);
	auto vulkanCubeMap = back_end.texture_manager.CreateCubeMap (skyboxCubeMap, details);

	auto descriptorSet = back_end.descriptor_manager.CreateDescriptorSet (descriptor_layout);

	std::vector<DescriptorUse> writes = {
		{ 0, 1, uniform_buffer.GetDescriptorType (), { uniform_buffer.GetDescriptorInfo () } },
		{ 1,
		    1,
		    back_end.texture_manager.GetDescriptorType (vulkanCubeMap),
		    { back_end.texture_manager.GetResource (vulkanCubeMap) } }
	};
	descriptorSet.Update (back_end.device.device, writes);

	SkyboxID new_id = cur_id++;
	Skybox sky = { descriptorSet, vulkanCubeMap, std::move (uniform_buffer) };
	skyboxes.emplace (new_id, std::move (sky));
	return new_id;
}

void SkyboxManager::Update (SkyboxID id, ViewCameraID cam_id)
{
	ViewCameraData& cam = camera_manager.GetCameraData (cam_id);

	SkyboxUniformBuffer sbo = {};
	sbo.proj = cam.GetProjMat ();
	sbo.view = cam.GetViewMat ();
	sbo.view.set_col (3, cml::vec4f::w_positive);

	skyboxes.at (id).uniform_buffer.CopyToBuffer (sbo);
};


void SkyboxManager::Draw (VkCommandBuffer commandBuffer, SpecificPass pass, SkyboxID id)
{
	skyboxes.at (id).descriptor_set.Bind (commandBuffer, back_end.pipeline_manager.GetPipeLayout (pipe), 2);

	back_end.pipeline_manager.BindPipe (commandBuffer, pass, pipe);

	back_end.model_manager.DrawIndexed (commandBuffer, skybox_cube_model);
}

PipeID SkyboxManager::CreatePipeline ()
{
	PipelineOutline out;

	auto vert = back_end.shader_manager.GetModule ("skybox", ShaderType::vertex);
	auto frag = back_end.shader_manager.GetModule ("skybox", ShaderType::fragment);

	ShaderModuleSet shader_set (vert, frag);
	out.SetShaderModuleSet (shader_set);

	out.UseModelVertexLayout (back_end.model_manager.GetLayout (skybox_cube_model));

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

	// out.AddDescriptorLayouts (double_buffer_man.GetGlobalLayouts ());
	out.AddDescriptorLayout (back_end.descriptor_manager.GetLayout (descriptor_layout));

	out.AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

	pipe = back_end.pipeline_manager.MakePipeGroup (out);
	return pipe;
}