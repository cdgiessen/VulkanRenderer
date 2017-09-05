#pragma once

#include <vector>
#include <tuple>


#include "ImGui\imgui.h"

//Holds position and size of a rectangle and how to draw itself relative to the canvas and a parent if present
struct Rectangle {
	ImVec2 pos;
	ImVec2 size;

	Rectangle() : pos(ImVec2(0, 0)), size(ImVec2(10, 10)) {}

	Rectangle(ImVec2 pos, ImVec2 size) : pos(pos), size(size) {}

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 parent_pos = ImVec2(0,0)) {
		draw_list->AddRect(ImVec2(canvas_pos.x + parent_pos.x + pos.x, canvas_pos.y + parent_pos.y + pos.y), ImVec2(canvas_pos.x + parent_pos.x + pos.x + size.x, canvas_pos.y + parent_pos.y + pos.y + size.y), ImColor(255, 255, 255));
	}
};

class Node {
public:
	std::string name;
	std::string description;
	
	std::vector<Node *> inputs;

	Rectangle rect;

	Node() {};
	Node(std::string name, std::string description);

	virtual void SetInputNode(int index, Node* n);

	virtual double GetValue(double x, double y, double z);

	virtual void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas);
};

class OutputNode : public Node {
public:

	OutputNode() : Node("Output", "End point for entire graph") {}

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas);

	Rectangle rectA = Rectangle(ImVec2(-10, 0), ImVec2(10, 10));
};

class ConstantNode : public Node {
public:

	ConstantNode() : Node("Constant", "Holds a value") {}

	double GetValue(double x, double y, double z);

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas);

	Rectangle rectO = Rectangle(ImVec2(rect.size.x, 0), ImVec2(10, 10));
};

class AdditionNode : public Node {
public:

	AdditionNode() : Node("Addition", "Adds two numbers together") {}

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas);

	Rectangle rectO = Rectangle(ImVec2(rect.size.x, 0), ImVec2(10, 10));
	Rectangle rectA = Rectangle(ImVec2(-10, 0), ImVec2(10, 10));
	Rectangle rectB = Rectangle(ImVec2(-10, 20), ImVec2(10, 10));
};

class SubtractionNode : public Node {
public:

	SubtractionNode() : Node("Subtraction", "Adds two numbers together") {}

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas);

	Rectangle rectO = Rectangle(ImVec2(rect.size.x, 0), ImVec2(10, 10));
	Rectangle rectA = Rectangle(ImVec2(-10, 0), ImVec2(10, 10));
	Rectangle rectB = Rectangle(ImVec2(-10, 20), ImVec2(10, 10));
};

class MultiplicationNode : public Node {
public:

	MultiplicationNode() : Node("Multiplication", "Adds two numbers together") {}

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas);

	Rectangle rectO = Rectangle(ImVec2(rect.size.x, 0), ImVec2(10, 10));
	Rectangle rectA = Rectangle(ImVec2(-10, 0), ImVec2(10, 10));
	Rectangle rectB = Rectangle(ImVec2(-10, 20), ImVec2(10, 10));
};

class DivisionNode : public Node {
public:

	DivisionNode() : Node("Division", "Adds two numbers together") {}

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 mouse_pos_in_canvas);

	Rectangle rectO = Rectangle(ImVec2(rect.size.x, 0), ImVec2(10, 10));
	Rectangle rectA = Rectangle(ImVec2(-10, 0), ImVec2(10, 10));
	Rectangle rectB = Rectangle(ImVec2(-10, 20), ImVec2(10, 10));
};


class NodeGraph
{
public:
	NodeGraph();
	~NodeGraph();

	void SaveGraph();
	void LoadGraph();

	void DrawGraph();

	void AddNode(Node* node);
	void RemoveNode(Node* node);

private:
	//static bool show_node_graph_window = true;
	std::vector<Node*> nodes;

	//nodeTypeList nodeTypes;
};