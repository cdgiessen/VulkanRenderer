#include "Engine.h"

#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "imgui.hpp"
#include "rendering/backend/ImGuiImpl.h"

#include "cml/cml.h"

#include <nlohmann/json.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

EngineSettings::EngineSettings (std::filesystem::path fileName) : fileName (fileName) { Load (); }

void EngineSettings::Load ()
{
	if (std::filesystem::exists (fileName))
	{
		std::ifstream input{ fileName.string () };
		nlohmann::json settings{};
		input >> settings;

		screenWidth = settings["initial-screen-size"]["width"];
		screenHeight = settings["initial-screen-size"]["height"];

		useValidationLayers = settings["use-validation-layers"];

		isFullscreen = settings["fullscreen"];

		isFrameCapped = settings["is-frame-rate-capped"];
		MaxFPS = settings["max-fps"];
	}
	else
	{
		Log.Debug ("Settings file didn't exist, creating one");
		Save ();
	}
}

void EngineSettings::Save ()
{
	nlohmann::json j;

	j["initial-screen-size"]["width"] = screenWidth;
	j["initial-screen-size"]["height"] = screenHeight;

	j["use-validation-layers"] = useValidationLayers;

	j["fullscreen"] = isFullscreen;

	j["is-frame-rate-capped"] = isFrameCapped;
	j["max-fps"] = MaxFPS;

	std::ofstream outFile (fileName);
	outFile << std::setw (4) << j;
	outFile.close ();
}

StaticInitializer::StaticInitializer () { InitializeGLFW (); }
StaticInitializer::~StaticInitializer () { TerminateGLFW (); }

Engine::Engine ()
: settings ("settings.json"),
  static_initializer (),
  window (input,
      settings.isFullscreen,
      "Vulkan Renderer",
      cml::vec2i (settings.screenWidth, settings.screenHeight),
      cml ::vec2i (10, 10)),
  input (window),
  resources (thread_pool),
  vulkan_renderer (settings.useValidationLayers, thread_pool, window, resources),
  scene (thread_pool, time, input, resources, vulkan_renderer)

{
	input.SetMouseControlStatus (false);

	Log.Debug (fmt::format ("Hardware Threads Available = {}", HardwareThreadCount ()));

	openxr = create_openxr (vulkan_renderer.GetOpenXRInit ());

	if (openxr)
	{
		XrSessionBeginInfo begin_info{ XR_TYPE_SESSION_BEGIN_INFO };
		begin_info.primaryViewConfigurationType = view_config_type;
		xrBeginSession (openxr->session, &begin_info);
	}
}


Engine::~Engine ()
{
	if (openxr)
	{
		xrEndSession (openxr->session);
		destroy_openxr (*openxr);
	}
}

void Engine::Run ()
{

	while (!window.CheckForWindowClose ())
	{
		if (window.CheckForWindowResizing ())
		{
			if (!window.CheckForWindowIconified ())
			{
				vulkan_renderer.RecreateSwapChain ();
				window.SetWindowResizeDone ();
			}
		}

		time.StartFrameTimer ();
		input.UpdateInputs ();
		HandleInputs ();
		scene.Update ();

		BuildImgui ();
		vulkan_renderer.RenderFrame ();
		input.ResetReleasedInput ();

		if (settings.isFrameCapped)
		{
			if (time.ExactTimeSinceFrameStart () < 1.0 / settings.MaxFPS)
			{
				std::this_thread::sleep_for (std::chrono::duration<double> (
				    1.0 / settings.MaxFPS - time.ExactTimeSinceFrameStart () - (1.0 / settings.MaxFPS) / 10.0));
			}
		}
		time.EndFrameTimer ();
	}

	vulkan_renderer.DeviceWaitTillIdle ();
	thread_pool.Stop ();
}

// Build imGui windows and elements
void Engine::BuildImgui ()
{

	imGuiTimer.StartTimer ();

	vulkan_renderer.ImGuiNewFrame ();
	imgui_draw_callback ();

	imGuiTimer.EndTimer ();
	// Log.Debug << imGuiTimer.GetElapsedTimeNanoSeconds();
}

void Engine::HandleInputs ()
{
	double deltaTime = time.DeltaTime ();

	imgui_update_callback ();
	if (input.GetKeyDown (Input::KeyCode::ESCAPE)) window.SetWindowToClose ();

	if (!input.GetTextInputMode ())
	{
		if (input.GetKeyDown (Input::KeyCode::ENTER))
			input.SetMouseControlStatus (!input.GetMouseControlStatus ());




		// if (input.GetKeyDown (Input::KeyCode::N)) scene.drawNormals = !scene.drawNormals;

		// if (input.GetKeyDown (Input::KeyCode::X))
		// {
		// 	vulkan_renderer.ToggleWireframe ();
		// 	Log.Debug ("Wireframe toggle");
		// }

		// if (input.GetKeyDown (Input::KeyCode::F))
		// {
		// 	scene.walkOnGround = !scene.walkOnGround;
		// 	Log.Debug ("flight mode toggled");
		// }
	}
	else
	{
		if (input.GetKeyDown (Input::KeyCode::ESCAPE)) input.ResetTextInputMode ();
	}

	if (input.GetMouseButtonPressed (input.GetMouseButtonPressed (0)))
	{
		if (!ImGui::IsWindowHovered (ImGuiHoveredFlags_AnyWindow))
		{
			input.SetMouseControlStatus (true);
		}
	}
}
