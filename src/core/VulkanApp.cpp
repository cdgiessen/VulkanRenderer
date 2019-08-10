#include "VulkanApp.h"

#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "ImGui/imgui.h"

#include "cml/cml.h"

#include <nlohmann/json.hpp>

#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"


VulkanAppSettings::VulkanAppSettings (std::string fileName) : fileName (fileName) { Load (); }

void VulkanAppSettings::Load ()
{
	if (fileExists (fileName))
	{
		std::ifstream input{ fileName };
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
		Log.Debug (fmt::format ("Settings file didn't exist, creating one\n"));
		Save ();
	}
}

void VulkanAppSettings::Save ()
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

unsigned int HardwareThreadCount ()
{
	unsigned int concurentThreadsSupported = std::thread::hardware_concurrency ();
	Log.Debug (fmt::format ("Hardware Threads Available = {}\n", concurentThreadsSupported));
	return concurentThreadsSupported > 0 ? concurentThreadsSupported : 1;
}

VulkanApp::VulkanApp ()
: settings ("settings.json"),
  workerPool (taskManager, HardwareThreadCount ()),
  timeManager (),
  window (settings.isFullscreen, cml::vec2i (settings.screenWidth, settings.screenHeight), cml ::vec2i (10, 10)),
  resourceManager (),
  vulkanRenderer (settings.useValidationLayers, window, resourceManager),
  imgui_nodeGraph_terrain (),
  scene (resourceManager, vulkanRenderer, timeManager, imgui_nodeGraph_terrain.GetGraph ())
{

	/*timeManager = std::make_unique<TimeManager>();

	window = std::make_unique<Window>(settings.isFullscreen,
	    cml:vec3(settings.screenWidth, settings.screenHeight),
	    cml:vec3(10, 10));*/
	Input::SetupInputDirector (&window);

	/*resourceManager = std::make_unique<ResourceManager>();


	vulkanRenderer = std::make_unique<VulkanRenderer>(settings.useValidationLayers, window.get());

	scene = std::make_unique<Scene>(*resourceManager.get(), *vulkanRenderer.get(), *timeManager.get(), imgui_nodeGraph_terrain.GetGraph());
*/
	vulkanRenderer.scene = &scene;
}


VulkanApp::~VulkanApp () { workerPool.StopWorkers (); }

void VulkanApp::Run ()
{

	while (!window.CheckForWindowClose ())
	{
		if (window.CheckForWindowResizing ())
		{
			if (!window.CheckForWindowIconified ())
			{
				RecreateSwapChain ();
				window.SetWindowResizeDone ();
			}
		}

		timeManager.StartFrameTimer ();
		Input::inputDirector.UpdateInputs ();
		HandleInputs ();
		scene.UpdateScene ();
		BuildImgui ();
		vulkanRenderer.RenderFrame ();
		Input::inputDirector.ResetReleasedInput ();

		if (settings.isFrameCapped)
		{
			if (timeManager.ExactTimeSinceFrameStart () < 1.0 / settings.MaxFPS)
			{
				std::this_thread::sleep_for (std::chrono::duration<double> (
				    1.0 / settings.MaxFPS - timeManager.ExactTimeSinceFrameStart () -
				    (1.0 / settings.MaxFPS) / 10.0));
			}
		}
		timeManager.EndFrameTimer ();
	}

	vulkanRenderer.DeviceWaitTillIdle ();
}

void VulkanApp::RecreateSwapChain ()
{
	vulkanRenderer.DeviceWaitTillIdle ();

	vulkanRenderer.RecreateSwapChain ();
}

void VulkanApp::DebugOverlay (bool* show_debug_overlay)
{

	static bool verbose = false;
	ImGui::SetNextWindowPos (ImVec2 (0, 0));
	if (!ImGui::Begin ("Debug Stats",
	        show_debug_overlay,
	        ImVec2 (0, 0),
	        0.3f,
	        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	            ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End ();
		return;
	}
	ImGui::Text ("FPS %.3f", ImGui::GetIO ().Framerate);
	ImGui::Text ("DeltaT: %f(s)", timeManager.DeltaTime ());
	if (ImGui::Button ("Toggle Verbose"))
	{
		verbose = !verbose;
	}
	if (verbose) ImGui::Text ("Run Time: %f(s)", timeManager.RunningTime ());
	if (verbose) ImGui::Text ("Last frame time%f(s)", timeManager.PreviousFrameTime ());
	if (verbose) ImGui::Text ("Last frame time%f(s)", timeManager.PreviousFrameTime ());
	ImGui::Separator ();
	ImGui::Text ("Mouse Position: (%.1f,%.1f)", ImGui::GetIO ().MousePos.x, ImGui::GetIO ().MousePos.y);
	ImGui::End ();
}

void VulkanApp::CameraWindow (bool* show_camera_window)
{
	ImGui::SetNextWindowPos (ImVec2 (0, 100), ImGuiSetCond_FirstUseEver);

	if (!ImGui::Begin ("Camera Window", show_camera_window))
	{
		ImGui::End ();
		return;
	};
	ImGui::Text ("Camera");
	// auto sPos = "Pos " + std::to_string (scene.GetCamera ()->Position.x);
	// auto sDir = "Dir " + std::to_string (scene.GetCamera ()->Front.x);
	// auto sSpeed = "Speed " + std::to_string (scene.GetCamera ()->MovementSpeed);
	// ImGui::Text ("%s", sPos.c_str ());
	// ImGui::Text ("%s", sDir.c_str ());
	// ImGui::Text ("%s", sSpeed.c_str ());

	ImGui::DragFloat3 ("Pos", &scene.GetCamera ()->Position.x, 2);
	ImGui::DragFloat3 ("Rot", &scene.GetCamera ()->Front.x, 2);
	ImGui::Text ("Camera Movement Speed");
	ImGui::Text ("%f", scene.GetCamera ()->MovementSpeed);
	ImGui::SliderFloat ("##camMovSpeed", &(scene.GetCamera ()->MovementSpeed), 0.1f, 100.0f);
	ImGui::End ();
}

void VulkanApp::ControlsWindow (bool* show_controls_window)
{
	return;
	ImGui::SetNextWindowPos (ImVec2 (0, 250), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin ("Controls", show_controls_window))
	{
		ImGui::Text ("Horizontal Movement: WASD");
		ImGui::Text ("Vertical Movement: Space/Shift");
		ImGui::Text ("Looking: Mouse");
		ImGui::Text ("Change Move Speed: E/Q");
		ImGui::Text ("Unlock Mouse: Enter");
		ImGui::Text ("Show Wireframe: X");
		ImGui::Text ("Toggle Flying: F");
		ImGui::Text ("Hide Gui: H");
		// ImGui::Text("Toggle Fullscreen: G");
		ImGui::Text ("Exit: Escape");
	}
	ImGui::End ();
}

void VulkanApp::ControllerWindow (bool* show_controller_window)
{


	if (ImGui::Begin ("Controller View", show_controller_window))
	{
		for (int i = 0; i < 16; i++)
		{
			if (Input::IsJoystickConnected (i))
			{

				ImGui::BeginGroup ();
				for (int j = 0; j < 6; j++)
				{
					ImGui::Text ("%f", Input::GetControllerAxis (i, j));
				}
				ImGui::EndGroup ();
				ImGui::SameLine ();
				ImGui::BeginGroup ();
				for (int j = 0; j < 14; j++)
				{
					Input::GetControllerButton (i, j) ? ImGui::Text ("true") : ImGui::Text ("false");
				}
				ImGui::EndGroup ();
			}
		}
	}
	ImGui::End ();
}


// Build imGui windows and elements
void VulkanApp::BuildImgui ()
{

	imGuiTimer.StartTimer ();

	ImGui_ImplGlfwVulkan_NewFrame ();
	if (debug_mode && panels.showGui)
	{

		if (panels.debug_overlay) DebugOverlay (&panels.debug_overlay);
		if (panels.camera_controls) CameraWindow (&panels.camera_controls);
		if (panels.controls_list) ControlsWindow (&panels.controls_list);


		scene.UpdateSceneGUI ();

		imgui_nodeGraph_terrain.Draw ();

		ControllerWindow (&panels.controller_list);
	}
	imGuiTimer.EndTimer ();
	// Log::Debug << imGuiTimer.GetElapsedTimeNanoSeconds() << "\n";
}

void VulkanApp::HandleInputs ()
{
	double deltaTime = timeManager.DeltaTime ();

	if (!Input::GetTextInputMode ())
	{

		if (Input::IsJoystickConnected (0))
		{
			scene.GetCamera ()->ProcessJoystickMove (Input::GetControllerAxis (0, 1),
			    Input::GetControllerAxis (0, 0),
			    (Input::GetControllerAxis (0, 4) + 1) / 2.0,
			    (Input::GetControllerAxis (0, 5) + 1) / 2.0,
			    deltaTime);
			scene.GetCamera ()->ProcessJoystickLook (
			    Input::GetControllerAxis (0, 3), Input::GetControllerAxis (0, 4), deltaTime);

			if (Input::GetControllerButton (0, 2))
				scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::UP, deltaTime);
			if (Input::GetControllerButton (0, 5))
				scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::DOWN, deltaTime);
		}

		if (Input::GetKey (Input::KeyCode::W))
			scene.GetCamera ()->ProcessKeyboard (Camera_Movement::FORWARD, deltaTime);
		if (Input::GetKey (Input::KeyCode::S))
			scene.GetCamera ()->ProcessKeyboard (Camera_Movement::BACKWARD, deltaTime);
		if (Input::GetKey (Input::KeyCode::A))
			scene.GetCamera ()->ProcessKeyboard (Camera_Movement::LEFT, deltaTime);
		if (Input::GetKey (Input::KeyCode::D))
			scene.GetCamera ()->ProcessKeyboard (Camera_Movement::RIGHT, deltaTime);
		if (!scene.walkOnGround)
		{
			if (Input::GetKey (Input::KeyCode::SPACE))
				scene.GetCamera ()->ProcessKeyboard (Camera_Movement::UP, deltaTime);
			if (Input::GetKey (Input::KeyCode::LEFT_SHIFT))
				scene.GetCamera ()->ProcessKeyboard (Camera_Movement::DOWN, deltaTime);
		}

		if (Input::GetKeyDown (Input::KeyCode::ESCAPE)) window.SetWindowToClose ();
		if (Input::GetKeyDown (Input::KeyCode::ENTER))
			Input::SetMouseControlStatus (!Input::GetMouseControlStatus ());

		if (Input::GetKey (Input::KeyCode::E))
			scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::UP, deltaTime);
		if (Input::GetKey (Input::KeyCode::Q))
			scene.GetCamera ()->ChangeCameraSpeed (Camera_Movement::DOWN, deltaTime);

		if (Input::GetKeyDown (Input::KeyCode::N)) scene.drawNormals = !scene.drawNormals;

		if (Input::GetKeyDown (Input::KeyCode::X))
		{
			vulkanRenderer.ToggleWireframe ();
			Log.Debug (fmt::format ("Wireframe toggle"));
		}

		if (Input::GetKeyDown (Input::KeyCode::F))
		{
			scene.walkOnGround = !scene.walkOnGround;
			Log.Debug (fmt::format ("flight mode toggled"));
		}

		if (Input::GetKeyDown (Input::KeyCode::H))
		{
			Log.Debug (fmt::format ("gui visibility toggled\n"));
			panels.showGui = !panels.showGui;
		}
	}
	else
	{
		if (Input::GetKeyDown (Input::KeyCode::ESCAPE)) Input::ResetTextInputMode ();
	}

	if (Input::GetKey (Input::KeyCode::C))
	{
		tempCameraSpeed *= 1.02f;
	}
	if (Input::GetKey (Input::KeyCode::V))
	{
		tempCameraSpeed *= 0.98f;
	}

	if (Input::GetMouseControlStatus ())
	{
		scene.GetCamera ()->ProcessMouseMovement (
		    tempCameraSpeed + Input::GetMouseChangeInPosition ().x,
		    Input::GetMouseChangeInPosition ().y);
		scene.GetCamera ()->ProcessMouseScroll (Input::GetMouseScrollY (), deltaTime);
	}

	if (Input::GetMouseButtonPressed (Input::GetMouseButtonPressed (0)))
	{
		if (!ImGui::IsMouseHoveringAnyWindow ())
		{
			Input::SetMouseControlStatus (true);
		}
	}
}


int run_engine ()
{
	VulkanApp* vkApp;
	try
	{
		vkApp = new VulkanApp ();
	}
	catch (const std::runtime_error& e)
	{
		Log.Debug (fmt::format ("ENGINE FAILED TO INITIALIZE\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}

	try
	{
		vkApp->Run ();
	}
	catch (const std::runtime_error& e)
	{
		Log.Error (fmt::format ("Engine quite in main loop\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}

	try
	{
		delete (vkApp);
	}
	catch (const std::runtime_error& e)
	{
		Log.Error (fmt::format ("Engine quite in destructor\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}
}