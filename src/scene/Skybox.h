#pragma once

#include <string>


#include <vulkan/vulkan.h>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../resources/Texture.h"

#include "../rendering/Model.h"
#include "../rendering/Renderer.h"
#include "../rendering/Texture.h"


struct SkyboxUniformBuffer
{
	glm::mat4 proj;
	glm::mat4 view;
};

class Skybox
{
	public:
	Skybox (VulkanRenderer& renderer);
	~Skybox ();

	VulkanRenderer& renderer;

	std::unique_ptr<Pipeline> normal;

	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::shared_ptr<VulkanModel> model;

	Resource::Texture::TexID skyboxCubeMap;
	std::shared_ptr<VulkanTexture> vulkanCubeMap;

	std::shared_ptr<VulkanBufferUniform> skyboxUniformBuffer;


	void InitSkybox ();

	void UpdateUniform (glm::mat4 proj, glm::mat4 view);

	void SetupUniformBuffer ();
	void SetupCubeMapImage ();
	void SetupDescriptor ();
	void SetupPipeline ();

	void WriteToCommandBuffer (VkCommandBuffer commandBuffer);
};
