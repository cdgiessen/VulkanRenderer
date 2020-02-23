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
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


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
: static_initializer (),
  settings ("settings.json"),
  window (input,
      settings.isFullscreen,
      "Vulkan Renderer",
      cml::vec2i (settings.screenWidth, settings.screenHeight),
      cml ::vec2i (10, 10)),
  input (window),
  resource_manager (task_manager),
  vulkan_renderer (settings.useValidationLayers, task_manager, window, resource_manager),
  scene (task_manager, time_manager, resource_manager, vulkan_renderer)
// imgui_nodeGraph_terrain ()
{
	input.SetMouseControlStatus (false);

	Log.Debug (fmt::format ("Hardware Threads Available = {}", HardwareThreadCount ()));
}


Engine::~Engine () {}

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

		time_manager.StartFrameTimer ();
		input.UpdateInputs ();
		HandleInputs ();
		scene.Update ();

		BuildImgui ();
		vulkan_renderer.RenderFrame ();
		input.ResetReleasedInput ();

		if (settings.isFrameCapped)
		{
			if (time_manager.ExactTimeSinceFrameStart () < 1.0 / settings.MaxFPS)
			{
				std::this_thread::sleep_for (std::chrono::duration<double> (
				    1.0 / settings.MaxFPS - time_manager.ExactTimeSinceFrameStart () -
				    (1.0 / settings.MaxFPS) / 10.0));
			}
		}
		time_manager.EndFrameTimer ();
	}

	vulkan_renderer.DeviceWaitTillIdle ();
	task_manager.Stop ();
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
	double deltaTime = time_manager.DeltaTime ();

	imgui_update_callback ();

	if (!input.GetTextInputMode ())
	{

		// if (input.IsJoystickConnected (0))
		// {
		// 	scene.GetCamera ()->ProcessJoystickMove (input.GetControllerAxis (0, 1),
		// 	    input.GetControllerAxis (0, 0),
		// 	    (input.GetControllerAxis (0, 4) + 1) / 2.0,
		// 	    (input.GetControllerAxis (0, 5) + 1) / 2.0,
		// 	    deltaTime);
		// 	scene.GetCamera ()->ProcessJoystickLook (
		// 	    input.GetControllerAxis (0, 3), input.GetControllerAxis (0, 4), deltaTime);

		// 	if (input.GetControllerButton (0, 2))
		// 		scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::UP, deltaTime);
		// 	if (input.GetControllerButton (0, 5))
		// 		scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::DOWN, deltaTime);
		// }

		// if (input.GetKey (Input::KeyCode::W))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::FORWARD, deltaTime);
		// if (input.GetKey (Input::KeyCode::S))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::BACKWARD, deltaTime);
		// if (input.GetKey (Input::KeyCode::A))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::LEFT, deltaTime);
		// if (input.GetKey (Input::KeyCode::D))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::RIGHT, deltaTime);
		// if (!scene.walkOnGround)
		// {
		// 	if (input.GetKey (Input::KeyCode::SPACE))
		// 		scene.GetCamera ()->ProcessKeyboard (Camera_Movement::UP, deltaTime);
		// 	if (input.GetKey (Input::KeyCode::LEFT_SHIFT))
		// 		scene.GetCamera ()->ProcessKeyboard (Camera_Movement::DOWN, deltaTime);
		// }

		if (input.GetKeyDown (Input::KeyCode::ESCAPE)) window.SetWindowToClose ();
		if (input.GetKeyDown (Input::KeyCode::ENTER))
			input.SetMouseControlStatus (!input.GetMouseControlStatus ());

		// if (input.GetKey (Input::KeyCode::E))
		// 	scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::UP, deltaTime);
		// if (input.GetKey (Input::KeyCode::Q))
		// 	scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::DOWN, deltaTime);

		// if (input.GetKeyDown (Input::KeyCode::N)) scene.drawNormals = !scene.drawNormals;

		if (input.GetKeyDown (Input::KeyCode::X))
		{
			vulkan_renderer.ToggleWireframe ();
			Log.Debug ("Wireframe toggle");
		}

		if (input.GetKeyDown (Input::KeyCode::F))
		{
			// scene.walkOnGround = !scene.walkOnGround;
			Log.Debug ("flight mode toggled");
		}
	}
	else
	{
		if (input.GetKeyDown (Input::KeyCode::ESCAPE)) input.ResetTextInputMode ();
	}

	// if (input.GetMouseControlStatus ())
	// {
	// 	scene.GetCamera ()->ProcessMouseMovement (
	// 	    input.GetMouseChangeInPosition ().x, input.GetMouseChangeInPosition ().y);
	// 	scene.GetCamera ()->ProcessMouseScroll (input.GetMouseScrollY (), deltaTime);
	// }

	if (input.GetMouseButtonPressed (input.GetMouseButtonPressed (0)))
	{
		if (!ImGui::IsWindowHovered (ImGuiHoveredFlags_AnyWindow))
		{
			input.SetMouseControlStatus (true);
		}
	}
}
