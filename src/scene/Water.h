#pragma once

#include <memory>

#include "cml/cml.h"

#include "rendering/Renderer.h"

#include "rendering/Model.h"
#include "rendering/Pipeline.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include "resources/Mesh.h"
#include "resources/Resource.h"

class Water
{
	VulkanRenderer& renderer;
	std::unique_ptr<VulkanModel> model;
	PipeID pipe;
	PipeID wireframe;

	Resource::Texture::TexID texture;
	VulkanTextureID vulkanTexture;

	std::unique_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	ModelBufferObject ubo = {};
	std::unique_ptr<VulkanBuffer> uniformBuffer;

	public:
	Water (Resource::AssetManager& resourceMan, VulkanRenderer& renderer);

	void UpdateUniform (cml::vec3f camera_pos);

	void Draw (VkCommandBuffer cmdBuf, bool wireframe);
};
