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

EngineSettings::EngineSettings (std::filesystem::path file_name) : file_name (file_name)
{
	load ();
}

void EngineSettings::load ()
{
	if (std::filesystem::exists (file_name))
	{
		std::ifstream input{ file_name.string () };
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
		Log.debug ("Settings file didn't exist, creating one");
		save ();
	}
}

void EngineSettings::save ()
{
	nlohmann::json j;

	j["initial-screen-size"]["width"] = screenWidth;
	j["initial-screen-size"]["height"] = screenHeight;

	j["use-validation-layers"] = useValidationLayers;

	j["fullscreen"] = isFullscreen;

	j["is-frame-rate-capped"] = isFrameCapped;
	j["max-fps"] = MaxFPS;

	std::ofstream outFile (file_name);
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
	input.set_mouse_control_status (false);

	Log.debug (fmt::format ("Hardware Threads Available = {}", HardwareThreadCount ()));

	openxr = create_openxr (vulkan_renderer.get_openxr_init ());

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

void Engine::run ()
{

	while (!window.should_window_close ())
	{
		if (window.should_window_resize ())
		{
			if (!window.is_window_iconified ())
			{
				vulkan_renderer.recreate_swapchain ();
				window.set_window_resize_done ();
			}
		}

		time.start_frame_timer ();
		input.update_inputs ();
		process_inputs ();
		scene.update ();

		build_imgui ();
		vulkan_renderer.render_frame ();
		input.reset_released_input ();

		if (settings.isFrameCapped)
		{
			if (time.exact_time_since_frame_start () < 1.0 / settings.MaxFPS)
			{
				std::this_thread::sleep_for (std::chrono::duration<double> (
				    1.0 / settings.MaxFPS - time.exact_time_since_frame_start () - (1.0 / settings.MaxFPS) / 10.0));
			}
		}
		time.end_frame_timer ();
	}

	vulkan_renderer.device_wait ();
	thread_pool.stop ();
}

// Build imGui windows and elements
void Engine::build_imgui ()
{

	imGuiTimer.start_timer ();

	vulkan_renderer.imgui_new_frame ();
	imgui_draw_callback ();

	imGuiTimer.end_timer ();
	// Log.Debug << imGuiTimer.get_elapsed_time_nano_seconds();
}

void Engine::process_inputs ()
{
	double deltaTime = time.delta_time ();

	imgui_update_callback ();
	if (input.get_key_down (Input::KeyCode::ESCAPE)) window.set_window_close ();

	if (!input.get_text_input_mode ())
	{
		if (input.get_key_down (Input::KeyCode::ENTER))
			input.set_mouse_control_status (!input.get_mouse_control_status ());




		// if (input.get_key_down (Input::KeyCode::N)) scene.drawNormals = !scene.drawNormals;

		// if (input.get_key_down (Input::KeyCode::X))
		// {
		// 	vulkan_renderer.ToggleWireframe ();
		// 	Log.debug ("Wireframe toggle");
		// }

		// if (input.get_key_down (Input::KeyCode::F))
		// {
		// 	scene.walkOnGround = !scene.walkOnGround;
		// 	Log.debug ("flight mode toggled");
		// }
	}
	else
	{
		if (input.get_key_down (Input::KeyCode::ESCAPE)) input.reset_text_input_mode ();
	}

	if (input.get_mouse_button_pressed (input.get_mouse_button_pressed (0)))
	{
		if (!ImGui::IsWindowHovered (ImGuiHoveredFlags_AnyWindow))
		{
			input.set_mouse_control_status (true);
		}
	}
}
