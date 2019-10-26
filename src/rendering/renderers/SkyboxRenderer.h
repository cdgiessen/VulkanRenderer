#pragma once

#include <memory>

#include <vulkan/vulkan.h>

#include "cml/cml.h"

#include "rendering/Model.h"
#include "rendering/Texture.h"
#include "rendering/ViewCamera.h"

class VulkanRenderer;

class SkyboxRenderer
{
	public:
	SkyboxRenderer (VulkanRenderer& renderer, VulkanTextureID cube_map);

	void Update (Camera& cam);
	void Draw (VkCommandBuffer commandBuffer);

	private:
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
};
