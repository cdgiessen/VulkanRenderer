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

#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\third-party\ImGui\imgui.h"

#include "Window.hpp"
#include "Input.h"
#include "Logger.h"
#include "TimeManager.h"

#include "..\vulkan\VulkanRenderer.hpp"

#include "..\scene\Scene.h"

#include "..\gui\ImGuiImpl.h"
#include "..\gui\NodeGraph.h"

const int WIDTH = 800;
const int HEIGHT = 600;

class VulkanApp
{
public:
	VulkanApp();
	~VulkanApp();

	void mainLoop();
	void HandleInputs();
	void clean();

	void RecreateSwapChain();

private:



	//ImGUI functions
	void PrepareImGui();
	void BuildImgui();
	void CleanUpImgui();

	std::shared_ptr<Window> window;
	std::shared_ptr<TimeManager> timeManager;

	std::shared_ptr<Scene> scene;
	std::shared_ptr<VulkanRenderer> vulkanRenderer;

	//Input stuff
	bool mouseControlEnabled;
	bool wireframe = false;
	void SetMouseControl(bool value);

	//ImGui resources
	VkDescriptorPool imgui_descriptor_pool;
	NodeGraph nodeGraph_terrain;
	SimpleTimer imGuiTimer;
	Logger appLog;
	
};

