#include "Editor.h"

int main (int argc, char* argv[])
{
	std::unique_ptr<Engine> vkApp;
	try
	{
		vkApp = std::make_unique<Engine> ();
	}
	catch (const std::runtime_error& e)
	{
		Log.debug (fmt::format ("Engine failed to initialize{}", e.what ()));
		return EXIT_FAILURE;
	}
	Editor editor (*vkApp.get ());

	vkApp->set_imgui_update_callback ([&editor] { editor.update_inputs (); });
	vkApp->set_imgui_draw_callback ([&editor] { editor.draw_imgui (); });
	try
	{
		vkApp->run ();
	}
	catch (const std::runtime_error& e)
	{
		Log.error (fmt::format ("Engine quite in main loop{}", e.what ()));
		return EXIT_FAILURE;
	}
	vkApp.reset ();

	return EXIT_SUCCESS;
}

Editor::Editor (Engine& engine) : engine (engine), imgui_nodeGraph_terrain (engine.input) {}

void Editor::update_inputs ()
{

	if (engine.input.get_key_down (Input::KeyCode::H))
	{
		Log.debug ("gui visibility toggled");
		panels.showGui = !panels.showGui;
	}
}

void Editor::draw_imgui ()
{
	if (debug_mode && panels.showGui)
	{

		if (panels.debug_overlay) debug_overlay (&panels.debug_overlay);
		if (panels.show_player_controller) player_controller (&panels.show_player_controller);
		if (panels.controls_list) controls_window (&panels.controls_list);

		imgui_nodeGraph_terrain.Draw ();

		controller_window (&panels.controller_list);
	}
}

void Editor::debug_overlay (bool* show_debug_overlay)
{

	static bool verbose = false;
	ImGui::SetNextWindowPos (ImVec2 (0, 0), ImGuiCond_Always);
	if (!ImGui::Begin ("Debug Stats",
	        show_debug_overlay,
	        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	            ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::End ();
		return;
	}
	ImGui::Text ("FPS %.3f", ImGui::GetIO ().Framerate);
	ImGui::Text ("DeltaT: %f(s)", engine.time.delta_time ());
	if (ImGui::Button ("Toggle Verbose"))
	{
		verbose = !verbose;
	}
	if (verbose) ImGui::Text ("Run Time: %f(s)", engine.time.running_time ());
	if (verbose) ImGui::Text ("Last frame time%f(s)", engine.time.previous_frame_time ());
	if (verbose) ImGui::Text ("Last frame time%f(s)", engine.time.previous_frame_time ());
	ImGui::Separator ();
	ImGui::Text ("Mouse Position: (%.1f,%.1f)", ImGui::GetIO ().MousePos.x, ImGui::GetIO ().MousePos.y);
	ImGui::End ();
}

void Editor::player_controller (bool* show_player_controller)
{
	ImGui::SetNextWindowPos (ImVec2 (0, 100), ImGuiCond_Once);

	if (!ImGui::Begin ("Player Controller", show_player_controller))
	{
		ImGui::End ();
		return;
	};
	auto pos = engine.scene.player.position ();
	auto rot = engine.scene.player.rotation ();
	auto rot_imag = rot.getImag ();
	ImGui::Text ("x:%f y:%f z:%f", pos.x, pos.y, pos.z);
	ImGui::Text ("w:%f x:%f y:%f z:%f", rot.getReal (), rot_imag.x, rot_imag.y, rot_imag.z);

	// ImGui::Text ("Camera");
	// auto sPos = "Pos " + std::to_string (scene.GetCamera ()->Position.x);
	// auto sDir = "Dir " + std::to_string (scene.GetCamera ()->Front.x);
	// auto sSpeed = "Speed " + std::to_string (scene.GetCamera ()->movement_speed);
	// ImGui::Text ("%s", sPos.c_str ());
	// ImGui::Text ("%s", sDir.c_str ());
	// ImGui::Text ("%s", sSpeed.c_str ());

	// ImGui::DragFloat3 ("Pos", &scene.GetCamera ()->Position.x, 2);
	// ImGui::DragFloat3 ("Rot", &scene.GetCamera ()->Front.x, 2);
	// ImGui::Text ("Camera Movement Speed");
	// ImGui::Text ("%f", scene.GetCamera ()->movement_speed);
	// ImGui::SliderFloat ("##camMovSpeed", &(scene.GetCamera ()->movement_speed), 0.1f, 100.0f);
	ImGui::End ();
}

void Editor::controls_window (bool* show_controls_window)
{
	return;
	ImGui::SetNextWindowPos (ImVec2 (0, 250), ImGuiCond_Once);
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

void Editor::controller_window (bool* show_controller_window)
{


	if (ImGui::Begin ("Controller View", show_controller_window))
	{
		for (int i = 0; i < 16; i++)
		{
			if (engine.input.is_joystick_connected (i))
			{

				ImGui::BeginGroup ();
				for (int j = 0; j < 6; j++)
				{
					ImGui::Text ("%f", engine.input.get_controller_joysticks (i, j));
				}
				ImGui::EndGroup ();
				ImGui::SameLine ();
				ImGui::BeginGroup ();
				for (int j = 0; j < 14; j++)
				{
					engine.input.get_controller_button (i, j) ? ImGui::Text ("true") : ImGui::Text ("false");
				}
				ImGui::EndGroup ();
			}
		}
	}
	ImGui::End ();
}
