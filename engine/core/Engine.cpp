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
		Log.Debug ("Settings file didn't exist, creating one\n");
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

Engine::Engine ()
: settings ("settings.json"),
  window (settings.isFullscreen, cml::vec2i (settings.screenWidth, settings.screenHeight), cml ::vec2i (10, 10)),
  resource_manager (task_manager),
  vulkan_renderer (settings.useValidationLayers, task_manager, window, resource_manager)
// imgui_nodeGraph_terrain ()
// scene (task_manager, resource_manager, vulkan_renderer, time_manager, imgui_nodeGraph_terrain.GetGraph ())
{
	Input::SetupInputDirector (&window);
	Input::SetMouseControlStatus (false);

	Log.Debug (fmt::format ("Hardware Threads Available = {}\n", HardwareThreadCount ()));
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
		Input::inputDirector.UpdateInputs ();
		HandleInputs ();
		// scene.UpdateScene ();

		BuildImgui ();
		vulkan_renderer.RenderFrame ();
		Input::inputDirector.ResetReleasedInput ();

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
	// Log.Debug << imGuiTimer.GetElapsedTimeNanoSeconds() << "\n";
}

void Engine::HandleInputs ()
{
	double deltaTime = time_manager.DeltaTime ();

	imgui_update_callback ();

	if (!Input::GetTextInputMode ())
	{

		// if (Input::IsJoystickConnected (0))
		// {
		// 	scene.GetCamera ()->ProcessJoystickMove (Input::GetControllerAxis (0, 1),
		// 	    Input::GetControllerAxis (0, 0),
		// 	    (Input::GetControllerAxis (0, 4) + 1) / 2.0,
		// 	    (Input::GetControllerAxis (0, 5) + 1) / 2.0,
		// 	    deltaTime);
		// 	scene.GetCamera ()->ProcessJoystickLook (
		// 	    Input::GetControllerAxis (0, 3), Input::GetControllerAxis (0, 4), deltaTime);

		// 	if (Input::GetControllerButton (0, 2))
		// 		scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::UP, deltaTime);
		// 	if (Input::GetControllerButton (0, 5))
		// 		scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::DOWN, deltaTime);
		// }

		// if (Input::GetKey (Input::KeyCode::W))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::FORWARD, deltaTime);
		// if (Input::GetKey (Input::KeyCode::S))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::BACKWARD, deltaTime);
		// if (Input::GetKey (Input::KeyCode::A))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::LEFT, deltaTime);
		// if (Input::GetKey (Input::KeyCode::D))
		// 	scene.GetCamera ()->ProcessKeyboard (Camera_Movement::RIGHT, deltaTime);
		// if (!scene.walkOnGround)
		// {
		// 	if (Input::GetKey (Input::KeyCode::SPACE))
		// 		scene.GetCamera ()->ProcessKeyboard (Camera_Movement::UP, deltaTime);
		// 	if (Input::GetKey (Input::KeyCode::LEFT_SHIFT))
		// 		scene.GetCamera ()->ProcessKeyboard (Camera_Movement::DOWN, deltaTime);
		// }

		if (Input::GetKeyDown (Input::KeyCode::ESCAPE)) window.SetWindowToClose ();
		if (Input::GetKeyDown (Input::KeyCode::ENTER))
			Input::SetMouseControlStatus (!Input::GetMouseControlStatus ());

		// if (Input::GetKey (Input::KeyCode::E))
		// 	scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::UP, deltaTime);
		// if (Input::GetKey (Input::KeyCode::Q))
		// 	scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::DOWN, deltaTime);

		// if (Input::GetKeyDown (Input::KeyCode::N)) scene.drawNormals = !scene.drawNormals;

		if (Input::GetKeyDown (Input::KeyCode::X))
		{
			vulkan_renderer.ToggleWireframe ();
			Log.Debug ("Wireframe toggle");
		}

		if (Input::GetKeyDown (Input::KeyCode::F))
		{
			// scene.walkOnGround = !scene.walkOnGround;
			Log.Debug ("flight mode toggled\n");
		}
	}
	else
	{
		if (Input::GetKeyDown (Input::KeyCode::ESCAPE)) Input::ResetTextInputMode ();
	}

	// if (Input::GetMouseControlStatus ())
	// {
	// 	scene.GetCamera ()->ProcessMouseMovement (
	// 	    Input::GetMouseChangeInPosition ().x, Input::GetMouseChangeInPosition ().y);
	// 	scene.GetCamera ()->ProcessMouseScroll (Input::GetMouseScrollY (), deltaTime);
	// }

	if (Input::GetMouseButtonPressed (Input::GetMouseButtonPressed (0)))
	{
		if (!ImGui::IsWindowHovered (ImGuiHoveredFlags_AnyWindow))
		{
			Input::SetMouseControlStatus (true);
		}
	}
}
