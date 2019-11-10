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
		Log.Debug (fmt::format ("Engine failed to initialize\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}
	Editor editor (*vkApp.get ());

	vkApp->SetImguiUpdateCallBack ([&editor] { editor.UpdateInputs (); });
	vkApp->SetImguiDrawCallBack ([&editor] { editor.DrawImgui (); });
	try
	{
		vkApp->Run ();
	}
	catch (const std::runtime_error& e)
	{
		Log.Error (fmt::format ("Engine quite in main loop\n{}\n", e.what ()));
		return EXIT_FAILURE;
	}
	vkApp.reset ();

	return EXIT_SUCCESS;
}

Editor::Editor (Engine& engine) : engine (engine) {}

void Editor::UpdateInputs ()
{

	if (Input::GetKeyDown (Input::KeyCode::H))
	{
		Log.Debug ("gui visibility toggled\n");
		panels.showGui = !panels.showGui;
	}
}

void Editor::DrawImgui ()
{
	if (debug_mode && panels.showGui)
	{

		if (panels.debug_overlay) DebugOverlay (&panels.debug_overlay);
		if (panels.camera_controls) CameraWindow (&panels.camera_controls);
		if (panels.controls_list) ControlsWindow (&panels.controls_list);

		imgui_nodeGraph_terrain.Draw ();

		ControllerWindow (&panels.controller_list);
	}
}

void Editor::DebugOverlay (bool* show_debug_overlay)
{

	static bool verbose = false;
	ImGui::SetNextWindowPos (ImVec2 (0, 0), ImGuiCond_Always);
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
	ImGui::Text ("DeltaT: %f(s)", engine.time_manager.DeltaTime ());
	if (ImGui::Button ("Toggle Verbose"))
	{
		verbose = !verbose;
	}
	if (verbose) ImGui::Text ("Run Time: %f(s)", engine.time_manager.RunningTime ());
	if (verbose) ImGui::Text ("Last frame time%f(s)", engine.time_manager.PreviousFrameTime ());
	if (verbose) ImGui::Text ("Last frame time%f(s)", engine.time_manager.PreviousFrameTime ());
	ImGui::Separator ();
	ImGui::Text ("Mouse Position: (%.1f,%.1f)", ImGui::GetIO ().MousePos.x, ImGui::GetIO ().MousePos.y);
	ImGui::End ();
}

void Editor::CameraWindow (bool* show_camera_window)
{
	ImGui::SetNextWindowPos (ImVec2 (0, 100), ImGuiCond_Once);

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

	// ImGui::DragFloat3 ("Pos", &scene.GetCamera ()->Position.x, 2);
	// ImGui::DragFloat3 ("Rot", &scene.GetCamera ()->Front.x, 2);
	// ImGui::Text ("Camera Movement Speed");
	// ImGui::Text ("%f", scene.GetCamera ()->MovementSpeed);
	// ImGui::SliderFloat ("##camMovSpeed", &(scene.GetCamera ()->MovementSpeed), 0.1f, 100.0f);
	ImGui::End ();
}

void Editor::ControlsWindow (bool* show_controls_window)
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

void Editor::ControllerWindow (bool* show_controller_window)
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
