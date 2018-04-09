#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>
#include <vulkan/vulkan.h>




#include "../rendering/Model.hpp"
#include "../rendering/Renderer.hpp"
#include "../rendering/Texture.hpp"
#include "../rendering/Descriptor.hpp"

#include "../resources/Mesh.h"
#include "../resources/Texture.h"

class GameObject
{
  public:
    GameObject();
    ~GameObject();

    std::shared_ptr<VulkanRenderer> renderer;

    std::shared_ptr<ManagedVulkanPipeline> mvp;

	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

    std::shared_ptr<Mesh> gameObjectMesh;
	std::shared_ptr<VulkanModel> gameObjectModel;

    std::shared_ptr<Texture> gameObjectTexture;
	std::shared_ptr<VulkanTexture2D> gameObjectVulkanTexture;

	ModelPushConstant modelPushConstant;

    void InitGameObject(std::shared_ptr<VulkanRenderer> renderer);
    void CleanUp();
    void UpdateUniformBuffer(float time);

    void LoadModel(std::string fileName);
    void LoadModel(std::shared_ptr<Mesh> mesh);

	//void SetupUniformBuffer();
    void SetupImage();
    void SetupModel();
    void SetupPipeline();

    void SetupDescriptor();

	void Draw(VkCommandBuffer commandBuffer, bool wireframe, bool drawNormals);
};
