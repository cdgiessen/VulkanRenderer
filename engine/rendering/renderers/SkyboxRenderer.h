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
class RenderCameras;
class Models;

class Skybox
{
	public:
	Skybox (RenderCameras& render_cameras,
	    Models& models,
	    DescriptorLayout descriptor_layout,
	    DescriptorPool& descriptor_pool,
	    DescriptorSet descriptor_set,
	    VulkanBuffer& uniform_buffer,
	    ModelID skybox_cube_model,
	    PipelineLayout& pipe_layout,
	    GraphicsPipeline& pipe);

	void update (ViewCameraID cam_id);
	void Draw (VkCommandBuffer cmdBuf);

	private:
	RenderCameras& render_cameras;
	Models& models;

	DescriptorLayout descriptor_layout;
	DescriptorPool descriptor_pool;
	DescriptorSet descriptor_set;

	VulkanBuffer uniform_buffer;

	ModelID skybox_cube_model;
	PipelineLayout pipe_layout;
	GraphicsPipeline pipe;
};

std::optional<Skybox> CreateSkybox (
    BackEnd& back_end, RenderCameras& render_cameras, Resource::Texture::TexID tex_resource);