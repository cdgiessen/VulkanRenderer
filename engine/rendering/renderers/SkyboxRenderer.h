#pragma once

#include <mutex>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include "cml/cml.h"

#include "rendering/backend/Descriptor.h"
#include "rendering/backend/Model.h"
#include "rendering/backend/Pipeline.h"
#include "rendering/backend/Texture.h"

#include "rendering/ViewCamera.h"
#include "rendering/backend/Buffer.h"

#include "resources/Texture.h"


struct BackEnd;
class ViewCameraManager;

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
	SkyboxManager (BackEnd& back_end, ViewCameraManager& camera_manager);

	SkyboxID CreateSkybox (Resource::Texture::TexID tex_resource);

	void Update (SkyboxID id, ViewCameraID cam_id);
	void Draw (VkCommandBuffer cmdBuf, SpecificPass pass, SkyboxID id);

	private:
	BackEnd& back_end;
	ViewCameraManager& camera_manager;

	PipeID CreatePipeline ();


	SkyboxID cur_id = 0;
	std::unordered_map<SkyboxID, Skybox> skyboxes;

	ModelID skybox_cube_model;
	LayoutID descriptor_layout;
	PipeID pipe;
};
