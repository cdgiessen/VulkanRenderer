#pragma once

#include <filesystem>
#include <optional>

#include "util/SimpleTimer.h"

#include "resources/Resource.h"

#include "rendering/Renderer.h"

#include "scene/Scene.h"

#include "Input.h"
#include "JobSystem.h"
#include "Logger.h"
#include "OpenXR.h"
#include "Time.h"
#include "Window.h"

class EngineSettings
{
	public:
	EngineSettings (std::filesystem::path file_name);
	void load ();
	void save ();

	int screenWidth = 800;
	int screenHeight = 600;
	bool isFullscreen = false;

	bool useValidationLayers = true;

	bool isFrameCapped = true;
	double MaxFPS = 100.0f;

	private:
	std::filesystem::path file_name;
};

struct StaticInitializer
{
	StaticInitializer ();
	~StaticInitializer ();
};

class Engine
{
	public:
	Engine ();
	~Engine ();
	Engine (Engine const& app) = delete;
	Engine& operator= (Engine const& app) = delete;
	Engine (Engine&& app) = delete;
	Engine& operator= (Engine&& app) = delete;

	void set_imgui_update_callback (std::function<void ()> cb) { imgui_update_callback = cb; }
	void set_imgui_draw_callback (std::function<void ()> cb) { imgui_draw_callback = cb; }

	void run ();
	void process_inputs ();

	EngineSettings settings;

	StaticInitializer static_initializer;

	job::ThreadPool thread_pool;

	Time time;
	Window window;
	Input::InputDirector input;
	Resource::Resources resources;

	VulkanRenderer vulkan_renderer;
	Scene scene;

	std::optional<OpenXR> openxr;

	private:
	std::function<void ()> imgui_update_callback;
	std::function<void ()> imgui_draw_callback;

	void build_imgui ();

	// ImGui resources
	SimpleTimer imGuiTimer;
};
