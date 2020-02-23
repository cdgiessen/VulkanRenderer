#include "ProcTerrainNodeGraph.h"

#include <filesystem>
#include <fstream>
#include <iomanip>

#include <nlohmann/json.hpp>

#include "noc/noc_file_dialog.h"

#include "core/Input.h"
#include "core/Logger.h"


template <typename Enumeration>
auto as_integer (Enumeration const value) -> typename std::underlying_type<Enumeration>::type
{
	return static_cast<typename std::underlying_type<Enumeration>::type> (value);
}

ProcTerrainNodeGraph::ProcTerrainNodeGraph (Input::InputDirector& input) : input (input)
{
	LoadGraphFromFile ("assets/graphs/default_terrain.json");
}


ProcTerrainNodeGraph::~ProcTerrainNodeGraph () {}

InternalGraph::GraphPrototype& ProcTerrainNodeGraph::GetGraph () { return protoGraph; }

void ProcTerrainNodeGraph::BuildTerGenNodeGraph () {}

ConId ProcTerrainNodeGraph::NewCon (ConnectionType conType, NodeId input, NodeId output, int output_slot_id)
{
	int nextID = nextConId++;
	connections.emplace (std::piecewise_construct,
	    std::forward_as_tuple (nextID),
	    std::forward_as_tuple (conType, input, output, output_slot_id));
	return nextID;
}

void ProcTerrainNodeGraph::DeleteNode (NodeId nodeId)
{
	auto it = nodes.find (nodeId);
	if (it != nodes.end ())
	{
		auto& n = nodes.at (nodeId);
		for (auto& slot : n.inputSlots)
		{
			if (slot.connection != -1)
			{
				ResetNodeInternalLinkByID (n.id, slot.slotNum);
				DeleteConnection (slot.connection);
			}
		}
		for (auto& con : n.outputSlot.connections)
		{
			DeleteConnection (con);
		}
		protoGraph.DeleteNode (n.internalNodeID);
		nodes.erase (it);
	}
	else
	{
		Log.Debug ("Couldn't find node to delete! Does it exist?");
	}
}

void ProcTerrainNodeGraph::DeleteConnection (ConId con)
{
	auto it = connections.find (con);
	if (it != connections.end ())
	{
		auto& c = connections.at (con);

		ResetNodeInternalLinkByID (nodes.at (c.output).internalNodeID, c.output_slot_id);

		for (auto& in : nodes.at (c.input).outputSlot.connections)
		{
			if (in == con)
			{
				in = -1;
			}
		}
		for (auto& out : nodes.at (c.output).inputSlots)
		{
			if (out.connection == con)
			{
				out.connection = -1;
			}
		}


		connections.erase (it);
	}
	else
	{
		Log.Error ("Couldn't find connection! Does it exist?");
	}
}

void ProcTerrainNodeGraph::ResetGraph ()
{
	connections.clear ();
	nodes.clear ();
	protoGraph.ResetGraph ();
	curID = 0;
}

void ProcTerrainNodeGraph::RecreateOutputNode ()
{
	outputNode = AddNode (NodeType::Output, startingNodePos, curID++);
}

void ProcTerrainNodeGraph::Draw ()
{

	ImGui::SetNextWindowSize (ImVec2 (800, 400), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos (ImVec2 (425, 0), ImGuiCond_FirstUseEver);


	if (ImGui::Begin ("Node Graph", &window_open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar))
	{
		windowPos = ImGui::GetWindowPos ();


		DrawMenuBar ();
		DrawButtonBar ();
		DrawNodeButtons ();
		DrawNodeCanvas ();
	}
	ImGui::End ();
}

void ProcTerrainNodeGraph::DrawMenuBar ()
{
	if (ImGui::BeginMenuBar ())
	{
		if (ImGui::BeginMenu ("File"))
		{
			if (ImGui::MenuItem ("New"))
			{
			}
			if (ImGui::MenuItem ("Open", "Ctrl+O"))
			{
				LoadGraphFromFile ();
			}
			if (ImGui::BeginMenu ("Open Recent"))
			{
				ImGui::EndMenu ();
			}
			if (ImGui::MenuItem ("Save", "Ctrl+S"))
			{
			}
			if (ImGui::MenuItem ("Save As.."))
			{
			}
			ImGui::EndMenu ();
		}

		if (ImGui::BeginMenu ("Options"))
		{
			ImGui::EndMenu ();
		}
		ImGui::EndMenuBar ();
	}
}

void ProcTerrainNodeGraph::DrawButtonBar ()
{
	ImGui::BeginGroup ();

	if (ImGui::Button ("Reset Viewport"))
	{
		graphOffset = ImVec2 (0, 0);
	}
	ImGui::SameLine ();
	if (ImGui::Button ("Reset graph - DELETES EVERYTHING!"))
	{
		ResetGraph ();
		RecreateOutputNode ();
	}
	ImGui::SameLine ();
	if (ImGui::Button ("Save graph"))
	{
		SaveGraphFromFile ();
	}
	ImGui::SameLine ();
	if (ImGui::Button ("Load graph"))
	{
		LoadGraphFromFile ();
	}

	ImGui::EndGroup ();
}

void ProcTerrainNodeGraph::DrawNodeButtons ()
{

	ImGui::PushStyleVar (ImGuiStyleVar_ChildRounding, 2.0f);
	ImGui::BeginChild ("node buttons", ImVec2 (120, 0.0f), true);

	ImGui::Text ("Noise Sources");
	if (ImGui::Button ("White Noise", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::WhiteNoise, startingNodePos);
	}
	if (ImGui::Button ("Perlin", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::PerlinNoise, startingNodePos);
	}
	if (ImGui::Button ("Simplex", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::SimplexNoise, startingNodePos);
	}
	if (ImGui::Button ("Value", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::ValueNoise, startingNodePos);
	}
	if (ImGui::Button ("Cubic", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::CubicNoise, startingNodePos);
	}
	if (ImGui::Button ("Fractal Type", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::FractalReturnType, startingNodePos);
	}
	if (ImGui::Button ("Voronoi", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::VoronoiNoise, startingNodePos);
	}
	if (ImGui::Button ("Cellular", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::CellNoise, startingNodePos);
	}
	if (ImGui::Button ("Cellular Type", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::CellularReturnType, startingNodePos);
	}

	ImGui::Separator ();
	ImGui::Text ("Modifiers");
	if (ImGui::Button ("Constant Int", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::ConstantInt, startingNodePos);
	}
	if (ImGui::Button ("Constant Float", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::ConstantFloat, startingNodePos);
	}
	if (ImGui::Button ("Inverter", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Invert, startingNodePos);
	}
	if (ImGui::Button ("Add", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Addition, startingNodePos);
	}
	if (ImGui::Button ("Subtract", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Subtraction, startingNodePos);
	}
	if (ImGui::Button ("Multiply", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Multiplication, startingNodePos);
	}
	if (ImGui::Button ("Divide", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Division, startingNodePos);
	}
	if (ImGui::Button ("Power", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Power, startingNodePos);
	}
	if (ImGui::Button ("Max", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Max, startingNodePos);
	}
	if (ImGui::Button ("Min", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Min, startingNodePos);
	}
	if (ImGui::Button ("Blend", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Blend, startingNodePos);
	}
	if (ImGui::Button ("Clamp", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Clamp, startingNodePos);
	}
	if (ImGui::Button ("Selector", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::Selector, startingNodePos);
	}

	ImGui::Separator ();
	ImGui::Text ("Colors");
	if (ImGui::Button ("Color Creator", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::ColorCreator, startingNodePos);
	}
	if (ImGui::Button ("MonoGradient", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::MonoGradient, startingNodePos);
	}
	if (ImGui::Button ("Texture Index", ImVec2 (-1.0f, 0.0f)))
	{
		AddNode (NodeType::TextureIndex, startingNodePos);
	}

	ImGui::EndChild ();
	ImGui::PopStyleVar ();
}

void ProcTerrainNodeGraph::DrawNodeCanvas ()
{
	// Create our child canvas
	// ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);

	ImGui::SameLine ();

	ImGui::BeginGroup ();
	ImGui::PushStyleVar (ImGuiStyleVar_FramePadding, ImVec2 (1, 1));
	ImGui::PushStyleVar (ImGuiStyleVar_WindowPadding, ImVec2 (0, 0));
	ImGui::PushStyleColor (ImGuiCol_ChildBg, ImVec4 (40, 40, 40, 200));
	ImGui::BeginChild ("scrolling_region", ImVec2 (0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth (120.0f);

	ImDrawList* draw_list = ImGui::GetWindowDrawList ();
	draw_list->ChannelsSplit (2);

	DrawNodes (draw_list);
	DrawPossibleConnection (draw_list);
	DrawConnections (draw_list);

	draw_list->ChannelsMerge ();

	ImGui::PopItemWidth ();
	ImGui::EndChild ();
	ImGui::PopStyleColor ();
	ImGui::PopStyleVar (2);
	ImGui::EndGroup ();
}

void ProcTerrainNodeGraph::DrawNodes (ImDrawList* imDrawList)
{

	std::vector<NodeId> nodesToDelete;

	for (auto& [id, node] : nodes)
	{

		// updates nodes positions, relative to mouseDelta
		if (ImGui::IsMouseDragging (1) && !posCon.isActive) // right mouse click
			node.pos = node.pos + ImGui::GetIO ().MouseDelta;


		int node_hovered_in_scene = -1;
		bool open_context_menu = false;

		ImGui::PushID (node.id);

		// Display node contents first
		imDrawList->ChannelsSetCurrent (1); // Foreground
		bool old_any_active = ImGui::IsAnyItemActive ();

		ImVec2 titleArea = ImVec2 (node.size.x, 30.0f);

		// Draw title in center
		ImVec2 textSize = ImGui::CalcTextSize (node.name.c_str ());

		ImVec2 textPos = node.pos + windowPadding;
		textPos.x = node.pos.x + (node.size.x / 2) - textSize.x / 2;

		ImGui::SetCursorScreenPos (windowPos + textPos);
		ImGui::Text ("%s", node.name.c_str ());

		// Save the size of what we have emitted and wether any of the widgets are being used
		// bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive ());

		// Display node box
		imDrawList->ChannelsSetCurrent (0); // Background

		ImGui::SetCursorScreenPos (windowPos + node.pos);
		ImGui::InvisibleButton ("node", titleArea);

		if (ImGui::IsItemHovered (ImGuiHoveredFlags_RectOnly))
		{
			node_hovered_in_scene = node.id;
			open_context_menu |= ImGui::IsMouseClicked (1);
		}

		bool node_moving_active = false;

		if (ImGui::IsItemActive ()) node_moving_active = true;

		// background
		ImU32 node_bg_color =
		    (node_hovered_in_scene == node.id) ? ImColor (75, 75, 75) : ImColor (60, 60, 60);
		imDrawList->AddRectFilled (windowPos + node.pos, windowPos + node.pos + node.size, node_bg_color, 4.0f);


		// Draw text bg area
		imDrawList->AddRectFilled (
		    windowPos + node.pos + ImVec2 (1, 1), windowPos + node.pos + titleArea, ImColor (100, 0, 0), 4.0f);
		imDrawList->AddRect (
		    windowPos + node.pos, windowPos + node.pos + node.size, ImColor (100, 100, 100), 4.0f);

		if (ImGui::BeginPopupContextItem ("slot context menu"))
		{
			if (ImGui::Selectable ("Delete Node"))
			{
				nodesToDelete.push_back (node.id);
			}
			ImGui::Text ("Edit name:");
			char name[32];
			memset (name, '\0', 32);
			node.name.copy (name, 31);
			if (node.hasTextInput == false)
			{
				input.SetTextInputMode ();
				node.hasTextInput = true;
			}
			if (ImGui::InputText ("##edit", name, sizeof (char) * 32))
			{
				name[31] = '\0';
				node.name.replace (node.name.begin (), node.name.end (), name);
			}
			else
			{
			}
			ImGui::EndPopup ();
		}
		else
		{
			if (node.hasTextInput)
			{
				input.ResetTextInputMode ();
				node.hasTextInput = false;
			}
		}

		node.outputSlot.Draw (imDrawList, *this, node, 0);

		int verticalOffset = 40;
		for (size_t i = 0; i < node.inputSlots.size (); i++)
		{
			verticalOffset += node.inputSlots[i].Draw (imDrawList, *this, node, verticalOffset);
		}
		node.size = ImVec2 (node.size.x, static_cast<float> (verticalOffset));

		if (node_moving_active && ImGui::IsMouseDragging (0) && !posCon.isActive)
			node.pos = node.pos + ImGui::GetIO ().MouseDelta;

		ImGui::PopID ();
	}

	for (auto& nodeId : nodesToDelete)
	{
		if (nodes.at (nodeId).hasTextInput)
		{
			input.ResetTextInputMode ();
		}
		DeleteNode (nodeId);
		nodes.erase (nodeId);
	}
}

void ProcTerrainNodeGraph::DrawPossibleConnection (ImDrawList* imDrawList)
{
	HoveredSlotInfo info = GetHoveredSlot ();
	ImVec2 startPos;
	switch (posCon.state)
	{
		case (PossibleConnection::State::Default):

			if (info.node != -1)
			{
				posCon.state = PossibleConnection::State::Hover;
				posCon.slot = info;
			}

			break;
		case (PossibleConnection::State::Hover):

			if (info.node == posCon.slot.node)
			{
				if (ImGui::IsMouseClicked (0) && posCon.slot.node != -1)
				{
					posCon.isActive = true;
					posCon.state = PossibleConnection::State::Dragging;

					if (!info.isOutput && (nodes.at (info.node).inputSlots[info.inputSlotNum].connection != -1))
					{
						ResetNodeInternalLinkByID (
						    nodes.at (posCon.slot.node).internalNodeID, posCon.slot.inputSlotNum);
						DeleteConnection (nodes.at (info.node).inputSlots[info.inputSlotNum].connection);
					}
				}
			}
			else
			{
				// posCon.slot.node->internal_node->ResetInputLink(posCon.slot.inputSlotNum);
				posCon.isActive = false;
				posCon.slot = HoveredSlotInfo ();
				posCon.state = PossibleConnection::State::Default;
			}

			break;
		case (PossibleConnection::State::Dragging):

			imDrawList->ChannelsSetCurrent (0); // Background

			startPos = windowPos + nodes.at (posCon.slot.node).pos +
			           PossibleConnection::GetActiveSlotPosition (posCon.slot, nodes);

			DrawHermite (imDrawList, startPos, ImGui::GetIO ().MousePos, 12);

			if (ImGui::IsMouseClicked (0))
			{
				if (PossibleConnection::GetActiveSlotType (posCon.slot, nodes) ==
				        PossibleConnection::GetActiveSlotType (info, nodes) &&
				    posCon.slot.node != info.node)
				{

					HoveredSlotInfo outgoingSlot;
					HoveredSlotInfo incomingSlot;

					// selection for which is input and output
					if (posCon.slot.isOutput)
					{
						outgoingSlot = posCon.slot;
						incomingSlot = info;
					}
					else
					{
						outgoingSlot = info;
						incomingSlot = posCon.slot;
					}

					// Delete old connection if exists
					if (nodes.at (incomingSlot.node).inputSlots[incomingSlot.inputSlotNum].connection != -1)
					{

						ResetNodeInternalLinkByID (
						    nodes.at (incomingSlot.node).internalNodeID, incomingSlot.inputSlotNum);
						DeleteConnection (
						    nodes.at (incomingSlot.node).inputSlots[incomingSlot.inputSlotNum].connection);
					}

					ConId newCon = NewCon (PossibleConnection::GetActiveSlotType (posCon.slot, nodes),
					    outgoingSlot.node,
					    incomingSlot.node,
					    incomingSlot.inputSlotNum);

					nodes.at (incomingSlot.node).inputSlots[incomingSlot.inputSlotNum].connection = newCon;
					nodes.at (outgoingSlot.node).outputSlot.connections.push_back (newCon);

					connections.at (newCon).startPosRelNode =
					    PossibleConnection::GetActiveSlotPosition (outgoingSlot, nodes);
					connections.at (newCon).endPosRelNode =
					    PossibleConnection::GetActiveSlotPosition (incomingSlot, nodes);

					// incomingSlot.node->SetInternalLink(incomingSlot.inputSlotNum, outgoingSlot.node->internalNodeID);
					SetNodeInternalLinkByID (nodes.at (incomingSlot.node).internalNodeID,
					    incomingSlot.inputSlotNum,
					    nodes.at (outgoingSlot.node).internalNodeID);
					// connections.push_back (newConnection);

					posCon.isActive = false;
					posCon.slot = HoveredSlotInfo ();
					posCon.state = PossibleConnection::State::Default;
				}
			}
			else if (ImGui::IsMouseClicked (1))
			{
				if (posCon.slot.isOutput)
				{
				}
				else
				{
					ResetNodeInternalLinkByID (
					    nodes.at (posCon.slot.node).internalNodeID, posCon.slot.inputSlotNum);
				}
				// posCon.slot.node->internal_node->ResetInputLink(posCon.slot.inputSlotNum);
				posCon.isActive = false;
				posCon.slot = HoveredSlotInfo ();
				posCon.state = PossibleConnection::State::Default;
			}
			break;
		default:
			// Something went wrong
			break;
	}
}

HoveredSlotInfo ProcTerrainNodeGraph::GetHoveredSlot ()
{
	for (auto& [id, node] : nodes)
	{
		for (int i = 0; i < static_cast<int> (node.inputSlots.size ()); i++)
		{
			if (node.inputSlots[i].IsHoveredOver (windowPos + node.pos))
				return HoveredSlotInfo (id, false, i);
		}
		if (node.outputSlot.IsHoveredOver (windowPos + node.pos))
			return HoveredSlotInfo (id, true, -1);
	}
	return HoveredSlotInfo ();
}

void ProcTerrainNodeGraph::DrawHermite (ImDrawList* imDrawList, ImVec2 p1, ImVec2 p2, int STEPS)
{
	ImVec2 t1 = ImVec2 (+80.0f, 0.0f);
	ImVec2 t2 = ImVec2 (+80.0f, 0.0f);

	for (int step = 0; step <= STEPS; step++)
	{
		float t = (float)step / (float)STEPS;
		float h1 = +2 * t * t * t - 3 * t * t + 1.0f;
		float h2 = -2 * t * t * t + 3 * t * t;
		float h3 = t * t * t - 2 * t * t + t;
		float h4 = t * t * t - t * t;
		imDrawList->PathLineTo (ImVec2 (h1 * p1.x + h2 * p2.x + h3 * t1.x + h4 * t2.x,
		    h1 * p1.y + h2 * p2.y + h3 * t1.y + h4 * t2.y));
	}

	imDrawList->PathStroke (ImColor (200, 200, 100), false, 3.0f);
}

void ProcTerrainNodeGraph::DrawConnections (ImDrawList* imDrawList)
{
	for (auto& [id, con] : connections)
	{
		con.endPosRelNode = nodes.at (con.output).inputSlots.at (con.output_slot_id).pos;
		ImVec2 start = windowPos + nodes.at (con.input).pos + con.startPosRelNode;
		ImVec2 end = windowPos + nodes.at (con.output).pos + con.endPosRelNode;

		DrawHermite (imDrawList, start, end, 12);
	}
}

PossibleConnection::PossibleConnection () : state (State::Default), slot () {}

ImVec2 PossibleConnection::GetActiveSlotPosition (HoveredSlotInfo info, std::unordered_map<NodeId, Node> nodes)
{
	if (info.node != -1)
	{
		if (info.isOutput)
		{
			return nodes.at (info.node).outputSlot.pos;
		}
		else
		{
			return nodes.at (info.node).inputSlots[info.inputSlotNum].pos;
		}
	}
	return ImVec2 (0, 0);
}

ConnectionType PossibleConnection::GetActiveSlotType (HoveredSlotInfo info, std::unordered_map<NodeId, Node> nodes)
{
	if (info.node != -1)
	{
		if (info.isOutput)
		{
			return nodes.at (info.node).outputSlot.GetType ();
		}
		else
		{
			return nodes.at (info.node).inputSlots[info.inputSlotNum].GetType ();
		}
	}
	return ConnectionType::Null;
}

void ProcTerrainNodeGraph::SaveGraphFromFile ()
{
	Log.Debug (fmt::format ("PWD = {}", std::filesystem::current_path ().string ()));
	const char* filename = noc_file_dialog_open (
	    NOC_FILE_DIALOG_SAVE, NULL, std::filesystem::current_path ().string ().c_str (), NULL);
	if (filename != NULL)
	{
		SaveGraphFromFile (std::string (filename));
	}
}

void ProcTerrainNodeGraph::SaveGraphFromFile (std::string fileName)
{
	std::ofstream outFile (fileName);
	if (!outFile)
	{
		Log.Debug ("Bad file name for terrain graph");
		return;
	}
	nlohmann::json j;

	j["numNodes"] = nodes.size ();

	int curNodeIndex = 0;
	for (auto& [id, node] : nodes)
	{
		nlohmann::json nodeJson;
		nodeJson["id"] = id;
		nodeJson["nodeType"] = as_integer (node.nodeType);
		nodeJson["winPosX"] = node.pos.x;
		nodeJson["winPosY"] = node.pos.y;
		nodeJson["numSlots"] = node.inputSlots.size ();



		for (auto& slot : node.inputSlots)
		{
			nlohmann::json slotJson;
			slotJson["slotName"] = slot.name;
			slotJson["slotType"] = as_integer (slot.conType);
			if (slot.connection == -1)
			{
				slotJson["hasConnection"] = false;
				if (slot.value.type == ConnectionType::Int)
					slotJson["value"] = std::get<int> (slot.value.value);
				else if (slot.value.type == ConnectionType::Float)
					slotJson["value"] = std::get<float> (slot.value.value);

				else if (slot.value.type == ConnectionType::Vec2)
				{
					cml::vec2f vec2 = std::get<cml::vec2f> (slot.value.value);
					slotJson["value"] = { vec2.x, vec2.y };
				}
				else if (slot.value.type == ConnectionType::Vec3)
				{
					cml::vec3f vec3 = std::get<cml::vec3f> (slot.value.value);
					slotJson["value"] = { vec3.x, vec3.y, vec3.z };
				}
				else if (slot.value.type == ConnectionType::Vec4 || slot.value.type == ConnectionType::Color)
				{
					cml::vec4f vec4 = std::get<cml::vec4f> (slot.value.value);
					slotJson["value"] = { vec4.x, vec4.y, vec4.z, vec4.w };
				}
			}
			else
			{
				slotJson["hasConnection"] = true;
				slotJson["value"] = connections.at (slot.connection).input;
			}


			nodeJson[std::to_string (slot.slotNum)] = slotJson;
		}

		j[std::to_string (curNodeIndex)] = nodeJson;
		curNodeIndex++;
	}

	outFile << std::setw (4) << j;
	outFile.close ();
}

void ProcTerrainNodeGraph::LoadGraphFromFile ()
{
	const char* filename = noc_file_dialog_open (
	    NOC_FILE_DIALOG_OPEN, NULL, std::filesystem::current_path ().string ().c_str (), NULL);
	if (filename != NULL)
	{
		LoadGraphFromFile (std::string (filename));
	}
	else
	{
		Log.Debug ("No file specified, stopping load");
	}
}

void ProcTerrainNodeGraph::LoadGraphFromFile (std::string fileName)
{

	std::ifstream inFile (fileName);
	if (!inFile)
	{
		Log.Debug ("Bad file name");
		return;
	}

	nlohmann::json j;

	if (inFile.peek () == std::ifstream::traits_type::eof ())
	{
		Log.Error ("Opened file is empty!");
		return;
	}

	try
	{
		inFile >> j;
	}
	catch (nlohmann::json::parse_error& e)
	{
		Log.Error (fmt::format ("{}", e.what ()));
	}
	inFile.close ();

	ResetGraph ();
	try
	{
		int numNodes = j["numNodes"];
		nodes.reserve (numNodes);
		for (int i = 0; i < numNodes; i++)
		{
			std::string curIndex (std::to_string (i));
			ImVec2 pos = ImVec2 (j[curIndex]["winPosX"], j[curIndex]["winPosY"]);
			int type = j[curIndex]["nodeType"];
			AddNode (static_cast<NodeType> (type), pos, j[curIndex]["id"]);
		}

		for (int i = 0; i < numNodes; i++)
		{
			std::string curIndex (std::to_string (i));
			auto node = j[curIndex]["id"];
			for (int slot = 0; slot < static_cast<int> (nodes.at (node).inputSlots.size ()); slot++)
			{
				std::string curSlotIndex (std::to_string (slot));

				bool hasCon = j[curIndex][curSlotIndex]["hasConnection"];
				if (hasCon)
				{
					int conIndex = j[curIndex][curSlotIndex]["value"];

					auto outGoingNode = conIndex;
					if (outGoingNode == -1)
					{
						Log.Error ("Couldn't find node by id in loaded graph");
					}

					int slotType = j[curIndex][curSlotIndex]["slotType"];
					ConId newConnection =
					    NewCon (static_cast<ConnectionType> (slotType), outGoingNode, node, slot);

					nodes.at (node).inputSlots[slot].connection = newConnection;
					nodes.at (outGoingNode).outputSlot.connections.push_back (newConnection);

					connections.at (newConnection).startPosRelNode =
					    nodes.at (outGoingNode).outputSlot.pos;
					connections.at (newConnection).endPosRelNode = nodes.at (node).inputSlots[slot].pos;

					SetNodeInternalLinkByID (
					    nodes.at (node).internalNodeID, slot, nodes.at (outGoingNode).internalNodeID);
				}
				else
				{
					int slotType = j[curIndex][curSlotIndex]["slotType"];
					ConnectionType type = static_cast<ConnectionType> (slotType);

					if (type == ConnectionType::Int)
					{
						int valInt = j[curIndex][curSlotIndex]["value"];
						nodes.at (node).inputSlots[slot].value.value = valInt;
					}
					if (type == ConnectionType::Float)
					{
						float valFloat = j[curIndex][curSlotIndex]["value"];
						nodes.at (node).inputSlots[slot].value.value = valFloat;
					}
					if (type == ConnectionType::Vec2)
					{
						float arr[2];
						arr[0] = j[curIndex][curSlotIndex]["value"];
						arr[1] = j[curIndex][curSlotIndex]["value"];

						nodes.at (node).inputSlots[slot].value.value = cml::vec2f (arr[0], arr[1]);
					}
					if (type == ConnectionType::Vec3)
					{
						float arr[3];
						arr[0] = j[curIndex][curSlotIndex]["value"];
						arr[1] = j[curIndex][curSlotIndex]["value"];
						arr[2] = j[curIndex][curSlotIndex]["value"];
						nodes.at (node).inputSlots[slot].value.value = cml::vec3f (arr[0], arr[1], arr[2]);
					}
					if (type == ConnectionType::Vec4 || type == ConnectionType::Color)
					{
						float arr[4];
						arr[0] = j[curIndex][curSlotIndex]["value"];
						arr[1] = j[curIndex][curSlotIndex]["value"];
						arr[2] = j[curIndex][curSlotIndex]["value"];
						arr[3] = j[curIndex][curSlotIndex]["value"];
						nodes.at (node).inputSlots[slot].value.value =
						    cml::vec4f (arr[0], arr[1], arr[2], arr[3]);
					}
				}
			}
		}
	}
	catch (nlohmann::json::parse_error& e)
	{
		Log.Error (fmt::format ("{}", e.what ()));
	}
}

NodeId ProcTerrainNodeGraph::AddNode (NodeType nodeType, ImVec2 position, int id)
{
	int nextID = -1;
	if (id >= 0)
	{
		nextID = id;
	}
	else
	{
		nextID = curID++;
	}
	nodes.emplace (std::piecewise_construct,
	    std::forward_as_tuple (nextID),
	    std::forward_as_tuple (nodeType, nextID, position, protoGraph));

	return nextID;
}

ConnectionSlot::ConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type)
: slotNum (slotNum), conType (type), pos (pos)
{
}

ConnectionSlot::ConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type, std::string name)
: slotNum (slotNum), pos (pos), name (name), conType (type)
{
}

bool ConnectionSlot::IsHoveredOver (ImVec2 parentPos) const
{
	ImVec2 mousePos = ImGui::GetIO ().MousePos;
	ImVec2 conPos = parentPos + pos;

	float xd = mousePos.x - conPos.x;
	float yd = mousePos.y - conPos.y;

	return ((xd * xd) + (yd * yd)) < (nodeSlotRadius * nodeSlotRadius);
}

ConnectionType ConnectionSlot::GetType () { return conType; }

InputConnectionSlot::InputConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type, std::string name)
: ConnectionSlot (slotNum, pos, type, name), value (type)
{
	switch (value.type)
	{
		case (ConnectionType::Int):
			value.value = 0;
			break;
		case (ConnectionType::Float):
			value.value = 0.0f;
			break;
		case (ConnectionType::Color):
			value.value = cml::vec4f (0);
			break;
		case (ConnectionType::Vec2):
			value.value = cml::vec2f (0);
			break;
		case (ConnectionType::Vec3):
			value.value = cml::vec3f (0);
			break;
		case (ConnectionType::Vec4):
			value.value = cml::vec4f (0);
			break;
		default:
			value.value = 0;
			break;
	}
}

InputConnectionSlot::InputConnectionSlot (
    int slotNum, ImVec2 pos, ConnectionType type, std::string name, SlotValueVariant defaultValue)
: ConnectionSlot (slotNum, pos, type, name), value (type)
{
	value.value = defaultValue;
}

InputConnectionSlot::InputConnectionSlot (int slotNum,
    ImVec2 pos,
    ConnectionType type,
    std::string name,
    SlotValueVariant defaultValue,
    float sliderStepSize,
    float lowerBound = 0.0f,
    float upperBound = 0.0f)
: ConnectionSlot (slotNum, pos, type, name),
  value (type),
  sliderStepSize (sliderStepSize),
  lowerBound (lowerBound),
  upperBound (upperBound)
{
	value.value = defaultValue;
}

int InputConnectionSlot::Draw (
    ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset)
{

	ImVec2 relPos = ImVec2 (graph.windowPos.x + parentNode.pos.x, graph.windowPos.y + parentNode.pos.y);
	pos = ImVec2 (0, static_cast<float> (verticalOffset));
	ImVec2 currentPos = relPos + pos;
	int slotHeight = 35;

	ImGui::SetCursorScreenPos (currentPos + ImVec2 (10, -nodeSlotRadius));
	ImGui::Text ("%s", name.c_str ());

	// ImVec2 slotNameSize = ImGui::CalcTextSize (name.c_str ());
	ImGui::SetCursorScreenPos (currentPos + ImVec2 (5, -nodeSlotRadius + 15));

	ImGui::PushItemWidth (parentNode.size.x - 10);
	std::string uniqueID = (std::to_string (parentNode.id * 100 + slotNum));
	switch (value.type)
	{
		case (ConnectionType::Int):
			ImGui::DragInt (std::string ("##int" + uniqueID).c_str (),
			    &(std::get<int> (value.value)),
			    sliderStepSize,
			    static_cast<int> (lowerBound),
			    static_cast<int> (upperBound));
			break;
		case (ConnectionType::Float):
			ImGui::DragFloat (std::string ("##float" + uniqueID).c_str (),
			    &(std::get<float> (value.value)),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		case (ConnectionType::Vec2):
			ImGui::DragFloat2 (std::string ("##vec2" + uniqueID).c_str (),
			    &(std::get<cml::vec2f> (value.value).x),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		case (ConnectionType::Vec3):
			ImGui::DragFloat3 (std::string ("##vec3" + uniqueID).c_str (),
			    &(std::get<cml::vec3f> (value.value).x),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		case (ConnectionType::Vec4):
			ImGui::DragFloat4 (std::string ("##vec4" + uniqueID).c_str (),
			    &(std::get<cml::vec4f> (value.value).x),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		case (ConnectionType::Color):
			ImGui::DragFloat4 (std::string ("##color" + uniqueID).c_str (),
			    &(std::get<cml::vec4f> (value.value).x),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		default:
			break;
	}
	graph.SetNodeInternalValueByID (parentNode.internalNodeID, slotNum, value.value);
	ImGui::PopItemWidth ();

	ImColor conColor = ImColor (150, 150, 150);

	if (connection != -1) conColor = ImColor (210, 210, 0);

	if (IsHoveredOver (relPos)) conColor = ImColor (200, 200, 200);

	imDrawList->AddCircleFilled (currentPos, nodeSlotRadius, conColor);

	return slotHeight;
}

OutputConnectionSlot::OutputConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type)
: ConnectionSlot (slotNum, pos, type)
{
}

int OutputConnectionSlot::Draw (
    ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset)
{
	slotColor = ImColor (150, 150, 150);
	ImVec2 relPos = ImVec2 (graph.windowPos.x + parentNode.pos.x, graph.windowPos.y + parentNode.pos.y);
	if (IsHoveredOver (relPos)) slotColor = ImColor (200, 200, 200);
	if (connections.size () > 0) slotColor = ImColor (210, 210, 0);
	imDrawList->AddCircleFilled (relPos + pos, nodeSlotRadius, slotColor);
	return 0;
}

Connection::Connection (ConnectionType conType, NodeId input, NodeId output, int output_slot_id)
: conType (conType), input (input), output (output), output_slot_id (output_slot_id)
{
}

void ProcTerrainNodeGraph::SetNodeInternalLinkByID (
    InternalGraph::NodeID internalNodeID, int index, InternalGraph::NodeID id)
{
	protoGraph.GetNodeByID (internalNodeID).SetLinkInput (index, id);
}

void ProcTerrainNodeGraph::ResetNodeInternalLinkByID (InternalGraph::NodeID internalNodeID, int index)
{
	protoGraph.GetNodeByID (internalNodeID).ResetLinkInput (index);
}

void ProcTerrainNodeGraph::SetNodeInternalValueByID (
    InternalGraph::NodeID internalNodeID, int index, InternalGraph::LinkTypeVariants val)
{
	protoGraph.GetNodeByID (internalNodeID).SetLinkValue (index, val);
}

void Node::AddInputSlot (ConnectionType type, std::string name)
{
	inputSlots.push_back (InputConnectionSlot ((int)inputSlots.size (), ImVec2 (0, 40), type, name));
}
void Node::AddInputSlot (ConnectionType type,
    std::string name,
    float defaultValue,
    float sliderStepSize = 0.01f,
    float lowerBound = 0.0f,
    float upperBound = 0.0f)
{
	inputSlots.push_back (InputConnectionSlot (
	    (int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue, sliderStepSize, lowerBound, upperBound));
}

void Node::AddInputSlot (ConnectionType type,
    std::string name,
    int defaultValue,
    float sliderStepSize = 0.1f,
    float lowerBound = 0.0f,
    float upperBound = 0.0f)
{
	inputSlots.push_back (InputConnectionSlot (
	    (int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue, sliderStepSize, lowerBound, upperBound));
}

void Node::AddInputSlot (ConnectionType type, std::string name, cml::vec2f defaultValue)
{
	inputSlots.push_back (InputConnectionSlot ((int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue));
}

void Node::AddInputSlot (ConnectionType type, std::string name, cml::vec3f defaultValue)
{
	inputSlots.push_back (InputConnectionSlot ((int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue));
}

void Node::AddInputSlot (ConnectionType type, std::string name, cml::vec4f defaultValue)
{
	inputSlots.push_back (InputConnectionSlot ((int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue));
}

Node::Node (NodeType type, NodeId id, ImVec2 position, InternalGraph::GraphPrototype& graph)
: nodeType (type), id (id), pos (position), outputSlot (0, ImVec2 (size.x, 15), ConnectionType::Float)
{
	switch (type)
	{
		case (NodeType::Output):
			outputSlot = OutputConnectionSlot (0, ImVec2 (size.x, 15), ConnectionType::Float);
			name = "Output";

			AddInputSlot (ConnectionType::Float, "HeightMap", -0.5f);
			AddInputSlot (ConnectionType::Color, "Splatmap", cml::vec4f (0));

			AddInputSlot (ConnectionType::Int, "Texture 1", 0);
			AddInputSlot (ConnectionType::Int, "Texture 2", 1);
			AddInputSlot (ConnectionType::Int, "Texture 3", 2);
			AddInputSlot (ConnectionType::Int, "Texture 4", 3);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Output);

			break;
		case (NodeType::Addition):
			name = "Addition";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Addition);

			break;
		case (NodeType::Subtraction):
			name = "Subtraction";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Subtraction);

			break;
		case (NodeType::Multiplication):
			name = "Multiplication";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Multiplication);

			break;
		case (NodeType::Division):
			name = "Division";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Division);

			break;
		case (NodeType::Power):
			name = "Power";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Power);

			break;
		case (NodeType::Max):
			name = "Max";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Max);

			break;
		case (NodeType::Min):
			name = "Min";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Min);

			break;
		case (NodeType::Blend):
			name = "Blend";

			AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "B", 1.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "Factor", 0.5f, 0.005f, 0.0f, 1.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Blend);

			break;
		case (NodeType::Clamp):
			name = "Clamp";

			AddInputSlot (ConnectionType::Float, "input", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "lower", 0.0f, 0.005f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "upper", 1.0f, 0.005f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Clamp);

			break;
		case (NodeType::Selector):
			name = "Selector";

			AddInputSlot (ConnectionType::Float, "value", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "a", 0.0f, 0.005f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "b", 1.0f, 0.005f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "bottom", 0.5f, 0.005f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "top", 0.75f, 0.005f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "smoothing range", 0.05f, 0.005f, 0.0f, 0.0f);

			internalNodeID = graph.AddNode (InternalGraph::NodeType::Selector);

			break;
		case (NodeType::WhiteNoise):
			name = "WhiteNoise";

			AddInputSlot (ConnectionType::Int, "Seed", 1337);
			AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
			break;
		case (NodeType::ValueNoise):
			name = "ValueNoise";

			AddInputSlot (ConnectionType::Int, "Seed", 1337);
			AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Int, "Octaves", 4, 0.1f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Float, "Persistence", 0.5f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 2);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::ValueNoise);

			break;
		case (NodeType::SimplexNoise):
			name = "SimplexNoise";

			AddInputSlot (ConnectionType::Int, "Seed", 1337);
			AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Int, "Octaves", 4, 0.1f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Float, "Persistence", 0.5f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 2);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::SimplexNoise);

			break;
		case (NodeType::PerlinNoise):
			name = "PerlinNoise";

			AddInputSlot (ConnectionType::Int, "Seed", 1337);
			AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Int, "Octaves", 4, 0.1f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Float, "Persistence", 0.5f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 2);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::PerlinNoise);

			break;
		case (NodeType::CubicNoise):
			name = "CubicNoise";

			AddInputSlot (ConnectionType::Int, "Seed", 1337);
			AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Int, "Octaves", 4, 0.1f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Float, "Persistence", 0.5f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 2);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::CubicNoise);

			break;
		case (NodeType::CellNoise):
			name = "CellNoise";

			AddInputSlot (ConnectionType::Int, "Seed", 1337);
			AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Float, "Jitter", 0.5f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 7);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::CellNoise);

			break;
		case (NodeType::VoronoiNoise):
			name = "VoronoiNoise";

			AddInputSlot (ConnectionType::Int, "Seed", 1337);
			AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
			AddInputSlot (ConnectionType::Float, "Jitter", 0.5f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 7);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::VoronoiNoise);

			break;
		case (NodeType::ConstantInt):
			outputSlot = OutputConnectionSlot (0, ImVec2 (size.x, 15), ConnectionType::Int);

			name = "ConstantInt";

			AddInputSlot (ConnectionType::Int, "Value", 0.0f, 0.1f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::ConstantInt);

			break;
		case (NodeType::ConstantFloat):
			name = "ConstantFloat";

			AddInputSlot (ConnectionType::Float, "Value", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::ConstantFloat);

			break;
		case (NodeType::Invert):
			name = "Invert";

			AddInputSlot (ConnectionType::Float, "Value", 0.0f, 0.01f, 0.0f, 0.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::Invert);

			break;
		case (NodeType::TextureIndex):
			outputSlot = OutputConnectionSlot (0, ImVec2 (size.x, 15), ConnectionType::Int);

			name = "TextureIndex";

			AddInputSlot (ConnectionType::Int, "Index", 0, 0.1f, 0.0f, 255.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::TextureIndex);

			break;
		case (NodeType::FractalReturnType):
			outputSlot = OutputConnectionSlot (0, ImVec2 (size.x, 15), ConnectionType::Int);

			name = "FractalReturnType";

			AddInputSlot (ConnectionType::Int, "FBM-Billow-Rigid", 0, 0.1f, 0, 2);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::FractalReturnType);

			break;
		case (NodeType::CellularReturnType):
			outputSlot = OutputConnectionSlot (0, ImVec2 (size.x, 15), ConnectionType::Int);

			name = "CellularReturnType";

			AddInputSlot (ConnectionType::Int, "Value-Distance-2-Add-Sub-Mul-Div-Cave", 0, 0.1f, 0, 7);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::FractalReturnType);

			break;
		case (NodeType::ColorCreator):
			outputSlot = OutputConnectionSlot (0, ImVec2 (size.x, 15), ConnectionType::Color);

			name = "ColorCreator";

			AddInputSlot (ConnectionType::Float, "Red", 0.0f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Float, "Blue", 0.0f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Float, "Green", 0.0f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Float, "Alpha", 0.0f, 0.01f, 0.0f, 1.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::ColorCreator);

			break;
		case (NodeType::MonoGradient):
			name = "MonoGradient";

			AddInputSlot (ConnectionType::Float, "input", 0.0f, 0.01f, 0.0f, 1.0f);
			AddInputSlot (ConnectionType::Float, "lower bound", 0.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "upper bound", 1.0f, 0.01f, 0.0f, 0.0f);
			AddInputSlot (ConnectionType::Float, "blend factor", 0.0f, 0.01f, 0.0f, 1.0f);
			internalNodeID = graph.AddNode (InternalGraph::NodeType::MonoGradient);

			break;
	}
}