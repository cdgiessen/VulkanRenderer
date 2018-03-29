#pragma once

#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Window.hpp"
#include "Input.h"
#include "Logger.h"
#include "TimeManager.h"
#include "CoreTools.h"

#include "../resources/ResourceManager.h"

#include "../rendering/Renderer.hpp"

#include "../scene/Scene.h"

#include "../gui/ImGuiImpl.h"
#include "../gui/ProcTerrainNodeGraph.h"

struct ImGUI_PanelSettings {
	bool showGui = true;
	bool camera_controls = true;
	bool log = true;
	bool debug_overlay = true;
	bool controls_list = true;
};


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
	void SetMouseControl(bool value);
	
	ImGUI_PanelSettings panels;

	void ReadSettings();

	//ImGUI functions
	void BuildImgui();
	void CleanUpImgui();

	void DebugOverlay(bool* show_debug_overlay);
	void CameraWindow(bool* show_camera_overlay);
	void ControlsWindow(bool* show_controls_window);

	//ImGui resources
	ProcTerrainNodeGraph imgui_nodeGraph_terrain;
	SimpleTimer imGuiTimer;
	Log::Logger appLog;

	int screenWidth = 800;
	int screenHeight = 600;
	bool isFullscreen = false;

	bool useValidationLayers = true;

};

