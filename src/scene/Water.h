#pragma once

#include <memory>


#include "rendering/Renderer.h"

#include "rendering/Model.h"
#include "rendering/Pipeline.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include "resources/Asset.h"
#include "resources/Mesh.h"
#include "resources/ResourceManager.h"

class Water
{
	VulkanRenderer& renderer;
	std::shared_ptr<MeshData> mesh;
	std::unique_ptr<VulkanModel> model;
	std::unique_ptr<Pipeline> pipe;
	std::unique_ptr<Pipeline> wireframe;

	Resource::Texture::TexID texture;
	std::shared_ptr<VulkanTexture> vulkanTexture;

	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	ModelBufferObject ubo = {};
	std::shared_ptr<VulkanBufferUniform> uniformBuffer;

	public:
	Water (Resource::AssetManager& resourceMan, VulkanRenderer& renderer);

	void UpdateUniform (glm::vec3 camera_pos);

	void Draw (VkCommandBuffer cmdBuf, bool wireframe);
};
