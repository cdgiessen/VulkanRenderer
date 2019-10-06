#pragma once

#include <filesystem>

#include "Input.h"
#include "JobSystem.h"
#include "Logger.h"
#include "TimeManager.h"
#include "Window.h"
#include "util/SimpleTimer.h"

#include "resources/Resource.h"

#include "rendering/Renderer.h"

#include "scene/Scene.h"

#include "gui/ImGuiImpl.h"
#include "gui/ProcTerrainNodeGraph.h"

struct ImGUI_PanelSettings
{
	bool showGui = true;
	bool camera_controls = true;
	bool log = true;
	bool debug_overlay = true;
	bool controls_list = true;
	bool controller_list = true;
};

class VulkanAppSettings
{
	public:
	VulkanAppSettings (std::filesystem::path fileName);
	void Load ();
	void Save ();

	int screenWidth = 800;
	int screenHeight = 600;
	bool isFullscreen = false;

	bool useValidationLayers = true;

	bool isFrameCapped = true;
	double MaxFPS = 100.0f;

	private:
	std::filesystem::path fileName;
};

class VulkanApp
{
	public:
	VulkanApp ();
	~VulkanApp ();

	void Run ();
	void HandleInputs ();

	void RecreateSwapChain ();

	private:
	VulkanAppSettings settings;

	job::TaskManager task_manager;

	TimeManager time_manager;
	Window window;
	Resource::AssetManager resource_manager;

	VulkanRenderer vulkan_renderer;

	ProcTerrainNodeGraph imgui_nodeGraph_terrain;
	Scene scene;

	bool debug_mode = true;

	ImGUI_PanelSettings panels;


	////ImGUI functions
	void BuildImgui ();

	void DebugOverlay (bool* show_debug_overlay);
	void CameraWindow (bool* show_camera_overlay);
	void ControlsWindow (bool* show_controls_window);
	void ControllerWindow (bool* show_controller_window);
	// ImGui resources
	SimpleTimer imGuiTimer;

	float tempCameraSpeed = 0.0f;
};

extern int run_engine ();