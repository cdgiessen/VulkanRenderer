#pragma once

#include <vector>
#include <tuple>

#include "..\third-party\ImGui\imgui.h"

//#include <noise\basictypes.h>
//#include <noiseutils.h>

enum LinkType{
Float = 0,
Int = 1,
Bool = 2,
Double = 3,
Vector2 = 4,
Vector3 = 5,
Vector4 = 6,
Color3 = 7,
Color4 = 8,
String = 9,
None = 10,
};

//Holds position and size of a rectangle and how to draw itself relative to the canvas and a parent if present
struct Rectangle {
	ImVec2 pos;
	ImVec2 size;

	Rectangle() : pos(ImVec2(0, 0)), size(ImVec2(10, 10)) {}

	Rectangle(ImVec2 pos, ImVec2 size) : pos(pos), size(size) {}

	void draw(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 parent_pos = ImVec2(0,0)) {
		draw_list->AddRect(canvas_pos + parent_pos + pos, canvas_pos + parent_pos + pos + size, ImColor(255, 255, 255));
	}
};


class Node;
struct Connector {
	Node* startNode;
	Node* endNode;
	Connector(ImVec2 start, ImVec2 end, ImColor color = ImColor(255,255,255,255)) : start(start), end(end), startPivot(ImVec2(start.x + 10, start.y)), endPivot(ImVec2(end.x - 10, end.y)), color(color) {}

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);

	//Sets connection from output
	void SetStart(Node* node);

	//Sets connection to input
	void SetEnd(Node* node, Rectangle inputRect);

private:
	ImVec2 start;
	ImVec2 startPivot;
	ImVec2 end;
	ImVec2 endPivot;

	ImColor color;
};

class Node {
public:
	std::string name;
	std::string description;
	
	LinkType outputType = LinkType::Float;
	std::vector<LinkType> inputTypes;
	std::vector<Node *> inputs;

	Rectangle bodyRect;
	Rectangle outputRect;
	std::vector<Rectangle> inputRects;

	std::vector<Connector*> inputConnections;
	std::vector<Connector*> outputConnections;
	
	Node() {};
	Node(std::string name, std::string description);

	void AddInputConnector(Connector* connector);
	void AddOutputConnector(Connector* connector);
	void RemoveInputConnector(Connector* connector);
	void RemoveOutputConnector(Connector* connector);

	virtual void SetInputNode(int index, Node* n);

	virtual double GetValue(double x, double y, double z);

	virtual void Update(ImVec2 mouse_pos_in_canvas, bool isDraggingWidget);

	virtual void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
protected:
	ImVec4 clip_rect = ImVec4(0,0,0,0);
};

class OutputNode : public Node {
public:

	OutputNode();

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
};

class ConstantNode : public Node {
public:
	float value;
	ConstantNode();

	double GetValue(double x, double y, double z);

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
};

class AdditionNode : public Node {
public:

	AdditionNode();

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
};

class SubtractionNode : public Node {
public:

	SubtractionNode();

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
};

class MultiplicationNode : public Node {
public:

	MultiplicationNode();

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
};

class DivisionNode : public Node {
public:

	DivisionNode();

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
};

class NoiseSourceNode : public Node {
public:
	//module::Perlin noiseInputSource;

	NoiseSourceNode();

	void SetInputNode(int index, Node* n);

	double GetValue(double x, double y, double z);

	void Draw(ImDrawList* draw_list, ImVec2 canvas_pos);
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
	float testVec[3];
	//static bool show_node_graph_window = true;
	std::vector<Node* > nodes;
	std::vector<Connector* > connectors;	

	OutputNode* outputNode;

	//nodeTypeList nodeTypes;
};