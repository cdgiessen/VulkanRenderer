#pragma once

#include <vulkan/vulkan.h>

#include "cml/cml.h"

#include "rendering/Material.h"
#include "rendering/Model.h"
#include "rendering/Renderer.h"
#include "rendering/Texture.h"


#include "resources/Mesh.h"
#include "resources/Texture.h"

class GameObject
{
	public:
	GameObject (VulkanRenderer& renderer);

	void InitGameObject ();

	void SetupUniformBuffer ();
	void SetupImage ();
	void SetupModel ();
	void SetupDescriptor ();
	void SetupMaterial ();
	void SetupPipeline ();


	void UpdateUniformBuffer (float time);

	void DrawDepthPrePass (VkCommandBuffer commandBuffer);
	void Draw (VkCommandBuffer commandBuffer, bool wireframe, bool drawNormals);

	VulkanRenderer& renderer;

	std::unique_ptr<Pipeline> normal;
	std::unique_ptr<Pipeline> wireframe;


	std::unique_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::unique_ptr<VulkanMaterial> mat;

	std::unique_ptr<VulkanDescriptor> materialDescriptor;
	DescriptorSet material_descriptorSet;

	std::unique_ptr<VulkanModel> gameObjectModel;

	Resource::Texture::TexID gameObjectTexture;
	VulkanTextureID gameObjectVulkanTexture;

	std::unique_ptr<VulkanBuffer> uniformBuffer;
	// ModelPushConstant modelPushConstant;

	bool usePBR;
	bool usePBR_Tex;

	Phong_Material phong_mat;
	PBR_Material pbr_mat;
	PBR_Mat_Tex pbr_mat_tex;

	std::unique_ptr<VulkanBuffer> materialBuffer;

	cml::vec3f position;

	bool isReadyToRender = false;
};
