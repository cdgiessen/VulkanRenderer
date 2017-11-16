#pragma once

#include <vulkan\vulkan.h>
#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\vulkan\VulkanRenderer.hpp"
#include "..\vulkan\VulkanModel.hpp"
#include "..\vulkan\VulkanTexture.hpp"

#include "..\core\Mesh.h"
#include "..\core\Texture.h"

class GameObject
{
public:
	GameObject();
	~GameObject();

	std::shared_ptr<VulkanRenderer> renderer;

	VkPipeline pipeline;
	VkPipeline wireframe;
	VkPipeline debugNormals;
	VkPipelineLayout pipelineLayout;

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
	void ReinitGameObject(std::shared_ptr<VulkanRenderer> renderer);
	void CleanUp();
	void UpdateUniformBuffer(float time);

	void LoadModel(std::string fileName);
	void LoadModel(std::shared_ptr<Mesh> mesh);
	void LoadTexture(std::string filename);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();

	void SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting);
	void SetupPipeline();
};

