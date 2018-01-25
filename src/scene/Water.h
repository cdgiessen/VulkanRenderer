#pragma once
 

#include "../rendering/VulkanRenderer.hpp"
#include "../rendering/VulkanModel.hpp"
#include "../rendering/VulkanTexture.hpp"
#include "../rendering/VulkanDescriptor.hpp"

#include "../resources/Mesh.h"
#include "../resources/Texture.h"
#include <glm/common.hpp>

class Water {
public:
	int numCells; //
	glm::vec3 pos; //position of corner
	glm::vec3 size; //width and length
	std::shared_ptr<Mesh> mesh;

	std::shared_ptr<VulkanRenderer> renderer;

	std::shared_ptr<ManagedVulkanPipeline> mvp;

	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::shared_ptr<Mesh> WaterMesh;
	VulkanModel WaterModel;

	std::shared_ptr<Texture> WaterTexture;
	VulkanTexture2D WaterVulkanTexture;

	ModelBufferObject modelUniformObject;
	VulkanBufferUniform modelUniformBuffer;

	std::vector<VkCommandBuffer> commandBuffers;

	Water(int numCells, float posX, float posY, float sizeX, float sizeY);
	~Water();

	void InitWater(std::shared_ptr<VulkanRenderer> renderer);
	void CleanUp();
	
	//void UpdateUniformBuffer(float time, glm::mat4 view);

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();
	void SetupPipeline();

	void SetupDescriptor();

	//void BuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass);
	//void RebuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass);
};
