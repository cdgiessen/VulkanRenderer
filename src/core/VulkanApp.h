#pragma once

#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED	

//#include <vulkan/vulkan.h>

#include "Window.hpp"
#include "Input.h"
#include "Logger.h"
#include "TimeManager.h"
#include "CoreTools.h"

#include "../resources/ResourceManager.h"

#include "../rendering/VulkanRenderer.hpp"

#include "../scene/Scene.h"

#include "../gui/ImGuiImpl.h"
#include "../gui/ProcTerrainNodeGraph.h"

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

	std::shared_ptr<Window> window;
	std::shared_ptr<ResourceManager> resourceManager;
	std::shared_ptr<TimeManager> timeManager;

	std::shared_ptr<Scene> scene;
	
	std::shared_ptr<VulkanRenderer> vulkanRenderer;

	//Input stuff
	bool mouseControlEnabled = true;
	bool wireframe = false;
	bool showGui = true;
	void SetMouseControl(bool value);

	void ReadSettings();

	//ImGUI functions
	void PrepareImGui();
	void BuildImgui();
	void CleanUpImgui();

	void DebugOverlay(bool* show_debug_overlay);
	void CameraWindow(bool* show_camera_overlay);

	//ImGui resources
	//VkDescriptorPool imgui_descriptor_pool;
	ProcTerrainNodeGraph imgui_nodeGraph_terrain;
	SimpleTimer imGuiTimer;
	Log::Logger appLog;

	int screenWidth = 800;
	int screenHeight = 600;

	bool useValidationLayers = true;

};

