#pragma once

#include <memory>

#include <vulkan/vulkan.h>

#include "cml/cml.h"

#include "rendering/Model.h"
#include "rendering/Renderer.h"
#include "rendering/Texture.h"

class SkyboxRenderer
{
	public:
	SkyboxRenderer (VulkanRenderer& renderer, VulkanTextureID cube_map);


	VulkanRenderer& renderer;

	PipeID pipe;

	std::unique_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::unique_ptr<VulkanModel> model;

	VulkanTextureID cube_map;

	std::unique_ptr<VulkanBuffer> skyboxUniformBuffer;

	void InitSkybox ();
	void SetupDescriptor ();
	void SetupPipeline ();


	void UpdateUniform (cml::mat4f proj, cml::mat4f view);

	void Draw (VkCommandBuffer commandBuffer);
};
