#pragma once

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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED	
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  

#include "Window.hpp"
#include "Input.h"
#include "Logger.h"

#include "..\vulkan\VulkanDevice.hpp"

#include "..\third-party\ImGui\imgui.h"
#include "..\gui\ImGuiImpl.h"

#include "TimeManager.h"
#include "..\scene\Scene.h"

#include "..\gui\NodeGraph.h"

const int WIDTH = 800;
const int HEIGHT = 600;

class VulkanApp
{
public:
	VulkanApp();
	~VulkanApp();

	//void initWindow();
	void initVulkan();
	void mainLoop();
	void HandleInputs();
	void drawFrame();
	void cleanup();

	void cleanupSwapChain();
	void recreateSwapChain();
	void reBuildCommandBuffers();

	//GLFW Callbacks
	//void MouseMoved(double xpos, double ypos);
	//void MouseClicked(int button, int action, int mods);
	//void KeyboardEvent(int key, int scancode, int action, int mods);

private:

	void createRenderPass();
	void createDepthResources();
	void createFramebuffers();

	void buildCommandBuffers();

	void createCommandBuffers();
	void createSemaphores();

	//ImGUI functions
	void PrepareImGui();
	void BuildImgui();
	void CleanUpImgui();

	void CreatePrimaryCommandBuffer();

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	Window* window;
	TimeManager* timeManager;

	Scene* scene;

	//Input stuff
	bool mouseControlEnabled;
	bool wireframe = false;
	void SetMouseControl(bool value);

	//ImGui resources
	VkDescriptorPool imgui_descriptor_pool;
	NodeGraph nodeGraph_terrain;
	SimpleTimer imGuiTimer;
	Logger appLog;
	

	//Vulkan specific members
	//uint32_t frameIndex = 1; // which frame of the swapchain it is on
	VulkanDevice vulkanDevice;
	VulkanSwapChain vulkanSwapChain;
	VkRenderPass renderPass;

	VulkanPipeline* pipelineManager;

	//Depth buffer
	VkImage depthImage; 
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	//Command buffer per frame
	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;
};

