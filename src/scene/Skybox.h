#pragma once

#include <string>


#include <vulkan/vulkan.h>

#include "cml/cml.h"

#include "resources/Texture.h"

#include "rendering/Model.h"
#include "rendering/Renderer.h"
#include "rendering/Texture.h"


struct SkyboxUniformBuffer
{
	cml::mat4f proj;
	cml::mat4f view;
};

class Skybox
{
	public:
	Skybox (VulkanRenderer& renderer);
	~Skybox ();

	VulkanRenderer& renderer;

	PipeID normal;

	std::unique_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::unique_ptr<VulkanModel> model;

	Resource::Texture::TexID skyboxCubeMap;
	VulkanTextureID vulkanCubeMap;

	std::unique_ptr<VulkanBuffer> skyboxUniformBuffer;

	void InitSkybox ();

	void UpdateUniform (cml::mat4f proj, cml::mat4f view);

	void SetupUniformBuffer ();
	void SetupCubeMapImage ();
	void SetupDescriptor ();
	void SetupPipeline ();

	void WriteToCommandBuffer (VkCommandBuffer commandBuffer);
};
