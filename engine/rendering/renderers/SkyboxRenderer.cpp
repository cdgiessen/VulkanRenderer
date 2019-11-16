#include "SkyboxRenderer.h"

#include <utility>

#include "rendering/DoubleBuffer.h"
#include "rendering/Model.h"
#include "rendering/Pipeline.h"
#include "rendering/Texture.h"
#include "rendering/ViewCamera.h"

struct SkyboxUniformBuffer
{
	cml::mat4f proj;
	cml::mat4f view;
};

SkyboxManager::SkyboxManager (VulkanDevice& device,
    DescriptorManager& desc_man,
    ShaderManager& shader_man,
    TextureManager& tex_man,
    ModelManager& model_man,
    PipelineManager& pipe_man,
    ViewCameraManager& cam_man)
: device (device),
  desc_man (desc_man),
  shader_man (shader_man),
  tex_man (tex_man),
  model_man (model_man),
  pipe_man (pipe_man),
  cam_man (cam_man),
  descriptor_layout ()
{
	std::vector<DescriptorSetLayoutBinding> m_bindings = {
		{ DescriptorType::uniform_buffer, ShaderStage::vertex, 0, 1 },
		{ DescriptorType::combined_image_sampler, ShaderStage::fragment, 1, 1 }
	};
	descriptor_layout = desc_man.CreateDescriptorSetLayout (m_bindings);
	skybox_cube_model = model_man.CreateModel (Resource::Mesh::CreateCube ());

	CreatePipeline ();
}

SkyboxID SkyboxManager::CreateSkybox (Resource::Texture::TexID skyboxCubeMap)
{
	auto uniform_buffer = VulkanBuffer (device, uniform_details (sizeof (SkyboxUniformBuffer)));

	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 3);
	auto vulkanCubeMap = tex_man.CreateCubeMap (skyboxCubeMap, details);

	auto descriptorSet = desc_man.CreateDescriptorSet (descriptor_layout);

	std::vector<DescriptorUse> writes = {
		{ 0, 1, uniform_buffer.GetDescriptorType (), { uniform_buffer.GetDescriptorInfo () } },
		{ 1, 1, tex_man.GetDescriptorType (vulkanCubeMap), { tex_man.GetResource (vulkanCubeMap) } }
	};
	descriptorSet.Update (device.device, writes);

	SkyboxID new_id = cur_id++;
	Skybox sky = { descriptorSet, vulkanCubeMap, std::move (uniform_buffer) };
	skyboxes.emplace (new_id, std::move (sky));
	return new_id;
}

void SkyboxManager::Update (SkyboxID id, ViewCameraID cam_id)
{
	ViewCameraData& cam = cam_man.GetCameraData (cam_id);

	SkyboxUniformBuffer sbo = {};
	sbo.proj = cam.GetProjMat ();
	sbo.view = cam.GetViewMat ();
	sbo.view.set_col (3, cml::vec4f::w_positive);

	skyboxes.at (id).uniform_buffer.CopyToBuffer (sbo);
};


void SkyboxManager::Draw (VkCommandBuffer commandBuffer, SpecificPass pass, SkyboxID id)
{
	skyboxes.at (id).descriptor_set.Bind (commandBuffer, pipe_man.GetPipeLayout (pipe), 2);

	pipe_man.BindPipe (commandBuffer, pass, pipe);

	model_man.DrawIndexed (commandBuffer, skybox_cube_model);
}

PipeID SkyboxManager::CreatePipeline ()
{
	PipelineOutline out;

	auto vert = shader_man.GetModule ("skybox", ShaderType::vertex);
	auto frag = shader_man.GetModule ("skybox", ShaderType::fragment);

	ShaderModuleSet shader_set (vert, frag);
	out.SetShaderModuleSet (shader_set);

	out.UseModelVertexLayout (model_man.GetLayout (skybox_cube_model));

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
	out.AddDescriptorLayout (desc_man.GetLayout (descriptor_layout));

	out.AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

	pipe = pipe_man.MakePipeGroup (out);
	return pipe;
}