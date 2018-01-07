#include "ProcTerrainNodeGraph.h"

//#include "../../third-party/noc/noc_file_dialog.h"

#include "../core/CoreTools.h"

#include "../core/Input.h"

#include <glm/gtc/type_ptr.hpp>

ProcTerrainNodeGraph::ProcTerrainNodeGraph()
{
	outputNode = std::make_shared<OutputNode>(curGraph);
	outputNode->id = curID++;
	outputNode->internal_node = curGraph.GetOutputNode();
	nodes.push_back(outputNode);
}


ProcTerrainNodeGraph::~ProcTerrainNodeGraph()
{
}

NewNodeGraph::TerGenNodeGraph& ProcTerrainNodeGraph::GetGraph() {
	return curGraph;
}

void ProcTerrainNodeGraph::BuildTerGenNodeGraph() {

}

void ProcTerrainNodeGraph::DeleteNode(std::shared_ptr<Node> node) {
	auto found = std::find(nodes.begin(), nodes.end(), node);
	if (found != nodes.end()) {
		for (auto slot : (*found)->inputSlots) 
			DeleteConnection(slot.connection);
		for(auto slot : (*found)->outputSlot.connections)
			DeleteConnection(slot);

		curGraph.DeleteNode((*found)->internal_node);
		nodes.erase(found);
	}
}
void ProcTerrainNodeGraph::DeleteConnection(std::shared_ptr<Connection> con) {
	auto found = std::find(connections.begin(), connections.end(), con);
	if (found != connections.end()) {
		(*found)->input.reset(); 
		(*found)->output.reset();
		connections.erase(found);
	}
}


void ProcTerrainNodeGraph::Draw() {

	ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(0, 300), ImGuiSetCond_FirstUseEver);


	if (ImGui::Begin("Node Graph", &window_open, ImGuiWindowFlags_MenuBar)) {
		windowPos = ImGui::GetWindowPos();


		DrawMenuBar();
		DrawButtonBar();
		DrawNodeButtons();
		DrawNodeCanvas();


	}
	ImGui::End();
}

void ProcTerrainNodeGraph::DrawMenuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New")) {

			}
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				LoadGraphFromFile();
			}
			if (ImGui::BeginMenu("Open Recent"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {

			}
			if (ImGui::MenuItem("Save As..")) {

			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			//static bool enabled = true;
			//ImGui::MenuItem("Enabled", "", &enabled);
			//ImGui::BeginChild("child", ImVec2(0, 60), true);
			//for (int i = 0; i < 10; i++)
			//	ImGui::Text("Scrolling Text %d", i);
			//ImGui::EndChild();
			//static float f = 0.5f;
			//static int n = 0;
			//ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
			//ImGui::InputFloat("Input", &f, 0.1f);
			//ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void ProcTerrainNodeGraph::DrawButtonBar() {
	ImGui::BeginGroup();

	if (ImGui::Button("Build internal representation")) {
		BuildTerGenNodeGraph();
	}
	ImGui::SameLine();
	if (ImGui::Button("Other Button")) {
		
	}

	ImGui::EndGroup();
}

void ProcTerrainNodeGraph::DrawNodeButtons() {

	ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 2.0f);
	ImGui::BeginChild("Sub2", ImVec2(120, 300), true);

	if (ImGui::Button("Add", ImVec2(-1.0f, 0.0f)))				{ AddNode(NodeType::Addition); }
	if (ImGui::Button("Subtract", ImVec2(-1.0f, 0.0f)))			{ AddNode(NodeType::Subtraction); }
	if (ImGui::Button("Multiply", ImVec2(-1.0f, 0.0f)))			{ AddNode(NodeType::Multiplication); }
	if (ImGui::Button("Divide", ImVec2(-1.0f, 0.0f)))			{ AddNode(NodeType::Division); }
	if (ImGui::Button("Selector", ImVec2(-1.0f, 0.0f)))			{ AddNode(NodeType::Selector); }
	if (ImGui::Button("Perlin Noise", ImVec2(-1.0f, 0.0f)))		{ AddNode(NodeType::Perlin); }
	if (ImGui::Button("Simplex Noise", ImVec2(-1.0f, 0.0f)))	{ AddNode(NodeType::Simplex); }
	if (ImGui::Button("Value Noise", ImVec2(-1.0f, 0.0f)))		{ AddNode(NodeType::ValueNoise); }
	if (ImGui::Button("Voronoi Noise", ImVec2(-1.0f, 0.0f)))	{ AddNode(NodeType::Voroni); }
	if (ImGui::Button("Cellular Noise", ImVec2(-1.0f, 0.0f)))	{ AddNode(NodeType::CellNoise); }
	if (ImGui::Button("Constant Int", ImVec2(-1.0f, 0.0f)))		{ AddNode(NodeType::ConstantInt); }
	if (ImGui::Button("Constant Float", ImVec2(-1.0f, 0.0f)))	{ AddNode(NodeType::ConstantFloat); }

	ImGui::EndChild();
	ImGui::PopStyleVar();

}

void ProcTerrainNodeGraph::DrawNodeCanvas() {
	// Create our child canvas
	//ImGui::Text("Hold middle mouse button to scroll (%.2f,%.2f)", scrolling.x, scrolling.y);

	ImGui::SameLine();

	ImGui::BeginGroup();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(40, 40, 40, 200));
	ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth(120.0f);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->ChannelsSplit(2);

	DrawNodes(draw_list);
	DrawPossibleConnection(draw_list);
	DrawConnections(draw_list);

	draw_list->ChannelsMerge();

	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndGroup();

}

void ProcTerrainNodeGraph::DrawNodes(ImDrawList* imDrawList) {

	for (auto node : nodes) {

		//updates nodes positions, relative to mouseDelta
		if (ImGui::IsMouseDragging(1) && !posCon.isActive) //right mouse click
			node->pos = node->pos + ImGui::GetIO().MouseDelta;


		int node_hovered_in_scene = -1;
		bool open_context_menu = false;

		ImGui::PushID(node->id);
		
		// Display node contents first
		imDrawList->ChannelsSetCurrent(1); // Foreground
		bool old_any_active = ImGui::IsAnyItemActive();

		ImVec2 titleArea = ImVec2(node->size.x, 30.0f);

		// Draw title in center
		ImVec2 textSize = ImGui::CalcTextSize(node->name.c_str());

		ImVec2 textPos = node->pos + windowPadding;
		textPos.x = node->pos.x + (node->size.x / 2) - textSize.x / 2;

		ImGui::SetCursorScreenPos(windowPos + textPos);
		ImGui::Text("%s", node->name.c_str());
		
		//ImGui::SliderFloat("##value", &node->Value, 0.0f, 1.0f, "Alpha %.2f");
		//float dummy_color[3] = { node->Pos.x / ImGui::GetWindowWidth(), node->Pos.y / ImGui::GetWindowHeight(), fmodf((float)node->ID * 0.5f, 1.0f) };
		//ImGui::ColorEdit3("##color", &dummy_color[0]);

		// Save the size of what we have emitted and weither any of the widgets are being used
		bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
		//node->size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;

		// Display node box
		imDrawList->ChannelsSetCurrent(0); // Background

		ImGui::SetCursorScreenPos(windowPos + node->pos);
		ImGui::InvisibleButton("node", titleArea);

		if (ImGui::IsItemHovered())
		{
			node_hovered_in_scene = node->id;
			open_context_menu |= ImGui::IsMouseClicked(1);
		}

		bool node_moving_active = false;

		if (ImGui::IsItemActive() )
			node_moving_active = true;

		ImU32 node_bg_color = (node_hovered_in_scene == node->id) ? ImColor(75, 75, 75) : ImColor(60, 60, 60);
		imDrawList->AddRectFilled(windowPos + node->pos, windowPos + node->pos + node->size, node_bg_color, 4.0f);


		// Draw text bg area
		imDrawList->AddRectFilled(windowPos + node->pos + ImVec2(1, 1), windowPos + node->pos + titleArea, ImColor(100, 0, 0), 4.0f);
		imDrawList->AddRect(windowPos + node->pos, windowPos + node->pos + node->size, ImColor(100, 100, 100), 4.0f);


		ImColor outSlotColor = ImColor(150, 150, 150);

		if (node->outputSlot.IsHoveredOver(windowPos + node->pos))
			outSlotColor = ImColor(200, 200, 200);

		// Draw output circle
		imDrawList->AddCircleFilled(windowPos + node->pos + node->outputSlot.pos, node->outputSlot.nodeSlotRadius, outSlotColor);
			
		/*if (ImGui::BeginPopupContextItem("output context menu"))
		{
			if (ImGui::Selectable("Reset Outputs")) {
				if (node->outputSlot.connections.size() > 0)
					for(auto con : node->outputSlot.connections)
						DeleteConnection(con);
			}
			ImGui::EndPopup();
		}
*/
		int i = 0;
		for (auto slot : node->inputSlots)
		{
			ImGui::SetCursorScreenPos(windowPos + node->pos + slot.pos + ImVec2(10, -slot.nodeSlotRadius));
			ImGui::Text("%s", slot.name.c_str());
			if (ImGui::BeginPopupContextItem("slot context menu"))
			{
				if (ImGui::Selectable("Reset Connection")) {
					if (slot.connection != nullptr)
						DeleteConnection(slot.connection);
				}
				ImGui::PushItemWidth(-1);
				//ImGui::DragFloat("##Value", &value, 0.1f, 0.0f, 0.0f);
				ImGui::PopItemWidth();
				ImGui::EndPopup();
			}

			ImVec2 slotNameSize = ImGui::CalcTextSize(slot.name.c_str());
			ImGui::SetCursorScreenPos(windowPos + node->pos + slot.pos + ImVec2(10 + slotNameSize.x, -slot.nodeSlotRadius));
			//ImGui::InvisibleButton("slot value", slotNameSize);
			
			switch (slot.value.type) {
			case(ConnectionType::Int): 
				ImGui::DragInt("##Value", &(node->inputSlots[i].value.i), 0.1f, 0.0f, 0.0f);
				break;
			case(ConnectionType::Float): 
				ImGui::DragFloat("##Value", &(node->inputSlots[i].value.f), 0.1f, 0.0f, 0.0f);
				break;
			case(ConnectionType::Color): 
				ImGui::DragFloat3("##Value", glm::value_ptr(node->inputSlots[i].value.glm3), 0.1f, 0.0f, 0.0f);
				break;
			case(ConnectionType::Vec2): 
				ImGui::DragFloat2("##Value", glm::value_ptr(node->inputSlots[i].value.glm2), 0.1f, 0.0f, 0.0f);
				break;
			case(ConnectionType::Vec3): 
				ImGui::DragFloat3("##Value", glm::value_ptr(node->inputSlots[i].value.glm3), 0.1f, 0.0f, 0.0f);
				break;
			case(ConnectionType::Vec4): 
				ImGui::DragFloat4("##Value", glm::value_ptr(node->inputSlots[i].value.glm4), 0.1f, 0.0f, 0.0f);
				break;
			}
			

			ImColor conColor = ImColor(150, 150, 150);

			if (slot.hasConnection)
				conColor = ImColor(220, 220, 0);

			if (slot.IsHoveredOver(windowPos + node->pos))
				conColor = ImColor(200, 200, 200);

			imDrawList->AddCircleFilled(windowPos + node->pos + slot.pos, slot.nodeSlotRadius, conColor);
			i++;
		}

		if (node_widgets_active || node_moving_active) {

			//selectedNode = node;
			//node_selected = node->id;
		}

		if (node_moving_active && ImGui::IsMouseDragging(0) && !posCon.isActive)
			node->pos = node->pos + ImGui::GetIO().MouseDelta;

		ImGui::PopID();
	}
}

void ProcTerrainNodeGraph::DrawPossibleConnection(ImDrawList* imDrawList) {
	HoveredSlotInfo info = GetHoveredSlot();
	ImVec2 startPos;
	switch (posCon.state) {
	case(PossibleConnection::State::Default):

		if (info.node != nullptr) {
			posCon.state = PossibleConnection::State::Hover;
			posCon.slot = info;
		}

		break;
	case(PossibleConnection::State::Hover):

		if (info.node == posCon.slot.node) {
			if (ImGui::IsMouseClicked(0) && posCon.slot.node != nullptr) {
				posCon.isActive = true;
				posCon.state = PossibleConnection::State::Dragging;

				if (!info.isOutput && (info.node->inputSlots[info.inputSlotNum].connection != nullptr)) {
					posCon.slot.node->internal_node->ResetInputLink(posCon.slot.inputSlotNum);
					DeleteConnection(info.node->inputSlots[info.inputSlotNum].connection);
				}
			}
		}
		else {
			//posCon.slot.node->internal_node->ResetInputLink(posCon.slot.inputSlotNum);
			posCon.isActive = false;
			posCon.slot = HoveredSlotInfo();
			posCon.state = PossibleConnection::State::Default;
		}

		break;
	case(PossibleConnection::State::Dragging):

		imDrawList->ChannelsSetCurrent(0); // Background

		startPos = windowPos + posCon.slot.node->pos 
			+ PossibleConnection::GetActiveSlotPosition(posCon.slot);

		DrawHermite(imDrawList, startPos, ImGui::GetIO().MousePos, 12);

		if (ImGui::IsMouseClicked(0))
		{
			if (PossibleConnection::GetActiveSlotType(posCon.slot)
				== PossibleConnection::GetActiveSlotType(info)) {

				std::shared_ptr<Connection> newConnection;

				HoveredSlotInfo outgoingSlot;
				HoveredSlotInfo incomingSlot;

				//selection for which is input and output
				if (posCon.slot.isOutput) {
					outgoingSlot = posCon.slot;
					incomingSlot = info;
				}
				else {
					outgoingSlot = info;
					incomingSlot = posCon.slot;
				}

				newConnection = std::make_shared<Connection>(
					PossibleConnection::GetActiveSlotType(posCon.slot),
					outgoingSlot.node,
					incomingSlot.node);

				//Delete old connection
				if (incomingSlot.node->inputSlots[incomingSlot.inputSlotNum].connection == nullptr) {
					incomingSlot.node->inputSlots[incomingSlot.inputSlotNum].connection = newConnection;

				} else {
					DeleteConnection(incomingSlot.node->inputSlots[incomingSlot.inputSlotNum].connection);
					incomingSlot.node->inputSlots[incomingSlot.inputSlotNum].connection = newConnection;
				}
				
				newConnection->startPosRelNode = PossibleConnection::GetActiveSlotPosition(outgoingSlot);
				newConnection->endPosRelNode = PossibleConnection::GetActiveSlotPosition(incomingSlot);

				incomingSlot.node->SetInternalLink(incomingSlot.inputSlotNum, outgoingSlot.node->internal_node);

				connections.push_back(newConnection);

				posCon.isActive = false;
				posCon.slot = HoveredSlotInfo();
				posCon.state = PossibleConnection::State::Default;
				
			}
		}
		else if (ImGui::IsMouseClicked(1))
		{
			posCon.slot.node->internal_node->ResetInputLink(posCon.slot.inputSlotNum);
			posCon.isActive = false;
			posCon.slot = HoveredSlotInfo();
			posCon.state = PossibleConnection::State::Default;
		}
		break;
	default:
		//Something went wrong
		break;
	}
}

HoveredSlotInfo ProcTerrainNodeGraph::GetHoveredSlot() {
	for (std::shared_ptr<Node> node : nodes) {
		for (int i = 0; i < node->inputSlots.size(); i++) {
			if (node->inputSlots[i].IsHoveredOver(windowPos + node->pos))
				return HoveredSlotInfo(node, false, i);
		}
		if (node->outputSlot.IsHoveredOver(windowPos + node->pos))
			return HoveredSlotInfo(node, true, -1);
	}
	return HoveredSlotInfo();
}

void ProcTerrainNodeGraph::DrawHermite(ImDrawList* imDrawList, ImVec2 p1, ImVec2 p2, int STEPS)
{
	ImVec2 t1 = ImVec2(+80.0f, 0.0f);
	ImVec2 t2 = ImVec2(+80.0f, 0.0f);

	for (int step = 0; step <= STEPS; step++)
	{
		float t = (float)step / (float)STEPS;
		float h1 = +2 * t*t*t - 3 * t*t + 1.0f;
		float h2 = -2 * t*t*t + 3 * t*t;
		float h3 = t * t*t - 2 * t*t + t;
		float h4 = t * t*t - t * t;
		imDrawList->PathLineTo(ImVec2(h1*p1.x + h2 * p2.x + h3 * t1.x + h4 * t2.x, h1*p1.y + h2 * p2.y + h3 * t1.y + h4 * t2.y));
	}

	imDrawList->PathStroke(ImColor(200, 200, 100), false, 3.0f);
}

void ProcTerrainNodeGraph::DrawConnections(ImDrawList*  imDrawList) {
	for (auto con : connections) {
		ImVec2 start = windowPos + con->input->pos + con->startPosRelNode;
		ImVec2 end = windowPos + con->output->pos + con->endPosRelNode;

		DrawHermite(imDrawList, start, end, 12);
	}
}

void ProcTerrainNodeGraph::SaveGraphFromFile()
{
	//const char * filename = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, NULL, GetExecutableFilePath().c_str(), NULL);

}

void ProcTerrainNodeGraph::LoadGraphFromFile()
{

	//const char * filename = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, NULL, GetExecutableFilePath().c_str(), NULL);

}

void ProcTerrainNodeGraph::AddNode(NodeType nodeType)
{
	std::shared_ptr<Node> newNode;

	switch (nodeType) {
	case(NodeType::Output): newNode = std::make_shared<OutputNode>(curGraph); break;
	case(NodeType::Addition): newNode = std::make_shared<AdditionNode>(curGraph); break;
	case(NodeType::Subtraction): newNode = std::make_shared<SubtractionNode>(curGraph); break;
	case(NodeType::Multiplication): newNode = std::make_shared<MultiplicationNode>(curGraph); break;
	case(NodeType::Division): newNode = std::make_shared<DivisionNode>(curGraph); break;
	case(NodeType::Power): newNode = std::make_shared<PowerNode>(curGraph); break;
	case(NodeType::Selector): newNode = std::make_shared<SelectorNode>(curGraph); break;
	case(NodeType::Perlin): newNode = std::make_shared<PerlinNode>(curGraph); break;
	case(NodeType::Simplex): newNode = std::make_shared<SimplexNode>(curGraph); break;
	case(NodeType::CellNoise): newNode = std::make_shared<CellNoiseNode>(curGraph); break;
	case(NodeType::ValueNoise): newNode = std::make_shared<ValueNoiseNode>(curGraph); break;
	case(NodeType::Voroni): newNode = std::make_shared<VoroniNode>(curGraph); break;
	case(NodeType::ConstantInt): newNode = std::make_shared<ConstantIntNode>(curGraph); break;
	case(NodeType::ConstantFloat): newNode = std::make_shared<ConstantFloatNode>(curGraph); break;
	}
	
	newNode->id = curID++;
	nodes.push_back(newNode);

}

ConnectionSlot::ConnectionSlot(ImVec2 pos, ConnectionType type): conType(type), pos(pos)
{
}

ConnectionSlot::ConnectionSlot(ImVec2 pos, ConnectionType type, std::string name) : conType(type), name(name), pos(pos)
{
}

bool ConnectionSlot::IsHoveredOver(ImVec2 parentPos)
{
	ImVec2 mousePos = ImGui::GetIO().MousePos;
	ImVec2 conPos = parentPos + pos;

	float xd = mousePos.x - conPos.x;
	float yd = mousePos.y - conPos.y;

	return ((xd * xd) + (yd *yd)) < (nodeSlotRadius * nodeSlotRadius);
}

ConnectionType ConnectionSlot::GetType() {
	return conType;
}

InputConnectionSlot::InputConnectionSlot(ImVec2 pos, ConnectionType type, std::string name) : ConnectionSlot(pos, type, name), value(type)
{
	switch (type) {
		case(ConnectionType::Int): value.i = 0; break;
		case(ConnectionType::Float): value.f = 0.0f;  break;
		case(ConnectionType::Color): value.glm3 = glm::vec3(0); break;
		case(ConnectionType::Vec2): value.glm2 = glm::vec2(0); break;
		case(ConnectionType::Vec3): value.glm3 = glm::vec3(0); break;
		case(ConnectionType::Vec4): value.glm4 = glm::vec4(0); break;
	}
}

OutputConnectionSlot::OutputConnectionSlot(ImVec2 pos, ConnectionType type): ConnectionSlot(pos, type)
{
}


Connection::Connection(ConnectionType conType, std::shared_ptr<Node> input, std::shared_ptr<Node> output)
	: conType(conType), input(input), output(output)
{
}

Node::Node(std::string name, ConnectionType outputType) : name(name), outputSlot(ImVec2(size.x, 15), outputType)
{


}

void Node::SetInternalLink(int index, std::shared_ptr<NewNodeGraph::INode> inode) {
	internal_node->SetInputLink(index, inode);
}

void Node::AddInputSlot(ConnectionType type, std::string name)
{
	inputSlots.push_back(InputConnectionSlot(ImVec2(0, 40 + (float)inputSlots.size() * 15), type, name));
}

OutputNode::OutputNode(NewNodeGraph::TerGenNodeGraph& graph) : Node("Output", ConnectionType::Float)
{
	AddInputSlot(ConnectionType::Float, "Out");
}

SelectorNode::SelectorNode(NewNodeGraph::TerGenNodeGraph& graph) : Node("Selector", ConnectionType::Float)
{
	AddInputSlot(ConnectionType::Float, "A");
	AddInputSlot(ConnectionType::Float, "B");
	AddInputSlot(ConnectionType::Float, "Control");
	internal_node = std::make_shared<NewNodeGraph::SelectorNode>();
	graph.AddNode(internal_node);
}

MathNode::MathNode(std::string name) : Node(name, ConnectionType::Float)
{
	AddInputSlot(ConnectionType::Float, "A");
	AddInputSlot(ConnectionType::Float, "B");
}

AdditionNode::AdditionNode(NewNodeGraph::TerGenNodeGraph& graph) : MathNode("Addition")
{
	internal_node = std::make_shared<NewNodeGraph::AdditionNode>();
	graph.AddNode(internal_node);
}

SubtractionNode::SubtractionNode(NewNodeGraph::TerGenNodeGraph& graph) : MathNode("Subtraction")
{
	internal_node = std::make_shared<NewNodeGraph::SubtractNode>();
	graph.AddNode(internal_node);
}

MultiplicationNode::MultiplicationNode(NewNodeGraph::TerGenNodeGraph& graph) : MathNode("Multiplication")
{
	internal_node = std::make_shared<NewNodeGraph::MultiplyNode>();
	graph.AddNode(internal_node);
}

DivisionNode::DivisionNode(NewNodeGraph::TerGenNodeGraph& graph) : MathNode("Division")
{
	internal_node = std::make_shared<NewNodeGraph::DivideNode>();
	graph.AddNode(internal_node);
}

PowerNode::PowerNode(NewNodeGraph::TerGenNodeGraph& graph) : MathNode("Power")
{
	internal_node = std::make_shared<NewNodeGraph::PowerNode>();
	graph.AddNode(internal_node);
}



NoiseNode::NoiseNode(std::string name) : Node(name, ConnectionType::Float)
{
	AddInputSlot(ConnectionType::Int, "Octaves");
	AddInputSlot(ConnectionType::Float, "Persistance");
	AddInputSlot(ConnectionType::Float, "Frequency");
}

SimplexNode::SimplexNode(NewNodeGraph::TerGenNodeGraph& graph) : NoiseNode("Simplex")
{
	internal_node_noise = std::make_shared<NewNodeGraph::SimplexFractalNoiseNode>();
	internal_node = internal_node_noise;
	graph.AddNoiseSourceNode(internal_node_noise);
}

PerlinNode::PerlinNode(NewNodeGraph::TerGenNodeGraph& graph) : NoiseNode("Perlin")
{
	internal_node_noise = std::make_shared<NewNodeGraph::PerlinFractalNoiseNode>();
	internal_node = internal_node_noise;
	graph.AddNoiseSourceNode(internal_node_noise);
}

VoroniNode::VoroniNode(NewNodeGraph::TerGenNodeGraph& graph) : NoiseNode("Voroni")
{
	internal_node_noise = std::make_shared<NewNodeGraph::VoironiFractalNoiseNode>();
	internal_node = internal_node_noise;
	graph.AddNoiseSourceNode(internal_node_noise);
}

ValueNoiseNode::ValueNoiseNode(NewNodeGraph::TerGenNodeGraph& graph) : NoiseNode("ValueNoise")
{
	internal_node_noise = std::make_shared<NewNodeGraph::ValueFractalNoiseNode>();
	internal_node = internal_node_noise;
	graph.AddNoiseSourceNode(internal_node_noise);
}

CellNoiseNode::CellNoiseNode(NewNodeGraph::TerGenNodeGraph& graph) : NoiseNode("CellNoise")
{
	internal_node_noise = std::make_shared<NewNodeGraph::CellularNoiseNode>();
	internal_node = internal_node_noise;
	graph.AddNoiseSourceNode(internal_node_noise);
}


ConstantIntNode::ConstantIntNode(NewNodeGraph::TerGenNodeGraph& graph) : Node("ConstantInt", ConnectionType::Int)
{
	AddInputSlot(ConnectionType::Int, "Value");
	internal_node = std::make_shared<NewNodeGraph::ConstantIntNode>(1);
	graph.AddNode(internal_node);
}

ConstantFloatNode::ConstantFloatNode(NewNodeGraph::TerGenNodeGraph& graph) : Node("ConstantFloat", ConnectionType::Float)
{
	AddInputSlot(ConnectionType::Float, "Value");
	internal_node = std::make_shared<NewNodeGraph::ConstantFloatNode>(1.0f);
	graph.AddNode(internal_node);
}

PossibleConnection::PossibleConnection() : state(State::Default), slot()
{
}

ImVec2 PossibleConnection::GetActiveSlotPosition(HoveredSlotInfo info) {
	if (info.node != nullptr) {
		if (info.isOutput) {
			return info.node->outputSlot.pos;
		}
		else {
			return info.node->inputSlots[info.inputSlotNum].pos;
		}
	}
	return ImVec2(0, 0);
}

ConnectionType PossibleConnection::GetActiveSlotType(HoveredSlotInfo info) {
	if (info.node != nullptr) {
		if (info.isOutput) {
			return info.node->outputSlot.GetType();
		}
		else {
			return info.node->inputSlots[info.inputSlotNum].GetType();
		}
	}
	return ConnectionType::Null;
}
