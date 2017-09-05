#include "NodeGraph.h"

#include <iostream>

//Does a rect bound check with the pos and size of the box
bool AABBCheck(ImVec2 pos, ImVec2 size, ImVec2 point) {
	return point.x >= pos.x && point.y >= pos.y && point.x <= pos.x + size.x && point.y <= pos.y + size.y;
}

Node::Node(std::string name, std::string description) : name(name), description(description) {
	rect = Rectangle(ImVec2(10, 10), ImVec2(50, 50));
}

void Node::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double Node::GetValue(double x, double y, double z) {
	return 0.0;
}

void Node::draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas) {

}

void OutputNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double OutputNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z);
}

void OutputNode::draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas) {
	ImVec4 clip_rect(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y, canvas_pos.x + rect.pos.x + rect.size.x, canvas_pos.y + rect.pos.y + rect.size.y);

	if (AABBCheck(rect.pos, rect.size, mouse_pos_in_canvas)) {
		if (ImGui::IsMouseDragging(0)) {
			rect.pos = ImVec2(mouse_pos_in_canvas.x - rect.size.x / 2, mouse_pos_in_canvas.y - rect.size.y / 2);
		}
	}

	rect.draw(draw_list, canvas_pos);
	rectA.draw(draw_list, canvas_pos, rect.pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y), ImColor(255, 255, 255, 255), " Out", NULL, 0.0f, &clip_rect);
}


double ConstantNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z);
}

void ConstantNode::draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas) {
	ImVec4 clip_rect(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y, canvas_pos.x + rect.pos.x + rect.size.x, canvas_pos.y + rect.pos.y + rect.size.y);

	if (AABBCheck(rect.pos, rect.size, mouse_pos_in_canvas)) {
		if (ImGui::IsMouseDragging(0)) {
			rect.pos = ImVec2(mouse_pos_in_canvas.x - rect.size.x / 2, mouse_pos_in_canvas.y - rect.size.y / 2);
		}
	}

	rect.draw(draw_list, canvas_pos);
	rectO.draw(draw_list, canvas_pos, rect.pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y), ImColor(255, 255, 255, 255), " Val", NULL, 0.0f, &clip_rect);

}


void AdditionNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double AdditionNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x,y,z) + inputs.at(1)->GetValue(x,y,z);
}

void AdditionNode::draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas) {
	ImVec4 clip_rect(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y, canvas_pos.x + rect.pos.x + rect.size.x, canvas_pos.y + rect.pos.y + rect.size.y);

	if (AABBCheck(rect.pos, rect.size, mouse_pos_in_canvas)) {
		if (ImGui::IsMouseDragging(0)) {
			rect.pos = ImVec2(mouse_pos_in_canvas.x - rect.size.x/2, mouse_pos_in_canvas.y - rect.size.y/2);
		}
	}

	rect.draw(draw_list, canvas_pos);
	rectO.draw(draw_list, canvas_pos, rect.pos);
	rectA.draw(draw_list, canvas_pos, rect.pos);
	rectB.draw(draw_list, canvas_pos, rect.pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y), ImColor(255, 255, 255, 255), " Add", NULL, 0.0f, &clip_rect);
}

void SubtractionNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double SubtractionNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z) - inputs.at(1)->GetValue(x, y, z);
}

void SubtractionNode::draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas) {
	ImVec4 clip_rect(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y, canvas_pos.x + rect.pos.x + rect.size.x, canvas_pos.y + rect.pos.y + rect.size.y);

	if (AABBCheck(rect.pos, rect.size, mouse_pos_in_canvas)) {
		if (ImGui::IsMouseDragging(0)) {
			rect.pos = ImVec2(mouse_pos_in_canvas.x - rect.size.x / 2, mouse_pos_in_canvas.y - rect.size.y / 2);
		}
	}

	rect.draw(draw_list, canvas_pos);
	rectO.draw(draw_list, canvas_pos, rect.pos);
	rectA.draw(draw_list, canvas_pos, rect.pos);
	rectB.draw(draw_list, canvas_pos, rect.pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y), ImColor(255, 255, 255, 255), " Sub", NULL, 0.0f, &clip_rect);
}

void MultiplicationNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double MultiplicationNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z) * inputs.at(1)->GetValue(x, y, z);
}

void MultiplicationNode::draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas) {
	ImVec4 clip_rect(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y, canvas_pos.x + rect.pos.x + rect.size.x, canvas_pos.y + rect.pos.y + rect.size.y);

	if (AABBCheck(rect.pos, rect.size, mouse_pos_in_canvas)) {
		if (ImGui::IsMouseDragging(0)) {
			rect.pos = ImVec2(mouse_pos_in_canvas.x - rect.size.x / 2, mouse_pos_in_canvas.y - rect.size.y / 2);
		}
	}

	rect.draw(draw_list, canvas_pos);
	rectO.draw(draw_list, canvas_pos, rect.pos);
	rectA.draw(draw_list, canvas_pos, rect.pos);
	rectB.draw(draw_list, canvas_pos, rect.pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y), ImColor(255, 255, 255, 255), " Mul", NULL, 0.0f, &clip_rect);
}

void DivisionNode::SetInputNode(int index, Node* n) {
	inputs.at(index) = n;
}

double DivisionNode::GetValue(double x, double y, double z) {
	return inputs.at(0)->GetValue(x, y, z) / inputs.at(1)->GetValue(x, y, z);
}

void DivisionNode::draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas) {
	ImVec4 clip_rect(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y, canvas_pos.x + rect.pos.x + rect.size.x, canvas_pos.y + rect.pos.y + rect.size.y);

	if (AABBCheck(rect.pos, rect.size, mouse_pos_in_canvas)) {
		if (ImGui::IsMouseDragging(0)) {
			rect.pos = ImVec2(mouse_pos_in_canvas.x - rect.size.x / 2, mouse_pos_in_canvas.y - rect.size.y / 2);
		}
	}

	rect.draw(draw_list, canvas_pos);
	rectO.draw(draw_list, canvas_pos, rect.pos);
	rectA.draw(draw_list, canvas_pos, rect.pos);
	rectB.draw(draw_list, canvas_pos, rect.pos);

	draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(canvas_pos.x + rect.pos.x, canvas_pos.y + rect.pos.y), ImColor(255, 255, 255, 255), " Div", NULL, 0.0f, &clip_rect);
}

NodeGraph::NodeGraph()
{

}


NodeGraph::~NodeGraph()
{
}



void NodeGraph::DrawGraph() {

	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiSetCond_Once);
	ImGui::SetNextWindowPos(ImVec2(0, 300), ImGuiSetCond_Once);

	if (ImGui::Begin("Node Graph", 0, ImGuiWindowFlags_MenuBar)) {
		ImGui::Text("Test");

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

			if (ImGui::Button("Output", ImVec2(60, 20))) {
				AddNode(new OutputNode());
			}
			ImGui::SameLine();

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
			
			for (Node* node : nodes) {
				node->draw(draw_list, canvas_pos, mouse_pos_in_canvas);
			}


			bool adding_preview = false;
			ImGui::InvisibleButton("canvas", canvas_size);
			if (adding_line)
			{
				adding_preview = true;
				points.push_back(mouse_pos_in_canvas);
				if (!ImGui::GetIO().MouseDown[0])
					adding_line = adding_preview = false;
			}
			if (ImGui::IsItemHovered())
			{
				if (!adding_line && ImGui::IsMouseClicked(0))
				{
					points.push_back(mouse_pos_in_canvas);
					adding_line = true;
				}
				if (ImGui::IsMouseClicked(1) && !points.empty())
				{
					adding_line = adding_preview = false;
					points.pop_back();
					points.pop_back();
				}
			}
			draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y));      // clip lines within the canvas (if we resize it, etc.)
			for (int i = 0; i < points.Size - 1; i += 2) {
				draw_list->AddLine(ImVec2(canvas_pos.x + points[i].x, canvas_pos.y + points[i].y), ImVec2(canvas_pos.x + points[i + 1].x, canvas_pos.y + points[i + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
			}
			draw_list->PopClipRect();
			if (adding_preview)
				points.pop_back();

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
	auto index = std::find(nodes.begin(), nodes.end(), node);
	nodes.erase(index);
}