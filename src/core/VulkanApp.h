#pragma once

#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Window.h"
#include "Input.h"
#include "Logger.h"
#include "TimeManager.h"
#include "CoreTools.h"

#include "../resources/ResourceManager.h"

#include "../rendering/Renderer.h"

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

class VulkanAppSettings {
public:
	VulkanAppSettings(std::string fileName);
	void Load();
	void Save();

	int screenWidth = 800;
	int screenHeight = 600;
	bool isFullscreen = false;

	bool useValidationLayers = true;

	bool isFrameCapped = true;
	double MaxFPS = 100.0f;
private:
	std::string fileName;
};


class VulkanApp
{
public:
	VulkanApp();
	~VulkanApp();

	void mainLoop();
	void HandleInputs();
	//void clean();

	void RecreateSwapChain();

private:
	VulkanAppSettings settings;

	std::unique_ptr<Window> window;
	std::unique_ptr<ResourceManager> resourceManager;
	std::unique_ptr<TimeManager> timeManager;

	std::unique_ptr<VulkanRenderer> vulkanRenderer;

	std::unique_ptr<Scene> scene;

	//Input stuff
	//bool mouseControlEnabled = true;
	bool wireframe = false;
	//void SetMouseControl(bool value);

	ImGUI_PanelSettings panels;

	////ImGUI functions
	void BuildImgui();

	void DebugOverlay(bool* show_debug_overlay);
	void CameraWindow(bool* show_camera_overlay);
	void ControlsWindow(bool* show_controls_window);

	//ImGui resources
	ProcTerrainNodeGraph imgui_nodeGraph_terrain;
	SimpleTimer imGuiTimer;
	Log::Logger appLog;
};

