#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>
#include <vulkan/vulkan.h>




#include "../rendering/VulkanModel.hpp"
#include "../rendering/VulkanRenderer.hpp"
#include "../rendering/VulkanTexture.hpp"

#include "../resources/Mesh.h"
#include "../resources/Texture.h"

class GameObject
{
  public:
    GameObject();
    ~GameObject();

    std::shared_ptr<VulkanRenderer> renderer;

    std::shared_ptr<ManagedVulkanPipeline> mvp;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    std::shared_ptr<Mesh> gameObjectMesh;
    VulkanModel gameObjectModel;

    std::shared_ptr<Texture> gameObjectTexture;
    VulkanTexture2D gameObjectVulkanTexture;

    ModelBufferObject modelUniformObject;
    VulkanBuffer modelUniformBuffer;

    void InitGameObject(std::shared_ptr<VulkanRenderer> renderer, VulkanBuffer &global, VulkanBuffer &lighting);
    void CleanUp();
    void UpdateUniformBuffer(float time);

    void LoadModel(std::string fileName);
    void LoadModel(std::shared_ptr<Mesh> mesh);

    void SetupUniformBuffer();
    void SetupImage();
    void SetupModel();
    void SetupPipeline();

    void SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting);

	void Draw(VkCommandBuffer commandBuffer, bool wireframe, bool drawNormals);
};
