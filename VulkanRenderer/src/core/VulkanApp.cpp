#include "VulkanApp.h"

#include "..\vulkan\VulkanInitializers.hpp"

VulkanApp::VulkanApp()
{
	timeManager = std::make_shared<TimeManager>();

	window = std::make_shared<Window>();
	window->createWindow(glm::uvec2(WIDTH, HEIGHT));
	SetMouseControl(true);

	resourceManager = std::make_shared<ResourceManager>();

	scene = std::make_shared<Scene>();
	vulkanRenderer = std::make_shared<VulkanRenderer>(scene);
	
	vulkanRenderer->InitVulkanRenderer(window->getWindowContext());
	vulkanRenderer->CreateSemaphores();
	scene->PrepareScene(resourceManager, vulkanRenderer);

	PrepareImGui();


	mainLoop();

	clean();
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

		InputDirector::GetInstance().UpdateInputs();
		HandleInputs();

		//updateScene();
		scene->UpdateScene(resourceManager, timeManager);
		BuildImgui();

		vulkanRenderer->RenderFrame();
		
		InputDirector::GetInstance().ResetReleasedInput();
		timeManager->EndFrameTimer();
		//std::cout << "main loop breaker. Break me if you want to stop after every frame!" << std::endl;
	}

	vkDeviceWaitIdle(vulkanRenderer->device.device);

}

void VulkanApp::RecreateSwapChain() {
	vkDeviceWaitIdle(vulkanRenderer->device.device);

	vulkanRenderer->RecreateSwapChain();
}

static void imgui_check_vk_result(VkResult err)
{
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

void  VulkanApp::PrepareImGui()
{
	//Creates a descriptor pool for imgui
	{	VkDescriptorPoolSize pool_size[11] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * 11;
		pool_info.poolSizeCount = 11;
		pool_info.pPoolSizes = pool_size;
		VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanRenderer->device.device, &pool_info, VK_NULL_HANDLE, &imgui_descriptor_pool));
	}

	ImGui_ImplGlfwVulkan_Init_Data init_data = {};
	init_data.allocator = VK_NULL_HANDLE;
	init_data.gpu = vulkanRenderer->device.physical_device;
	init_data.device = vulkanRenderer->device.device;
	init_data.render_pass = vulkanRenderer->renderPass;
	init_data.pipeline_cache = VK_NULL_HANDLE;
	init_data.descriptor_pool = imgui_descriptor_pool;
	init_data.check_vk_result = imgui_check_vk_result;
	
	ImGui_ImplGlfwVulkan_Init(window->getWindowContext(), false, &init_data);

	VkCommandBuffer fontUploader = vulkanRenderer->device.createCommandBuffer(vulkanRenderer->device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	ImGui_ImplGlfwVulkan_CreateFontsTexture(fontUploader);
	vulkanRenderer->device.flushCommandBuffer(fontUploader, vulkanRenderer->device.graphics_queue, true);
}

void VulkanApp::DebugOverlay(bool* show_debug_overlay) {
	{
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
}

void VulkanApp::CameraWindow(bool* show_camera_window) {
	ImGui::Begin("Camera Window", show_camera_window);
	ImGui::Text("Camera");
	ImGui::DragFloat3("Position", &scene->GetCamera()->Position.x, 2);
	ImGui::DragFloat3("Rotation", &scene->GetCamera()->Front.x, 2);
	ImGui::Text("Camera Movement Speed");
	ImGui::SliderFloat("Speed", &scene->GetCamera()->MovementSpeed, 0.1f, 100.0f);
	//ImGui::ColorEdit3("clear color", (float*)&clear_color);
	//if (ImGui::Button("Test Window")) show_test_window ^= 1;
	//if (ImGui::Button("Another Window")) show_another_window ^= 1;
	//ImGui::Text("Frame index (of swap chain) : %u", (frameIndex));
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

		if (show_debug_overlay) DebugOverlay(&show_debug_overlay);
		if (show_camera_window) CameraWindow(&show_camera_window);

		scene->UpdateSceneGUI();

		{ //simple app log - taken from imgui exampels
			appLog.Draw("Example: Log", &show_log_window);
		}

		imgui_nodeGraph_terrain.Draw();

	}
	imGuiTimer.EndTimer();
	//std::cout << imGuiTimer.GetElapsedTimeNanoSeconds() << std::endl;

}

//Release associated resources and shutdown imgui
void VulkanApp::CleanUpImgui() {
	ImGui_ImplGlfwVulkan_Shutdown();
	vkDestroyDescriptorPool(vulkanRenderer->device.device, imgui_descriptor_pool, VK_NULL_HANDLE);
}

void VulkanApp::HandleInputs() {
	//std::cout << camera->Position.x << " " << camera->Position.y << " " << camera->Position.z << std::endl;

	if (Input::GetKey(GLFW_KEY_W))
		scene->GetCamera()->ProcessKeyboard(Camera_Movement::FORWARD, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_S))
		scene->GetCamera()->ProcessKeyboard(Camera_Movement::BACKWARD, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_A))
		scene->GetCamera()->ProcessKeyboard(Camera_Movement::LEFT, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_D))
		scene->GetCamera()->ProcessKeyboard(Camera_Movement::RIGHT, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_SPACE))
		scene->GetCamera()->ProcessKeyboard(Camera_Movement::UP, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_LEFT_SHIFT))
		scene->GetCamera()->ProcessKeyboard(Camera_Movement::DOWN, (float)timeManager->GetDeltaTime());

	if (Input::GetKeyDown(GLFW_KEY_0))
		appLog.AddLog("ZERO WAS HIT REPEAT ZERO WAS HIT\n");

	if (Input::GetKeyDown(GLFW_KEY_ESCAPE))
		window->SetWindowToClose();
	if (Input::GetKeyDown(GLFW_KEY_ENTER))
		SetMouseControl(!mouseControlEnabled);

	if (Input::GetKey(GLFW_KEY_E))
		scene->GetCamera()->ChangeCameraSpeed(Camera_Movement::UP, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_Q))
		scene->GetCamera()->ChangeCameraSpeed(Camera_Movement::DOWN, (float)timeManager->GetDeltaTime());

	if (Input::GetKeyDown(GLFW_KEY_N))
		scene->drawNormals = !scene->drawNormals;
	
	if (Input::GetKeyDown(GLFW_KEY_X  )) {
		wireframe = !wireframe;
		vulkanRenderer->SetWireframe(wireframe);
		std::cout << "wireframe toggled" << std::endl;
	}

	if (Input::GetKeyDown(GLFW_KEY_F)) {
		//walkOnGround = !walkOnGround;
		//std::cout << "flight mode toggled " << std::endl;
	}

	if (Input::GetKeyDown(GLFW_KEY_H)) {
		showGui = !showGui;
	}

	if (Input::GetKeyDown(GLFW_KEY_F10)) {
		vulkanRenderer->SaveScreenshot("VulkanScreenshot.png");
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
	if(mouseControlEnabled)
		glfwSetInputMode(window->getWindowContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else
		glfwSetInputMode(window->getWindowContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
