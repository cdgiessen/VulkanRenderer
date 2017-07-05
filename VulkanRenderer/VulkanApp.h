#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <string>
#include <array>
#include <set>

#include "VulkanInitializers.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanDevice.hpp"

#include "Mesh.h"
#include "Camera.h"

const int WIDTH = 800;
const int HEIGHT = 600;

const float deltaTime = 0.016f;

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class VulkanApp
{
public:
	VulkanApp();
	~VulkanApp();

	void initWindow();
	void initVulkan();
	void mainLoop();
	void drawFrame();
	void handleInputs();
	void cleanup();

	void cleanupSwapChain();
	void recreateSwapChain();

	void MouseMoved(double xpos, double ypos);
	void MouseClicked(int button, int action, int mods);

private:

	bool firstMouse;
	double lastX, lastY;
	bool mouseControlEnabled;
	void SetMouseControl(bool value);

	
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	//void createCommandPool();
	void createDepthResources();
	void createFramebuffers();

	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createMeshBuffers(Mesh* mesh);
	void createVertexBuffer(Mesh* mesh);
	void createIndexBuffer(Mesh* mesh);
	void createUniformBuffer();
	//void createUniformBufferCUBE();
	void createDescriptorPool();
	void createDescriptorSet(VkDescriptorSet& descriptorSet);
	void createCommandBuffers();
	void createSemaphores();

	void updateUniformBuffer();
	void updateUniformBufferCUBE();



	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	Mesh* myMesh;
	Mesh* mySecondMesh;
	Camera* camera;

	VulkanDevice vulkanDevice;

	VulkanSwapChain vulkanSwapChain;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSetOne;
	VkDescriptorSet descriptorSetTwo;

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
};

