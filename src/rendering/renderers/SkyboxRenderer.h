#pragma once

#include <mutex>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include "cml/cml.h"

#include "resources/Texture.h"

#include "rendering/Descriptor.h"
#include "rendering/Model.h"

#include "rendering/Device.h"
#include "rendering/DoubleBuffer.h"
#include "rendering/Pipeline.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "rendering/ViewCamera.h"

class VulkanRenderer;

struct Skybox
{
	DescriptorSet descriptor_set;
	VulkanTextureID cube_map;
	VulkanBuffer uniform_buffer;
};

using SkyboxID = uint32_t;
class SkyboxManager
{
	public:
	SkyboxManager (VulkanDevice& device,
	    DescriptorManager& desc_man,
	    ShaderManager& shader_man,
	    TextureManager& tex_man,
	    ModelManager& model_man,
	    PipelineManager& pipe_man,
	    ViewCameraManager& cam_man);

	SkyboxID CreateSkybox (Resource::Texture::TexID tex_resource);

	void Update (SkyboxID id, ViewCameraID cam_id);
	void Draw (VkCommandBuffer cmdBuf, SpecificPass pass, SkyboxID id);

	private:
	PipeID CreatePipeline ();

	VulkanDevice& device;
	DescriptorManager& desc_man;
	ShaderManager& shader_man;
	TextureManager& tex_man;
	ModelManager& model_man;
	PipelineManager& pipe_man;
	ViewCameraManager& cam_man;

	SkyboxID cur_id = 0;
	std::unordered_map<SkyboxID, Skybox> skyboxes;

	ModelID skybox_cube_model;
	LayoutID descriptor_layout;
	PipeID pipe;
};
