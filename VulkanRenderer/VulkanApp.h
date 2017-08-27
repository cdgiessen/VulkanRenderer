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


#include "VulkanSwapChain.hpp"
#include "VulkanDevice.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"

#include "ImGui\imgui.h"
#include "ImGuiImpl.h"

#include "Mesh.h"
#include "Camera.h"
#include "Terrain.h"
#include "Skybox.h"
#include "GameObject.h"
#include "Water.h"

const int WIDTH = 1000;
const int HEIGHT = 800;

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

	//Callbacks
	void MouseMoved(double xpos, double ypos);
	void MouseClicked(int button, int action, int mods);
	void KeyboardEvent(int key, int scancode, int action, int mods);

private:

	void createRenderPass();
	void createDepthResources();
	void createFramebuffers();

	void createTextureImage(VkImage image, VkDeviceMemory imageMemory);
	void createTextureSampler(VkSampler* textureSampler);
	void createUniformBuffers();
	void buildCommandBuffer(uint32_t index);

	void createDescriptorSets();

	void createCommandBuffers();
	void createSemaphores();

	void updateScene();
	void RecreateTerrain();

	//ImGUI functions
	void PrepareImGui();
	void BuildImgui();
	void CleanUpImgui();

	void CreatePrimaryCommandBuffer();

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

	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
	SimpleTimer frameTimer;

	double timeSinceStart = 0.0f; //in seconds
	double deltaTime = 0.016f; //in seconds
	std::array<float, 50> frameTimes{};
	float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;

	//Input stuff
	Camera* camera;
	bool firstMouse;
	double lastX, lastY;
	bool mouseControlEnabled;
	bool keys[512] = {};
	bool wireframe = false;
	bool walkOnGround = false;
	void SetMouseControl(bool value);

	//global uniform and lighting
	VulkanBuffer globalVariableBuffer;
	VulkanBuffer lightsInfoBuffer;
	std::vector<PointLight> pointLights;
	
	//Scene objects to render
	Skybox* skybox;
	GameObject* cubeObject;
	std::vector<Terrain*> terrains;
	std::vector<Water*> waters;

	bool recreateTerrain = false;
	float terrainWidth = 1000;
	int terrainMaxLevels = 3;
	int terrainGridDimentions = 1;
	SimpleTimer terrainUpdateTimer;

	//ImGui resources
	VkDescriptorPool imgui_descriptor_pool;
	

	//Vulkan specific members
	uint32_t frameIndex = 1; // which frame of the swapchain it is on
	VulkanDevice vulkanDevice;
	VulkanSwapChain vulkanSwapChain;
	VkRenderPass renderPass;

	VkImage depthImage; 
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;
};

