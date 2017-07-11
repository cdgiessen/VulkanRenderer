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
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"

#include "Mesh.h"
#include "Camera.h"

const int WIDTH = 800;
const int HEIGHT = 600;

const float deltaTime = 0.016f;

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 lightPos = glm::vec4(25.0f, 20.0f, 5.0f, 1.0f);
};

class VulkanApp
{
public:
	VulkanApp();
	~VulkanApp();

	void initWindow();
	void initVulkan();
	void prepareScene();
	void mainLoop();
	void HandleInputs();
	void drawFrame();
	void cleanup();

	void cleanupSwapChain();
	void recreateSwapChain();
	void reBuildCommandBuffers();

	void MouseMoved(double xpos, double ypos);
	void MouseClicked(int button, int action, int mods);
	void KeyboardEvent(int key, int scancode, int action, int mods);

private:

	bool firstMouse;
	double lastX, lastY;
	bool mouseControlEnabled;
	bool keys[512] = {};
	void SetMouseControl(bool value);
	bool wireframe = false;

	void createRenderPass();
	void createDescriptorSetLayout();
	void createPipelineCache();
	void createGraphicsPipelines();
	//void createCommandPool();
	void createDepthResources();
	void createFramebuffers();

	void createTextureImage(VkImage image, VkDeviceMemory imageMemory);
	void createTextureSampler(VkSampler* textureSampler);
	void createUniformBuffers();

	void createDescriptorPool();
	void createDescriptorSet(VkDescriptorSet& descriptorSet);
	void createCommandBuffers();
	void createSemaphores();

	void updateUniformBuffers();




	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	VulkanModel terrain;
	VulkanModel cube;
	VulkanTexture2D statueFace;
	VulkanTexture2D grassTexture;
	Camera* camera;

	VulkanDevice vulkanDevice;

	VulkanSwapChain vulkanSwapChain;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkPipeline wireframePipeline;

	VkImage depthImage; 
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VulkanBuffer uniformVulkanBufferA;
	VulkanBuffer uniformVulkanBufferB;
	VulkanBuffer uniformVulkanBufferC;

	VkDescriptorPool descriptorPoolA;
	VkDescriptorPool descriptorPoolB;
	VkDescriptorSet descriptorSetA;
	VkDescriptorSet descriptorSetB;

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;
};

