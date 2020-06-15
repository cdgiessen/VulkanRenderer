#pragma once


#include <memory>

#include "imgui.hpp"

#include "core/Engine.h"

#include "ProcTerrainNodeGraph.h"


struct ImGUI_PanelSettings
{
	bool showGui = true;
	bool show_player_controller = true;
	bool log = true;
	bool debug_overlay = true;
	bool controls_list = true;
	bool controller_list = true;
};

class Editor
{
	public:
	Editor (Engine& engine);

	void update_inputs ();

	void draw_imgui ();

	Engine& engine;
	void debug_overlay (bool* show_debug_overlay);
	void player_controller (bool* show_player_controller);
	void controls_window (bool* show_controls_window);
	void controller_window (bool* show_controller_window);

	ImGUI_PanelSettings panels;

	ProcTerrainNodeGraph imgui_nodeGraph_terrain;
	bool debug_mode = true;
};