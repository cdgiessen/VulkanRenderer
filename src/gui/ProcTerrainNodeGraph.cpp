#include "ProcTerrainNodeGraph.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

#include "noc/noc_file_dialog.h"

#include "core/CoreTools.h"
#include "core/Input.h"
#include "core/Logger.h"

#include <glm/gtc/type_ptr.hpp>

template <typename Enumeration>
auto as_integer (Enumeration const value) -> typename std::underlying_type<Enumeration>::type
{
	return static_cast<typename std::underlying_type<Enumeration>::type> (value);
}

ProcTerrainNodeGraph::ProcTerrainNodeGraph ()
{
	// RecreateOutputNode();
	LoadGraphFromFile ("assets/graphs/default_terrain.json");
}


ProcTerrainNodeGraph::~ProcTerrainNodeGraph () {}

InternalGraph::GraphPrototype& ProcTerrainNodeGraph::GetGraph () { return protoGraph; }

void ProcTerrainNodeGraph::BuildTerGenNodeGraph () {}

void ProcTerrainNodeGraph::DeleteNode (std::shared_ptr<Node> node)
{
	auto found = std::find (nodes.begin (), nodes.end (), node);
	if (found != nodes.end ())
	{

		for (auto& slot : (*found)->inputSlots)
			if (slot.connection != nullptr)
			{
				ResetNodeInternalLinkByID ((*found)->internalNodeID, slot.slotNum);
				DeleteConnection (slot.connection);
			}
		for (auto& con : (*found)->outputSlot.connections)
		{
			DeleteConnection (con);
		}

		protoGraph.DeleteNode ((*found)->internalNodeID);
		nodes.erase (found);
	}
	else
	{
		Log::Error << "Couldn't find node to delete! Does it actually exist?\n";
	}
}

void ProcTerrainNodeGraph::DeleteConnection (std::shared_ptr<Connection> con)
{
	auto found = std::find (connections.begin (), connections.end (), con);
	if (found != connections.end ())
	{

		ResetNodeInternalLinkByID ((*found)->output->internalNodeID, (*found)->output_slot_id);

		//(*found)->input.reset();

		//(*found)->output->internal_node->ResetInputLink(0);
		//(*found)->output.reset();
		connections.erase (found);
	}
	else
	{
		Log::Error << "Couldn't find connection to delete! Does it actually exist?\n";
	}
}

void ProcTerrainNodeGraph::ResetGraph ()
{
	while (nodes.size () > 0)
	{
		DeleteNode (nodes.at (0));
	}
	while (connections.size () > 0)
	{
		DeleteConnection (connections.at (0));
	}
	connections.clear ();
	nodes.clear ();
	protoGraph.ResetGraph ();
	curID = 0;
}

void ProcTerrainNodeGraph::RecreateOutputNode ()
{

	outputNode.reset ();
	outputNode = std::make_shared<OutputNode> (protoGraph);

	outputNode->id = curID++;
	outputNode->type = NodeType::Output;
	outputNode->internalNodeID = protoGraph.AddNode (InternalGraph::NodeType::Output);
	SetNodeInternalValueByID (outputNode->internalNodeID, 0, -0.5f);
	ResetNodeInternalLinkByID (outputNode->internalNodeID, 0);
	nodes.push_back (outputNode);
}

void ProcTerrainNodeGraph::Draw ()
{

	ImGui::SetNextWindowSize (ImVec2 (800, 400), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos (ImVec2 (425, 0), ImGuiSetCond_FirstUseEver);


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
			// static bool enabled = true;
			// ImGui::MenuItem("Enabled", "", &enabled);
			// ImGui::BeginChild("child", ImVec2(0, 60), true);
			// for (int i = 0; i < 10; i++)
			//	ImGui::Text("Scrolling Text %d", i);
			// ImGui::EndChild();
			// static float f = 0.5f;
			// static int n = 0;
			// ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
			// ImGui::InputFloat("Input", &f, 0.1f);
			// ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
			ImGui::EndMenu ();
		}
		ImGui::EndMenuBar ();
	}
}

void ProcTerrainNodeGraph::DrawButtonBar ()
{
	ImGui::BeginGroup ();

	// if (ImGui::Button("Reset Viewport")) {
	//	graphOffset = ImVec2(0, 0);
	//}
	// ImGui::SameLine();
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

	ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 2.0f);
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
		AddNode (NodeType::VoroniNoise, startingNodePos);
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
	ImGui::PushStyleColor (ImGuiCol_ChildWindowBg, ImColor (40, 40, 40, 200));
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

	std::vector<std::shared_ptr<Node>> nodesToDelete;

	for (auto& node : nodes)
	{

		// updates nodes positions, relative to mouseDelta
		if (ImGui::IsMouseDragging (1) && !posCon.isActive) // right mouse click
			node->pos = node->pos + ImGui::GetIO ().MouseDelta;


		int node_hovered_in_scene = -1;
		bool open_context_menu = false;

		ImGui::PushID (node->id);

		// Display node contents first
		imDrawList->ChannelsSetCurrent (1); // Foreground
		bool old_any_active = ImGui::IsAnyItemActive ();

		ImVec2 titleArea = ImVec2 (node->size.x, 30.0f);

		// Draw title in center
		ImVec2 textSize = ImGui::CalcTextSize (node->name.c_str ());

		ImVec2 textPos = node->pos + windowPadding;
		textPos.x = node->pos.x + (node->size.x / 2) - textSize.x / 2;

		ImGui::SetCursorScreenPos (windowPos + textPos);
		ImGui::Text ("%s", node->name.c_str ());

		// ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
		// float dummy_color[3] = { node->Pos.x / ImGui::GetWindowWidth(), node->Pos.y /
		// ImGui::GetWindowHeight(), fmodf((float)node->ID * 0.5f, 1.0f) };
		// ImGui::ColorEdit3("##color", &dummy_color[0]);

		// Save the size of what we have emitted and weither any of the widgets are being used
		bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive ());
		// node->size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;

		// Display node box
		imDrawList->ChannelsSetCurrent (0); // Background

		ImGui::SetCursorScreenPos (windowPos + node->pos);
		ImGui::InvisibleButton ("node", titleArea);

		if (ImGui::IsItemHovered ())
		{
			node_hovered_in_scene = node->id;
			open_context_menu |= ImGui::IsMouseClicked (1);
		}

		bool node_moving_active = false;

		if (ImGui::IsItemActive ()) node_moving_active = true;

		// background
		ImU32 node_bg_color =
		    (node_hovered_in_scene == node->id) ? ImColor (75, 75, 75) : ImColor (60, 60, 60);
		imDrawList->AddRectFilled (
		    windowPos + node->pos, windowPos + node->pos + node->size, node_bg_color, 4.0f);


		// Draw text bg area
		imDrawList->AddRectFilled (
		    windowPos + node->pos + ImVec2 (1, 1), windowPos + node->pos + titleArea, ImColor (100, 0, 0), 4.0f);
		imDrawList->AddRect (
		    windowPos + node->pos, windowPos + node->pos + node->size, ImColor (100, 100, 100), 4.0f);

		if (ImGui::BeginPopupContextItem ("slot context menu"))
		{
			if (ImGui::Selectable ("Delete Node"))
			{
				nodesToDelete.push_back (node);
			}
			ImGui::Text ("Edit name:");
			char name[32];
			memset (name, '\0', 32);
			node->name.copy (name, 31);
			if (node->hasTextInput == false)
			{
				Input::SetTextInputMode ();
				node->hasTextInput = true;
			}
			if (ImGui::InputText ("##edit", name, sizeof (char) * 32))
			{
				name[31] = '\0';
				node->name.replace (node->name.begin (), node->name.end (), name);
			}
			else
			{
			}
			ImGui::EndPopup ();
		}
		else
		{
			if (node->hasTextInput)
			{
				Input::ResetTextInputMode ();
				node->hasTextInput = false;
			}
		}

		node->outputSlot.Draw (imDrawList, *this, *node, 0);

		float verticalOffset = 40;
		for (int i = 0; i < node->inputSlots.size (); i++)
		{
			verticalOffset += node->inputSlots[i].Draw (imDrawList, *this, *node, verticalOffset);
		}
		node->size = ImVec2 (node->size.x, verticalOffset);
		//		for (auto& slot : node->inputSlots) {
		//			slot.Draw(imDrawList, *this, *node);
		//		}

		if (node_widgets_active || node_moving_active)
		{

			// selectedNode = node;
			// node_selected = node->id;
		}

		if (node_moving_active && ImGui::IsMouseDragging (0) && !posCon.isActive)
			node->pos = node->pos + ImGui::GetIO ().MouseDelta;

		ImGui::PopID ();
	}

	while (nodesToDelete.size () > 0)
	{
		if (nodesToDelete.at (0)->hasTextInput)
		{
			Input::ResetTextInputMode ();
		}
		DeleteNode (nodesToDelete.at (0));
		nodesToDelete.erase (nodesToDelete.begin ());
	}
}

void ProcTerrainNodeGraph::DrawPossibleConnection (ImDrawList* imDrawList)
{
	HoveredSlotInfo info = GetHoveredSlot ();
	ImVec2 startPos;
	switch (posCon.state)
	{
		case (PossibleConnection::State::Default):

			if (info.node != nullptr)
			{
				posCon.state = PossibleConnection::State::Hover;
				posCon.slot = info;
			}

			break;
		case (PossibleConnection::State::Hover):

			if (info.node == posCon.slot.node)
			{
				if (ImGui::IsMouseClicked (0) && posCon.slot.node != nullptr)
				{
					posCon.isActive = true;
					posCon.state = PossibleConnection::State::Dragging;

					if (!info.isOutput && (info.node->inputSlots[info.inputSlotNum].connection != nullptr))
					{
						ResetNodeInternalLinkByID (posCon.slot.node->internalNodeID, posCon.slot.inputSlotNum);
						DeleteConnection (info.node->inputSlots[info.inputSlotNum].connection);
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

			startPos = windowPos + posCon.slot.node->pos +
			           PossibleConnection::GetActiveSlotPosition (posCon.slot);

			DrawHermite (imDrawList, startPos, ImGui::GetIO ().MousePos, 12);

			if (ImGui::IsMouseClicked (0))
			{
				if (PossibleConnection::GetActiveSlotType (posCon.slot) ==
				        PossibleConnection::GetActiveSlotType (info) &&
				    posCon.slot.node != info.node)
				{

					std::shared_ptr<Connection> newConnection;

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
					if (incomingSlot.node->inputSlots[incomingSlot.inputSlotNum].connection != nullptr)
					{

						ResetNodeInternalLinkByID (incomingSlot.node->internalNodeID, incomingSlot.inputSlotNum);
						DeleteConnection (incomingSlot.node->inputSlots[incomingSlot.inputSlotNum].connection);
					}

					newConnection =
					    std::make_shared<Connection> (PossibleConnection::GetActiveSlotType (posCon.slot),
					        outgoingSlot.node,
					        incomingSlot.node,
					        incomingSlot.inputSlotNum);

					incomingSlot.node->inputSlots[incomingSlot.inputSlotNum].connection = newConnection;
					outgoingSlot.node->outputSlot.connections.push_back (newConnection);

					newConnection->startPosRelNode = PossibleConnection::GetActiveSlotPosition (outgoingSlot);
					newConnection->endPosRelNode = PossibleConnection::GetActiveSlotPosition (incomingSlot);

					// incomingSlot.node->SetInternalLink(incomingSlot.inputSlotNum, outgoingSlot.node->internalNodeID);
					SetNodeInternalLinkByID (incomingSlot.node->internalNodeID,
					    incomingSlot.inputSlotNum,
					    outgoingSlot.node->internalNodeID);
					connections.push_back (newConnection);

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
					ResetNodeInternalLinkByID (posCon.slot.node->internalNodeID, posCon.slot.inputSlotNum);
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
	for (std::shared_ptr<Node> node : nodes)
	{
		for (int i = 0; i < node->inputSlots.size (); i++)
		{
			if (node->inputSlots[i].IsHoveredOver (windowPos + node->pos))
				return HoveredSlotInfo (node, false, i);
		}
		if (node->outputSlot.IsHoveredOver (windowPos + node->pos))
			return HoveredSlotInfo (node, true, -1);
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
	for (auto& con : connections)
	{
		con->endPosRelNode = con->output->inputSlots.at (con->output_slot_id).pos;
		ImVec2 start = windowPos + con->input->pos + con->startPosRelNode;
		ImVec2 end = windowPos + con->output->pos + con->endPosRelNode;

		DrawHermite (imDrawList, start, end, 12);
	}
}


void ProcTerrainNodeGraph::SaveGraphFromFile ()
{
	const char* filename =
	    noc_file_dialog_open (NOC_FILE_DIALOG_SAVE, NULL, GetExecutableFilePath ().c_str (), NULL);
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
		Log::Debug << "Bad file name for terrain graph!\n";
		return;
	}
	nlohmann::json j;

	j["numNodes"] = nodes.size ();

	int curNodeIndex = 0;
	for (auto& node : nodes)
	{
		nlohmann::json nodeJson;
		nodeJson["id"] = node->id;
		nodeJson["nodeType"] = as_integer (node->type);
		nodeJson["winPosX"] = node->pos.x;
		nodeJson["winPosY"] = node->pos.y;
		nodeJson["numSlots"] = node->inputSlots.size ();

		for (auto& slot : node->inputSlots)
		{
			nlohmann::json slotJson;
			slotJson["slotName"] = slot.name;
			slotJson["slotType"] = as_integer (slot.conType);
			if (slot.connection == nullptr)
			{
				slotJson["hasConnection"] = false;
				if (slot.value.type == ConnectionType::Int)
					slotJson["value"] = std::get<int> (slot.value.value);
				else if (slot.value.type == ConnectionType::Float)
					slotJson["value"] = std::get<float> (slot.value.value);

				else if (slot.value.type == ConnectionType::Vec2)
				{
					glm::vec2 vec2 = std::get<glm::vec2> (slot.value.value);
					slotJson["value"] = { vec2.x, vec2.y };
				}
				else if (slot.value.type == ConnectionType::Vec3)
				{
					glm::vec3 vec3 = std::get<glm::vec3> (slot.value.value);
					slotJson["value"] = { vec3.x, vec3.y, vec3.z };
				}
				else if (slot.value.type == ConnectionType::Vec4 || slot.value.type == ConnectionType::Color)
				{
					glm::vec4 vec4 = std::get<glm::vec4> (slot.value.value);
					slotJson["value"] = { vec4.x, vec4.y, vec4.z, vec4.w };
				}
			}
			else
			{
				slotJson["hasConnection"] = true;
				slotJson["value"] = slot.connection->input->id;
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
	const char* filename =
	    noc_file_dialog_open (NOC_FILE_DIALOG_OPEN, NULL, GetExecutableFilePath ().c_str (), NULL);
	if (filename != NULL)
	{
		LoadGraphFromFile (std::string (filename));
	}
	else
	{
		Log::Debug << "User didn't specify file, Aborting load\n";
	}
}

void ProcTerrainNodeGraph::LoadGraphFromFile (std::string fileName)
{

	std::ifstream inFile (fileName);
	if (!inFile)
	{
		Log::Debug << "Bad file name for terrain graph!\n";
		return;
	}

	nlohmann::json j;

	if (inFile.peek () == std::ifstream::traits_type::eof ())
	{
		Log::Error << "Opened graph is empty! Did something go wrong?\n";
		return;
	}

	try
	{
		inFile >> j;
	}
	catch (nlohmann::json::parse_error& e)
	{
		Log::Error << e.what () << "\n";
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
			auto node = GetNodeById (j[curIndex]["id"]);
			for (int slot = 0; slot < node->inputSlots.size (); slot++)
			{
				std::string curSlotIndex (std::to_string (slot));

				bool hasCon = j[curIndex][curSlotIndex]["hasConnection"];
				if (hasCon)
				{
					int conIndex = j[curIndex][curSlotIndex]["value"];

					auto outGoingNode = GetNodeById (conIndex);
					if (outGoingNode == nullptr)
					{
						Log::Error << "Couldn't find node by id in graph loading!\n";
					}

					int slotType = j[curIndex][curSlotIndex]["slotType"];
					std::shared_ptr<Connection> newConnection = std::make_shared<Connection> (
					    static_cast<ConnectionType> (slotType), outGoingNode, node, slot);

					node->inputSlots[slot].connection = newConnection;
					outGoingNode->outputSlot.connections.push_back (newConnection);

					newConnection->startPosRelNode = outGoingNode->outputSlot.pos;
					newConnection->endPosRelNode = node->inputSlots[slot].pos;

					// node->SetInternalLink(slot, outGoingNode->internal_node);
					SetNodeInternalLinkByID (node->internalNodeID, slot, outGoingNode->internalNodeID);

					connections.push_back (newConnection);
				}
				else
				{
					int slotType = j[curIndex][curSlotIndex]["slotType"];
					ConnectionType type = static_cast<ConnectionType> (slotType);

					if (type == ConnectionType::Int)
					{
						int valInt = j[curIndex][curSlotIndex]["value"];
						node->inputSlots[slot].value.value = valInt;
					}
					if (type == ConnectionType::Float)
					{
						float valFloat = j[curIndex][curSlotIndex]["value"];
						node->inputSlots[slot].value.value = valFloat;
					}
					if (type == ConnectionType::Vec2)
					{
						float arr[2];
						arr[0] = j[curIndex][curSlotIndex]["value"];
						node->inputSlots[slot].value.value = glm::make_vec2 (arr);
					}
					if (type == ConnectionType::Vec3)
					{
						float arr[3];
						arr[0] = j[curIndex][curSlotIndex]["value"];
						node->inputSlots[slot].value.value = glm::make_vec3 (arr);
					}
					if (type == ConnectionType::Vec4 || type == ConnectionType::Color)
					{
						float arr[4];
						arr[0] = j[curIndex][curSlotIndex]["value"];
						node->inputSlots[slot].value.value = glm::make_vec4 (arr);
					}
				}
			}
		}
	}
	catch (nlohmann::json::parse_error& e)
	{
		Log::Error << e.what () << "\n";
	}
}

void ProcTerrainNodeGraph::AddNode (NodeType nodeType, ImVec2 position, int id)
{
	if (nodeType == NodeType::Output)
	{

		outputNode.reset ();
		outputNode = std::make_shared<OutputNode> (protoGraph);

		if (id >= 0)
			outputNode->id = id;
		else
			outputNode->id = curID;
		curID++;

		outputNode->type = NodeType::Output;
		outputNode->pos = position;
		outputNode->internalNodeID = protoGraph.AddNode (InternalGraph::NodeType::Output);
		SetNodeInternalValueByID (outputNode->internalNodeID, 0, -0.5f);
		ResetNodeInternalLinkByID (outputNode->internalNodeID, 0);
		nodes.push_back (outputNode);
		return;
	}

	std::shared_ptr<Node> newNode;


	switch (nodeType)
	{
		case (NodeType::Addition):
			newNode = std::make_shared<AdditionNode> (protoGraph);
			break;
		case (NodeType::Subtraction):
			newNode = std::make_shared<SubtractionNode> (protoGraph);
			break;
		case (NodeType::Multiplication):
			newNode = std::make_shared<MultiplicationNode> (protoGraph);
			break;
		case (NodeType::Division):
			newNode = std::make_shared<DivisionNode> (protoGraph);
			break;
		case (NodeType::Power):
			newNode = std::make_shared<PowerNode> (protoGraph);
			break;
		case (NodeType::Max):
			newNode = std::make_shared<MaxNode> (protoGraph);
			break;
		case (NodeType::Min):
			newNode = std::make_shared<MinNode> (protoGraph);
			break;
		case (NodeType::Blend):
			newNode = std::make_shared<BlendNode> (protoGraph);
			break;
		case (NodeType::Clamp):
			newNode = std::make_shared<ClampNode> (protoGraph);
			break;
		case (NodeType::Selector):
			newNode = std::make_shared<SelectorNode> (protoGraph);
			break;

		case (NodeType::WhiteNoise):
			newNode = std::make_shared<WhiteNoiseNode> (protoGraph);
			break;
		case (NodeType::PerlinNoise):
			newNode = std::make_shared<PerlinNode> (protoGraph);
			break;
		case (NodeType::SimplexNoise):
			newNode = std::make_shared<SimplexNode> (protoGraph);
			break;
		case (NodeType::ValueNoise):
			newNode = std::make_shared<ValueNode> (protoGraph);
			break;
		case (NodeType::CubicNoise):
			newNode = std::make_shared<CubicNode> (protoGraph);
			break;
		case (NodeType::FractalReturnType):
			newNode = std::make_shared<FractalReturnType> (protoGraph);
			break;

		case (NodeType::CellNoise):
			newNode = std::make_shared<CellNoiseNode> (protoGraph);
			break;
		case (NodeType::VoroniNoise):
			newNode = std::make_shared<VoroniNode> (protoGraph);
			break;
		case (NodeType::CellularReturnType):
			newNode = std::make_shared<CellularReturnType> (protoGraph);
			break;

		case (NodeType::ConstantInt):
			newNode = std::make_shared<ConstantIntNode> (protoGraph);
			break;
		case (NodeType::ConstantFloat):
			newNode = std::make_shared<ConstantFloatNode> (protoGraph);
			break;
		case (NodeType::Invert):
			newNode = std::make_shared<InvertNode> (protoGraph);
			break;
		case (NodeType::TextureIndex):
			newNode = std::make_shared<TextureIndexNode> (protoGraph);
			break;
		case (NodeType::ColorCreator):
			newNode = std::make_shared<ColorCreator> (protoGraph);
			break;
		case (NodeType::MonoGradient):
			newNode = std::make_shared<MonoGradient> (protoGraph);
			break;
	}
	if (id >= 0)
		newNode->id = id;
	else
		newNode->id = curID;
	curID++;

	newNode->type = nodeType;
	newNode->pos = position;
	nodes.push_back (newNode);
}

std::shared_ptr<Node> ProcTerrainNodeGraph::GetNodeById (int id)
{
	for (auto& node : nodes)
	{
		if (id == node->id)
		{
			return node;
		}
	}
	return nullptr;
}

ConnectionSlot::ConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type)
: slotNum (slotNum), conType (type), pos (pos)
{
}

ConnectionSlot::ConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type, std::string name)
: slotNum (slotNum), conType (type), pos (pos), name (name)
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
: ConnectionSlot (slotNum, pos, type, name),
  value (type),
  sliderStepSize (sliderStepSize),
  lowerBound (lowerBound),
  upperBound (upperBound)
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
			value.value = glm::vec4 (0);
			break;
		case (ConnectionType::Vec2):
			value.value = glm::vec2 (0);
			break;
		case (ConnectionType::Vec3):
			value.value = glm::vec3 (0);
			break;
		case (ConnectionType::Vec4):
			value.value = glm::vec4 (0);
			break;
	}
}

InputConnectionSlot::InputConnectionSlot (int slotNum,
    ImVec2 pos,
    ConnectionType type,
    std::string name,
    std::variant<int, float, glm::vec2, glm::vec3, glm::vec4> defaultValue)
: ConnectionSlot (slotNum, pos, type, name), value (type)
{
	value.value = defaultValue;
}

InputConnectionSlot::InputConnectionSlot (int slotNum,
    ImVec2 pos,
    ConnectionType type,
    std::string name,
    std::variant<int, float, glm::vec2, glm::vec3, glm::vec4> defaultValue,
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
	pos = ImVec2 (0, verticalOffset);
	ImVec2 currentPos = relPos + pos;
	int slotHeight = 35;

	ImGui::SetCursorScreenPos (currentPos + ImVec2 (10, -nodeSlotRadius));
	ImGui::Text ("%s", name.c_str ());
	// if (ImGui::BeginPopupContextItem("slot context menu"))
	//{
	//	if (ImGui::Selectable("Reset Connection")) {
	//		if (slot.connection != nullptr)
	//			//TODO
	//			//graph.DeleteConnection(slot.connection);
	//	}
	//	ImGui::EndPopup();
	//}

	ImVec2 slotNameSize = ImGui::CalcTextSize (name.c_str ());
	ImGui::SetCursorScreenPos (currentPos + ImVec2 (5, -nodeSlotRadius + 15));
	// ImGui::InvisibleButton("slot value", slotNameSize);

	ImGui::PushItemWidth (parentNode.size.x - 10);
	std::string uniqueID = (std::to_string (parentNode.id * 100 + slotNum));
	switch (value.type)
	{
		case (ConnectionType::Int):
			ImGui::DragInt (std::string ("##int" + uniqueID).c_str (),
			    &(std::get<int> (value.value)),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
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
			    glm::value_ptr (std::get<glm::vec2> (value.value)),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		case (ConnectionType::Vec3):
			ImGui::DragFloat3 (std::string ("##vec3" + uniqueID).c_str (),
			    glm::value_ptr (std::get<glm::vec3> (value.value)),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		case (ConnectionType::Vec4):
			ImGui::DragFloat4 (std::string ("##vec4" + uniqueID).c_str (),
			    glm::value_ptr (std::get<glm::vec4> (value.value)),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
		case (ConnectionType::Color):
			ImGui::DragFloat4 (std::string ("##color" + uniqueID).c_str (),
			    glm::value_ptr (std::get<glm::vec4> (value.value)),
			    sliderStepSize,
			    lowerBound,
			    upperBound);
			break;
	}
	graph.SetNodeInternalValueByID (parentNode.internalNodeID, slotNum, value.value);
	ImGui::PopItemWidth ();

	ImColor conColor = ImColor (150, 150, 150);

	if (connection != nullptr) conColor = ImColor (210, 210, 0);

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

Connection::Connection (
    ConnectionType conType, std::shared_ptr<Node> input, std::shared_ptr<Node> output, int output_slot_id)
: conType (conType), input (input), output (output), output_slot_id (output_slot_id)
{
}

Node::Node (std::string name, ConnectionType outputType)
: name (name), outputSlot (0, ImVec2 (size.x, 15), outputType)
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
// void Node::SetInternalLink(int index, InternalGraph::NodeID id) {
//	->SetInputLink(index, id);
//}

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

void Node::AddInputSlot (ConnectionType type, std::string name, glm::vec2 defaultValue)
{
	inputSlots.push_back (InputConnectionSlot ((int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue));
}

void Node::AddInputSlot (ConnectionType type, std::string name, glm::vec3 defaultValue)
{
	inputSlots.push_back (InputConnectionSlot ((int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue));
}

void Node::AddInputSlot (ConnectionType type, std::string name, glm::vec4 defaultValue)
{
	inputSlots.push_back (InputConnectionSlot ((int)inputSlots.size (), ImVec2 (0, 40), type, name, defaultValue));
}

OutputNode::OutputNode (InternalGraph::GraphPrototype& graph)
: Node ("Output", ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "HeightMap", -0.5f);
	AddInputSlot (ConnectionType::Color, "Splatmap", glm::vec4 (0));

	AddInputSlot (ConnectionType::Int, "Texture 1", 0);
	AddInputSlot (ConnectionType::Int, "Texture 2", 1);
	AddInputSlot (ConnectionType::Int, "Texture 3", 2);
	AddInputSlot (ConnectionType::Int, "Texture 4", 3);
}

BlendNode::BlendNode (InternalGraph::GraphPrototype& graph) : Node ("Blend", ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "B", 1.0f, 0.01f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "Factor", 0.5f, 0.005f, 0.0f, 1.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Blend));
}

ClampNode::ClampNode (InternalGraph::GraphPrototype& graph) : Node ("Clamp", ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "input", 0.0f, 0.01f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "lower", 0.0f, 0.005f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "upper", 1.0f, 0.005f, 0.0f, 0.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Clamp));
}

SelectorNode::SelectorNode (InternalGraph::GraphPrototype& graph)
: Node ("Selector", ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "value", 0.0f, 0.01f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "a", 0.0f, 0.005f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "b", 1.0f, 0.005f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "bottom", 0.5f, 0.005f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "top", 0.75f, 0.005f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "smoothing range", 0.05f, 0.005f, 0.0f, 0.0f);

	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Selector));
}

// Maths

MathNode::MathNode (std::string name) : Node (name, ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "A", 0.0f, 0.01f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "B", 0.0f, 0.01f, 0.0f, 0.0f);
}

AdditionNode::AdditionNode (InternalGraph::GraphPrototype& graph) : MathNode ("Addition")
{
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Addition));
}

SubtractionNode::SubtractionNode (InternalGraph::GraphPrototype& graph) : MathNode ("Subtraction")
{
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Subtraction));
}

MultiplicationNode::MultiplicationNode (InternalGraph::GraphPrototype& graph)
: MathNode ("Multiplication")
{
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Multiplication));
}

DivisionNode::DivisionNode (InternalGraph::GraphPrototype& graph) : MathNode ("Division")
{
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Division));
}

PowerNode::PowerNode (InternalGraph::GraphPrototype& graph) : MathNode ("Power")
{
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Power));
}

MaxNode::MaxNode (InternalGraph::GraphPrototype& graph) : MathNode ("Max")
{
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Max));
}

MinNode::MinNode (InternalGraph::GraphPrototype& graph) : MathNode ("Min")
{
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Min));
}

// Noise Nodes

NoiseNode::NoiseNode (std::string name) : Node (name, ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Int, "Seed", 1337);
	AddInputSlot (ConnectionType::Float, "Frequency", 2.0f, 0.001f, 0.0f, 10.0f);
}

WhiteNoiseNode::WhiteNoiseNode (InternalGraph::GraphPrototype& graph) : NoiseNode ("White Noise")
{
	internalNodeID = graph.AddNoiseNoide (InternalGraph::Node (InternalGraph::NodeType::WhiteNoise));
}

// Fractal noises

FractalNoiseNode::FractalNoiseNode (std::string name) : NoiseNode (name)
{
	AddInputSlot (ConnectionType::Int, "Octaves", 4, 0.1f, 0.0f, 10.0f);
	AddInputSlot (ConnectionType::Float, "Persistence", 0.5f, 0.01f, 0.0f, 1.0f);
	AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 2);
}

SimplexNode::SimplexNode (InternalGraph::GraphPrototype& graph) : FractalNoiseNode ("Simplex")
{
	internalNodeID = graph.AddNoiseNoide (InternalGraph::Node (InternalGraph::NodeType::SimplexNoise));
}

PerlinNode::PerlinNode (InternalGraph::GraphPrototype& graph) : FractalNoiseNode ("Perlin")
{
	internalNodeID = graph.AddNoiseNoide (InternalGraph::Node (InternalGraph::NodeType::PerlinNoise));
}

ValueNode::ValueNode (InternalGraph::GraphPrototype& graph) : FractalNoiseNode ("Value")
{
	internalNodeID = graph.AddNoiseNoide (InternalGraph::Node (InternalGraph::NodeType::ValueNoise));
}

CubicNode::CubicNode (InternalGraph::GraphPrototype& graph) : FractalNoiseNode ("Cubic")
{
	internalNodeID = graph.AddNoiseNoide (InternalGraph::Node (InternalGraph::NodeType::CubicNoise));
}

// Cellular noises

CellularNoiseNode::CellularNoiseNode (std::string name) : NoiseNode (name)
{
	AddInputSlot (ConnectionType::Float, "Jitter", 0.5f, 0.01f, 0.0f, 1.0f);
	AddInputSlot (ConnectionType::Int, "Type", 0, 0.1f, 0, 7);
}

VoroniNode::VoroniNode (InternalGraph::GraphPrototype& graph) : CellularNoiseNode ("Voroni")
{
	internalNodeID = graph.AddNoiseNoide (InternalGraph::Node (InternalGraph::NodeType::VoroniNoise));
}

CellNoiseNode::CellNoiseNode (InternalGraph::GraphPrototype& graph) : CellularNoiseNode ("Cellular")
{
	internalNodeID = graph.AddNoiseNoide (InternalGraph::Node (InternalGraph::NodeType::CellNoise));
}

// Utilities

ConstantIntNode::ConstantIntNode (InternalGraph::GraphPrototype& graph)
: Node ("ConstantInt", ConnectionType::Int)
{
	AddInputSlot (ConnectionType::Int, "Value", 0.0f, 0.1f, 0.0f, 0.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::ConstantInt));
}

ConstantFloatNode::ConstantFloatNode (InternalGraph::GraphPrototype& graph)
: Node ("ConstantFloat", ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "Value", 0.0f, 0.01f, 0.0f, 0.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::ConstantFloat));
}

InvertNode::InvertNode (InternalGraph::GraphPrototype& graph)
: Node ("Inverter", ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "Value", 0.0f, 0.01f, 0.0f, 0.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::Invert));
}

TextureIndexNode::TextureIndexNode (InternalGraph::GraphPrototype& graph)
: Node ("Texture Index", ConnectionType::Int)
{
	AddInputSlot (ConnectionType::Int, "Index", 0, 0.1f, 0.0f, 255.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::TextureIndex));
}

FractalReturnType::FractalReturnType (InternalGraph::GraphPrototype& graph)
: Node ("Fractal Type", ConnectionType::Int)
{
	AddInputSlot (ConnectionType::Int, "FBM-Billow-Rigid", 0, 0.1f, 0, 2);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::FractalReturnType));
}

CellularReturnType::CellularReturnType (InternalGraph::GraphPrototype& graph)
: Node ("Cellular Type", ConnectionType::Int)
{
	// CellValue, Distance, Distance2, Distance2Add, Distance2Sub, Distance2Mul, Distance2Div, NoiseLookup, Distance2Cave
	AddInputSlot (ConnectionType::Int, "Value-Distance-2-Add-Sub-Mul-Div-Cave", 0, 0.1f, 0, 7);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::FractalReturnType));
}

// Colors

ColorCreator::ColorCreator (InternalGraph::GraphPrototype& graph)
: Node ("ColorCreator", ConnectionType::Color)
{
	AddInputSlot (ConnectionType::Float, "Red", 0.0f, 0.01f, 0.0f, 1.0f);
	AddInputSlot (ConnectionType::Float, "Blue", 0.0f, 0.01f, 0.0f, 1.0f);
	AddInputSlot (ConnectionType::Float, "Green", 0.0f, 0.01f, 0.0f, 1.0f);
	AddInputSlot (ConnectionType::Float, "Alpha", 0.0f, 0.01f, 0.0f, 1.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::ColorCreator));
}

MonoGradient::MonoGradient (InternalGraph::GraphPrototype& graph)
: Node ("MonoGradient", ConnectionType::Float)
{
	AddInputSlot (ConnectionType::Float, "input", 0.0f, 0.01f, 0.0f, 1.0f);
	AddInputSlot (ConnectionType::Float, "lower bound", 0.0f, 0.01f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "upper bound", 1.0f, 0.01f, 0.0f, 0.0f);
	AddInputSlot (ConnectionType::Float, "blend factor", 0.0f, 0.01f, 0.0f, 1.0f);
	internalNodeID = graph.AddNode (InternalGraph::Node (InternalGraph::NodeType::MonoGradient));
}

PossibleConnection::PossibleConnection () : state (State::Default), slot () {}

ImVec2 PossibleConnection::GetActiveSlotPosition (HoveredSlotInfo info)
{
	if (info.node != nullptr)
	{
		if (info.isOutput)
		{
			return info.node->outputSlot.pos;
		}
		else
		{
			return info.node->inputSlots[info.inputSlotNum].pos;
		}
	}
	return ImVec2 (0, 0);
}

ConnectionType PossibleConnection::GetActiveSlotType (HoveredSlotInfo info)
{
	if (info.node != nullptr)
	{
		if (info.isOutput)
		{
			return info.node->outputSlot.GetType ();
		}
		else
		{
			return info.node->inputSlots[info.inputSlotNum].GetType ();
		}
	}
	return ConnectionType::Null;
}
