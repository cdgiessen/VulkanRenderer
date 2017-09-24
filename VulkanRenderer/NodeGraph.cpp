#include "NodeGraph.h"

#include <iostream>
#include <string>

//Does a rect bound check with the pos and size of the box
bool AABBCheck(ImVec2 pos, ImVec2 size, ImVec2 point) {
	return point.x >= pos.x && point.y >= pos.y && point.x <= pos.x + size.x && point.y <= pos.y + size.y;
}

void Connector::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	draw_list->AddBezierCurve(startNode->bodyRect.pos + start + canvas_pos, startNode->bodyRect.pos + startPivot + canvas_pos, endNode->bodyRect.pos + endPivot + canvas_pos, endNode->bodyRect.pos + end + canvas_pos, color, 1);
}

//Sets connection from output
void Connector::SetStart(Node* node) {
	startNode = node;
	
	start = node->outputRect.pos + ImVec2(5, 5);
	startPivot = start + ImVec2(15, 0);
}

//Sets connection to input
void Connector::SetEnd(Node* node, Rectangle inputRect) {
	endNode = node;
	end = inputRect.pos + ImVec2(5, 5);
	endPivot = end + ImVec2(-15, 0);
}

Node::Node(std::string name, std::string description) : name(name), description(description) {
	bodyRect = Rectangle(ImVec2(10, 10), ImVec2(50, 50));
}

void Node::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double Node::GetValue(double x, double y, double z) {
	return 0.0;
}

void Node::AddInputConnector(Connector* connector) {
	inputConnections.push_back(connector);
}

void Node::AddOutputConnector(Connector *connector) {
	inputConnections.push_back(connector);
}

void Node::RemoveInputConnector(Connector* connector) {
	auto con = std::find(inputConnections.begin(), inputConnections.end(), connector);
	if(con != inputConnections.end())
		inputConnections.erase(con);
}

void Node::RemoveOutputConnector(Connector* connector) {
	auto con = std::find(inputConnections.begin(), inputConnections.end(), connector);
	if (con != inputConnections.end())
		inputConnections.erase(con);
}

void Node::Update(ImVec2 mouse_pos_in_canvas, bool isDraggingWidget) {
	
	if (AABBCheck(bodyRect.pos, bodyRect.size, mouse_pos_in_canvas)) {
		if (ImGui::IsMouseDragging(0) && !isDraggingWidget) {
			bodyRect.pos = ImVec2(mouse_pos_in_canvas.x - bodyRect.size.x / 2, mouse_pos_in_canvas.y - bodyRect.size.y / 2);
			isDraggingWidget = true;
		}
	}
}

void Node::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	clip_rect = ImVec4(canvas_pos.x + bodyRect.pos.x, canvas_pos.y + bodyRect.pos.y, canvas_pos.x + bodyRect.pos.x + bodyRect.size.x, canvas_pos.y + bodyRect.pos.y + bodyRect.size.y);

	bodyRect.draw(draw_list, canvas_pos);
	outputRect.draw(draw_list, canvas_pos, bodyRect.pos);
	for (Rectangle rect : inputRects) {
		rect.draw(draw_list, canvas_pos, bodyRect.pos);
	}
}

OutputNode::OutputNode() : Node("Output", "End point for entire graph") {
	outputType = LinkType::Float;
	inputTypes = { LinkType::Float };
	outputRect = Rectangle(ImVec2(bodyRect.size.x, 0), ImVec2(10, 10));
	inputRects.push_back(Rectangle(ImVec2(-10, 0), ImVec2(10, 10)));
}

void OutputNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double OutputNode::GetValue(double x, double y, double z) {
	if (inputs.size() <= 0)
		return 0;
	else
		return inputs.at(0)->GetValue(x, y, z);
}

void OutputNode::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	Node::Draw(draw_list, canvas_pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + bodyRect.pos.x, canvas_pos.y + bodyRect.pos.y), ImColor(255, 255, 255, 255), " Out", NULL, 0.0f, &clip_rect);

}

ConstantNode::ConstantNode() : Node("Constant", "Holds a value") {
	outputRect = Rectangle(ImVec2(bodyRect.size.x, 0), ImVec2(10, 10));
}

double ConstantNode::GetValue(double x, double y, double z) {
	return value;
}

void ConstantNode::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	Node::Draw(draw_list, canvas_pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), canvas_pos + bodyRect.pos, ImColor(255, 255, 255, 255), " Val", NULL, 0.0f, &clip_rect);
	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), canvas_pos + bodyRect.pos + ImVec2(0, 10), ImColor(255, 255, 255, 255), std::to_string(value).c_str(), NULL, 0.0f, &clip_rect);
}

AdditionNode::AdditionNode() : Node("Addition", "Adds two numbers together") {
	outputType = LinkType::Float;
	inputTypes = { LinkType::Float,  LinkType::Float };
	outputRect = Rectangle(ImVec2(bodyRect.size.x, 0), ImVec2(10, 10));
	inputRects.push_back(Rectangle(ImVec2(-10, 0), ImVec2(10, 10)));
	inputRects.push_back(Rectangle(ImVec2(-10, 20), ImVec2(10, 10)));
}

void AdditionNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double AdditionNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x,y,z) + inputs.at(1)->GetValue(x,y,z);
}

void AdditionNode::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	Node::Draw(draw_list, canvas_pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + bodyRect.pos.x, canvas_pos.y + bodyRect.pos.y), ImColor(255, 255, 255, 255), " Add", NULL, 0.0f, &clip_rect);
}

SubtractionNode::SubtractionNode() : Node("Subtraction", "Subtracts two numbers together") {
	outputType = LinkType::Float;
	inputTypes = { LinkType::Float,  LinkType::Float };
	outputRect = Rectangle(ImVec2(bodyRect.size.x, 0), ImVec2(10, 10));
	inputRects.push_back(Rectangle(ImVec2(-10, 0), ImVec2(10, 10)));
	inputRects.push_back(Rectangle(ImVec2(-10, 20), ImVec2(10, 10)));
}

void SubtractionNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double SubtractionNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z) - inputs.at(1)->GetValue(x, y, z);
}

void SubtractionNode::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	Node::Draw(draw_list, canvas_pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + bodyRect.pos.x, canvas_pos.y + bodyRect.pos.y), ImColor(255, 255, 255, 255), " Sub", NULL, 0.0f, &clip_rect);
}

MultiplicationNode::MultiplicationNode() : Node("Multiplication", "Multiplies two numbers together") {
	outputType = LinkType::Float;
	inputTypes = { LinkType::Float,  LinkType::Float };
	outputRect = Rectangle(ImVec2(bodyRect.size.x, 0), ImVec2(10, 10));
	inputRects.push_back(Rectangle(ImVec2(-10, 0), ImVec2(10, 10)));
	inputRects.push_back(Rectangle(ImVec2(-10, 20), ImVec2(10, 10)));
}

void MultiplicationNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double MultiplicationNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z) * inputs.at(1)->GetValue(x, y, z);
}

void MultiplicationNode::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	Node::Draw(draw_list, canvas_pos);
	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + bodyRect.pos.x, canvas_pos.y + bodyRect.pos.y), ImColor(255, 255, 255, 255), " Mul", NULL, 0.0f, &clip_rect);
}

DivisionNode::DivisionNode() : Node("Division", "Divides two numbers together") {
	outputType = LinkType::Float;
	inputTypes = { LinkType::Float,  LinkType::Float };
	outputRect = Rectangle(ImVec2(bodyRect.size.x, 0), ImVec2(10, 10));
	inputRects.push_back(Rectangle(ImVec2(-10, 0), ImVec2(10, 10)));
	inputRects.push_back(Rectangle(ImVec2(-10, 20), ImVec2(10, 10)));
}

void DivisionNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double DivisionNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z) / inputs.at(1)->GetValue(x, y, z);
}

void DivisionNode::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	Node::Draw(draw_list, canvas_pos);
	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + bodyRect.pos.x, canvas_pos.y + bodyRect.pos.y), ImColor(255, 255, 255, 255), " Div", NULL, 0.0f, &clip_rect);
}

NoiseSourceNode::NoiseSourceNode() : Node("Noise Source", "function on inputs") {
	outputType = LinkType::Float;
	inputTypes = { LinkType::Float,  LinkType::Float, LinkType::Int};
	outputRect = Rectangle(ImVec2(bodyRect.size.x, 0), ImVec2(10, 10));
	inputRects.push_back(Rectangle(ImVec2(-10, 0), ImVec2(10, 10)));
	inputRects.push_back(Rectangle(ImVec2(-10, 20), ImVec2(10, 10)));
	inputRects.push_back(Rectangle(ImVec2(-10, 40), ImVec2(10, 10)));
}

void NoiseSourceNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double NoiseSourceNode::GetValue(double x, double y, double z) {
	noiseInputSource.SetFrequency(inputs.at(0)->GetValue(x,y,z));
	noiseInputSource.SetPersistence(inputs.at(1)->GetValue(x, y, z));
	noiseInputSource.SetOctaveCount(inputs.at(2)->GetValue(x, y, z));

	return noiseInputSource.GetValue(x,y,z);
}

void NoiseSourceNode::Draw(ImDrawList* draw_list, ImVec2 canvas_pos) {
	Node::Draw(draw_list, canvas_pos);
	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + bodyRect.pos.x, canvas_pos.y + bodyRect.pos.y), ImColor(255, 255, 255, 255), " Noise", NULL, 0.0f, &clip_rect);
}

NodeGraph::NodeGraph()
{
	outputNode = new OutputNode(); //means every graph has an output, otherwise whats the point of one

	AddNode(outputNode);
}


NodeGraph::~NodeGraph()
{
}



void NodeGraph::DrawGraph() {

	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiSetCond_Once);
	ImGui::SetNextWindowPos(ImVec2(0, 300), ImGuiSetCond_Once);

	if (ImGui::Begin("Node Graph", 0, ImGuiWindowFlags_MenuBar)) {
		ImGui::Text("Test");
		ImGui::InputFloat3("Test Inputs", &testVec[0], -10, 10);
		if (ImGui::Button("Calc on test inputs")) {
			std::cout << outputNode->GetValue(testVec[0], testVec[1], testVec[2]) << std::endl;
		}

		{ //menu bar - implement functions
			if (ImGui::BeginMenuBar())
			{
				//TODO
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New")) {}
					if (ImGui::MenuItem("Open", "Ctrl+O")) {}
					if (ImGui::BeginMenu("Open Recent"))
					{
						ImGui::EndMenu();
					}
					if (ImGui::MenuItem("Save", "Ctrl+S")) {}
					if (ImGui::MenuItem("Save As..")) {}
					ImGui::Separator();
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
					
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit"))
				{
					if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
					if (ImGui::MenuItem("Redo", "CTRL+Y", false, true)) {}  // Disabled item
					ImGui::Separator();
					if (ImGui::MenuItem("Cut", "CTRL+X")) {}
					if (ImGui::MenuItem("Copy", "CTRL+C")) {}
					if (ImGui::MenuItem("Paste", "CTRL+V")) {}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

		}

		//Node toolbar list 
		{
			ImGui::BeginChild("scrolling", ImVec2(0, ImGui::GetItemsLineHeightWithSpacing() + 30), true, ImGuiWindowFlags_HorizontalScrollbar);
			//ImGui::BeginGroup();

			if (ImGui::Button("Constant", ImVec2(60, 20))) {
				AddNode(new ConstantNode());
			}
			ImGui::SameLine();

			if (ImGui::Button("Addition", ImVec2(60, 20))) {
				AddNode(new AdditionNode());
			}
			ImGui::SameLine();
			
			if (ImGui::Button("Subtraction", ImVec2(60, 20))) {
				AddNode(new SubtractionNode());
			}
			ImGui::SameLine();
			if (ImGui::Button("Multiplication", ImVec2(60, 20))) {
				AddNode(new MultiplicationNode());
			}
			ImGui::SameLine();
			if (ImGui::Button("Division", ImVec2(60, 20))) {
				AddNode(new DivisionNode());
			}
			ImGui::SameLine();
			if (ImGui::Button("Noise Source", ImVec2(60, 20))) {
				AddNode(new NoiseSourceNode());
			}

			//ImGui::EndGroup();
			ImGui::EndChild();

		}

		{ //canvas of nodes
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			static ImVector<ImVec2> points;
			static bool adding_line = false;
			if (ImGui::Button("Clear")) {
				points.clear();
				nodes.clear();
				connectors.clear();
			}
			if (points.Size >= 2) { 
				ImGui::SameLine(); 
				if (ImGui::Button("Points Undo")) { 
					points.pop_back(); 
					points.pop_back(); 
					
				} 
			}
			if (nodes.size() > 0) {
				ImGui::SameLine();
				if (ImGui::Button("Undo Node Creation")) {
					nodes.pop_back();
				}
			}
			ImGui::Text("Left-click and drag to add lines,\nRight-click to undo");

			// Here we are using InvisibleButton() as a convenience to 1) advance the cursor and 2) allows us to use IsItemHovered()
			// However you can draw directly and poll mouse/keyboard by yourself. You can manipulate the cursor using GetCursorPos() and SetCursorPos().
			// If you only use the ImDrawList API, you can notify the owner window of its extends by using SetCursorPos(max).
			ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
			ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
			if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
			if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
			draw_list->AddRectFilledMultiColor(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), ImColor(50, 50, 50), ImColor(50, 50, 60), ImColor(60, 60, 70), ImColor(50, 50, 60));
			draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), ImColor(255, 255, 255));

			ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - canvas_pos.x, ImGui::GetIO().MousePos.y - canvas_pos.y);

			static bool adding_connection = false;
			static bool FirstConnectionIsOutput; //whether the beginning of the connector is an output or an input
			static Connector adding_connector = Connector(ImVec2(0, 0), ImVec2(0, 0));
			static bool isDraggingWidget = false;
			Node* nodeToDelete;
			static bool deleteNode;

			for (Node* node : nodes) {
				if (adding_connection) { //user is looking for the endpoint
					if (!FirstConnectionIsOutput) { //make sure the connected node is of the opposite type
						if (AABBCheck(node->outputRect.pos + node->bodyRect.pos, node->outputRect.size, mouse_pos_in_canvas)) {
							if (ImGui::IsMouseClicked(0)) {
								if (adding_connector.endNode != node) { //not the same node
									adding_connection = false;
									adding_connector.SetStart(node);
									connectors.insert(connectors.begin(), new Connector(adding_connector));
									node->AddOutputConnector(connectors.at(0));
									connectors.at(0)->startNode->AddInputConnector(connectors.at(0));
									std::cout << "connected to output " << std::endl;
								}
							}
						}
					}
					else if (FirstConnectionIsOutput) {
						for (Rectangle inputRect : node->inputRects) {
							if (AABBCheck(inputRect.pos + node->bodyRect.pos, inputRect.size, mouse_pos_in_canvas)) {
								if (ImGui::IsMouseClicked(0)) { 
									if (adding_connector.startNode != node) { //not the same node
										adding_connection = false;
										adding_connector.SetEnd(node, inputRect);
										connectors.insert(connectors.begin(), new Connector(adding_connector));
										node->AddInputConnector(connectors.at(0));
										connectors.at(0)->endNode->AddOutputConnector(connectors.at(0));
										std::cout << "connected to input " << std::endl;
									}
								}
							}
						}
					}
				}
				else { //user hasn't clicked on any node
					if (AABBCheck(node->outputRect.pos + node->bodyRect.pos, node->outputRect.size, mouse_pos_in_canvas)) {
						if (ImGui::IsMouseClicked(0)) {
							std::cout << "Clicked on outputnode " << std::endl;
							adding_connection = true;
							adding_connector.SetStart(node); 
							FirstConnectionIsOutput = true;
						}
					}
					for (Rectangle inputRect : node->inputRects) {
						if (AABBCheck(inputRect.pos + node->bodyRect.pos, inputRect.size, mouse_pos_in_canvas)) {
							if (ImGui::IsMouseClicked(0)) {
								std::cout << "Clicked on inputNode" << std::endl;
								adding_connection = true;
								adding_connector.SetEnd(node, inputRect);
								FirstConnectionIsOutput = false;
							}
						}
					}
				}
				//Delete node if right clicking and over the node
				if (AABBCheck(node->bodyRect.pos, node->bodyRect.size, mouse_pos_in_canvas) && ImGui::IsMouseClicked(1)) {
					std::cout << "Removed Node" << std::endl;
					
					//Remove the connector's nodes reference to connected nodes
					for (Connector* c : node->inputConnections) {
						c->startNode->RemoveInputConnector(c);
					}
					for (Connector* c : node->outputConnections) {
						c->endNode->RemoveOutputConnector(c);
					}

					//Remove the connections
					for (Connector* c : node->inputConnections) {
						auto con = std::find(connectors.begin(), connectors.end(), c);
						if (con != connectors.end())
							connectors.erase(con);
					}
					for (Connector* c : node->outputConnections) {
						auto con = std::find(connectors.begin(), connectors.end(), c);
						if (con != connectors.end())
							connectors.erase(con);
					}
					deleteNode = true;
					nodeToDelete = node;

				}
				else {
					node->Update(mouse_pos_in_canvas, isDraggingWidget);
					node->Draw(draw_list, canvas_pos);
				}
			}
			if (deleteNode) {
				RemoveNode(nodeToDelete);
				deleteNode = false;
			}

			for (Connector* connect : connectors) {
				connect->Draw(draw_list, canvas_pos);
			}


			ImGui::InvisibleButton("canvas", canvas_size);
			//bool adding_preview = false;
			//if (adding_line)
			//{
			//	adding_preview = true;
			//	points.push_back(mouse_pos_in_canvas);
			//	if (!ImGui::GetIO().MouseDown[0])
			//		adding_line = adding_preview = false;
			//}
			//if (ImGui::IsItemHovered())
			//{
			//	if (!adding_line && ImGui::IsMouseClicked(0))
			//	{
			//		points.push_back(mouse_pos_in_canvas);
			//		adding_line = true;
			//	}
			//	if (ImGui::IsMouseClicked(1) && !points.empty())
			//	{
			//		adding_line = adding_preview = false;
			//		points.pop_back();
			//		points.pop_back();
			//	}
			//}
			//draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y));      // clip lines within the canvas (if we resize it, etc.)
			//for (int i = 0; i < points.Size - 1; i += 2) {
			//	draw_list->AddLine(ImVec2(canvas_pos.x + points[i].x, canvas_pos.y + points[i].y), ImVec2(canvas_pos.x + points[i + 1].x, canvas_pos.y + points[i + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
			//}
			//draw_list->PopClipRect();
			//if (adding_preview)
			//	points.pop_back();

		}

	}
	ImGui::End();
	
	
	//ImGui::SliderFloat("Terrain Width", &terrainWidth, 1, 10000);
	//ImGui::SliderInt("Terrain Max Subdivision", &terrainMaxLevels, 0, 10);
	//ImGui::SliderInt("Terrain Grid Width", &terrainGridDimentions, 1, 10);

	//if (ImGui::Button("Recreate Terrain", ImVec2(130, 20))) {
	//	recreateTerrain = true;
	//}

	//ImGui::Text("All terrains update Time: %u(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds());
	//for (Terrain* ter : terrains)
	//{
	//	ImGui::Text("Terrain Draw Time: %u(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds());
	//	ImGui::Text("Terrain Quad Count %d", ter->numQuads);
	//}

}

void NodeGraph::AddNode(Node* node) {
	nodes.push_back(node);
}
void NodeGraph::RemoveNode(Node* node) {
	auto it = std::find(nodes.begin(), nodes.end(), node);
	if (it != nodes.end())
		nodes.erase(it);

}