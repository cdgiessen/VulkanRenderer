#include "VulkanApp.h"

#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <string>
#include <array>
#include <set>

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include "../../third-party/ImGui/imgui.h"

#include "../../third-party/json/json.hpp"

#include "../rendering/Initializers.hpp"

VulkanApp::VulkanApp()
{
	ReadSettings();

	timeManager = std::make_shared<TimeManager>();

	window = std::make_shared<Window>();
	window->createWindow(glm::uvec2(screenWidth, screenHeight));
	SetMouseControl(true);

	resourceManager = std::make_shared<ResourceManager>();

	scene = std::make_shared<Scene>();

	vulkanRenderer = std::make_shared<VulkanRenderer>(useValidationLayers, scene);
	vulkanRenderer->InitVulkanRenderer(window->getWindowContext());
	vulkanRenderer->CreateSemaphores();

	scene->PrepareScene(resourceManager, vulkanRenderer, imgui_nodeGraph_terrain.GetGraph());

	PrepareImGui(window, vulkanRenderer);
}


VulkanApp::~VulkanApp()
{
}


void VulkanApp::clean() {
	CleanUpImgui();

	scene->CleanUpScene();

	vulkanRenderer->CleanVulkanResources();

	window->destroyWindow();
	glfwTerminate();

}

void VulkanApp::mainLoop() {

	while (!window->CheckForWindowClose()) {
		timeManager->StartFrameTimer();

		if (window->CheckForWindowResizing()) {
			RecreateSwapChain();
			window->SetWindowResizeDone();
		}

		Input::inputDirector.UpdateInputs();
		HandleInputs();

		//updateScene();
		scene->UpdateScene(resourceManager, timeManager);
		BuildImgui();

		vulkanRenderer->RenderFrame();

		Input::inputDirector.ResetReleasedInput();
		timeManager->EndFrameTimer();
		//Log::Debug << "main loop breaker. Break me if you want to stop after every frame!\n";
	}

	vkDeviceWaitIdle(vulkanRenderer->device.device);

}

void VulkanApp::ReadSettings() {

	std::ifstream input("settings.json");
	nlohmann::json settings;
	input >> settings;

	screenWidth = settings["initial-screen-size"]["width"];
	screenHeight = settings["initial-screen-size"]["height"];

	useValidationLayers = settings["use-validation-layers"];
}

void VulkanApp::RecreateSwapChain() {
	vkDeviceWaitIdle(vulkanRenderer->device.device);

	vulkanRenderer->RecreateSwapChain();
}

void VulkanApp::DebugOverlay(bool* show_debug_overlay) {

	static bool verbose = false;
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	if (!ImGui::Begin("Debug Stats", show_debug_overlay, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End();
		return;
	}
	ImGui::Text("FPS %.3f", ImGui::GetIO().Framerate);
	ImGui::Text("DeltaT: %f(s)", timeManager->GetDeltaTime());
	if (ImGui::Button("Toggle Verbose")) {
		verbose = !verbose;
	}
	if (verbose) ImGui::Text("Run Time: %f(s)", timeManager->GetRunningTime());
	if (verbose) ImGui::Text("Last frame time%f(s)", timeManager->GetPreviousFrameTime());
	if (verbose) ImGui::Text("Last frame time%f(s)", timeManager->GetPreviousFrameTime());
	ImGui::Separator();
	ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
	ImGui::End();

}

void VulkanApp::CameraWindow(bool* show_camera_window) {
	ImGui::SetNextWindowPos(ImVec2(0, 100), ImGuiSetCond_FirstUseEver);

	if (!ImGui::Begin("Camera Window", show_camera_window))
	{
		ImGui::End();
		return;
	};
	ImGui::Text("Camera");
	ImGui::DragFloat3("Pos", &scene->GetCamera()->Position.x, 2);
	ImGui::DragFloat3("Rot", &scene->GetCamera()->Front.x, 2);
	ImGui::Text("Camera Movement Speed");
	ImGui::Text(std::to_string(scene->GetCamera()->MovementSpeed).c_str());
	ImGui::SliderFloat("##camMovSpeed", &(scene->GetCamera()->MovementSpeed), 0.1f, 100.0f);
	ImGui::End();
}

void VulkanApp::ControlsWindow(bool* show_controls_window) {
	ImGui::SetNextWindowPos(ImVec2(0, 250), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin("Controls", show_controls_window)) {
		ImGui::Text("Horizontal Movement: WASD");
		ImGui::Text("Vertical Movement: Space/Shift");
		ImGui::Text("Looking: Mouse");
		ImGui::Text("Change Move Speed: E/Q");
		ImGui::Text("Unlock Mouse: Enter");
		ImGui::Text("Show Wireframe: X");
		//ImGui::Text("Show Normals: N");
		ImGui::Text("Toggle Flying: F");
		ImGui::Text("Screenshot: F10 - EXPERIMENTAL!");
		ImGui::Text("Hide Gui: H");
		ImGui::Text("Exit: Escape");
	}
	ImGui::End();

}

// Build imGui windows and elements
void VulkanApp::BuildImgui() {

	imGuiTimer.StartTimer();

	ImGui_ImplGlfwVulkan_NewFrame();
	if (showGui) {

		bool show_camera_window = true;
		bool show_log_window = true;
		bool show_debug_overlay = true;
		bool show_controls_list = true;

		if (show_debug_overlay) DebugOverlay(&show_debug_overlay);
		if (show_camera_window) CameraWindow(&show_camera_window);
		if(show_controls_list) ControlsWindow(&show_controls_list);


		scene->UpdateSceneGUI();

		if (show_log_window) {
			appLog.Draw("Example: Log", &show_log_window);
		}

		imgui_nodeGraph_terrain.Draw();

	}
	imGuiTimer.EndTimer();
	//Log::Debug << imGuiTimer.GetElapsedTimeNanoSeconds() << "\n";


}

//Release associated resources and shutdown imgui
void VulkanApp::CleanUpImgui() {
	ImGui_ImplGlfwVulkan_Shutdown();
	//vkDestroyDescriptorPool(vulkanRenderer->device.device, imgui_descriptor_pool, VK_NULL_HANDLE);
}

void VulkanApp::HandleInputs() {
	//Log::Debug << camera->Position.x << " " << camera->Position.y << " " << camera->Position.z << "\n";

	if (!Input::GetTextInputMode()) {

		if (Input::GetKey(Input::KeyCode::W))
			scene->GetCamera()->ProcessKeyboard(Camera_Movement::FORWARD, (float)timeManager->GetDeltaTime());
		if (Input::GetKey(Input::KeyCode::S))
			scene->GetCamera()->ProcessKeyboard(Camera_Movement::BACKWARD, (float)timeManager->GetDeltaTime());
		if (Input::GetKey(Input::KeyCode::A))
			scene->GetCamera()->ProcessKeyboard(Camera_Movement::LEFT, (float)timeManager->GetDeltaTime());
		if (Input::GetKey(Input::KeyCode::D))
			scene->GetCamera()->ProcessKeyboard(Camera_Movement::RIGHT, (float)timeManager->GetDeltaTime());
		if (!scene->walkOnGround) {
			if (Input::GetKey(Input::KeyCode::SPACE))
				scene->GetCamera()->ProcessKeyboard(Camera_Movement::UP, (float)timeManager->GetDeltaTime());
			if (Input::GetKey(Input::KeyCode::LEFT_SHIFT))
				scene->GetCamera()->ProcessKeyboard(Camera_Movement::DOWN, (float)timeManager->GetDeltaTime());
		}

		if (Input::GetKeyDown(Input::KeyCode::ESCAPE))
			window->SetWindowToClose();
		if (Input::GetKeyDown(Input::KeyCode::ENTER))
			SetMouseControl(!mouseControlEnabled);

		if (Input::GetKey(Input::KeyCode::E))
			scene->GetCamera()->ChangeCameraSpeed(Camera_Movement::UP, (float)timeManager->GetDeltaTime());
		if (Input::GetKey(Input::KeyCode::Q))
			scene->GetCamera()->ChangeCameraSpeed(Camera_Movement::DOWN, (float)timeManager->GetDeltaTime());

		if (Input::GetKeyDown(Input::KeyCode::N))
			scene->drawNormals = !scene->drawNormals;

		if (Input::GetKeyDown(Input::KeyCode::X)) {
			wireframe = !wireframe;
			vulkanRenderer->SetWireframe(wireframe);
			Log::Debug << "wireframe toggled" << "\n";
		}

		if (Input::GetKeyDown(Input::KeyCode::F)) {
			scene->walkOnGround = !scene->walkOnGround;
			Log::Debug << "flight mode toggled " << "\n";
		}

		if (Input::GetKeyDown(Input::KeyCode::H)) {
			Log::Debug << "gui visibility toggled " << "\n";
			showGui = !showGui;
		}

		if (Input::GetKeyDown(Input::KeyCode::F10)) {
			Log::Debug << "screenshot taken " << "\n";
			vulkanRenderer->SaveScreenshotNextFrame();
		}
	}
	else {
		if (Input::GetKeyDown(Input::KeyCode::ESCAPE))
			Input::ResetTextInputMode();
	}

	if (mouseControlEnabled) {
		scene->GetCamera()->ProcessMouseMovement(Input::GetMouseChangeInPosition().x, Input::GetMouseChangeInPosition().y);
		scene->GetCamera()->ProcessMouseScroll(Input::GetMouseScrollY());
	}

	if (Input::GetMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		if (!ImGui::IsMouseHoveringAnyWindow()) {
			SetMouseControl(true);
		}
	}
}

void VulkanApp::SetMouseControl(bool value) {
	mouseControlEnabled = value;
	if (mouseControlEnabled)
		glfwSetInputMode(window->getWindowContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else
		glfwSetInputMode(window->getWindowContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
