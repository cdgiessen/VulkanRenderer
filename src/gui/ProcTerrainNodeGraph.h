#pragma once

#include <stdio.h>

#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <stdint.h>

#include "TerGenNodeGraph.h"

#include "../../third-party/ImGui/imgui.h"

enum class ConnectionType
{
	Null,
	Float,
	Int,
	Vec2,
	Vec3,
	Vec4,
	Color,
};

struct SlotValueHolder {
	
	int i;
	float f;
	glm::vec2 glm2;
	glm::vec3 glm3;
	glm::vec4 glm4;
	
	
	ConnectionType type = ConnectionType::Null;

	SlotValueHolder(ConnectionType type) : type(type) {};
};

class Node;
class ProcTerrainNodeGraph;

class Connection {
public:
	Connection(ConnectionType conType, std::shared_ptr<Node> input, std::shared_ptr<Node> output);

	ConnectionType conType;

	ImVec2 startPosRelNode = ImVec2(0.0f,0.0f);
	ImVec2 endPosRelNode = ImVec2(0.0f, 0.0f);

	std::shared_ptr<Node> input;
	std::shared_ptr<Node> output;
	
};

class ConnectionSlot {
public:
	ConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type);
	ConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type, std::string name);

	virtual void Draw(ImDrawList* imDrawList, const ProcTerrainNodeGraph& graph, const Node& parentNode) =0;
	bool IsHoveredOver(ImVec2 parentPos) const;
	ConnectionType GetType();

	int slotNum;
	ImVec2 pos;
	float nodeSlotRadius = 5.0f;
	ImColor slotColor = ImColor(150, 150, 150);

	std::string name;

	ConnectionType conType;

};

class InputConnectionSlot : public ConnectionSlot {
public:
	InputConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type, std::string name);
	InputConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type, std::string name, 
		float defaultVal, float sliderStepSize, float lowerBound, float upperBound);
	InputConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type, std::string name, 
		int defaultVal, float sliderStepSize, float lowerBound, float upperBound);

	void Draw(ImDrawList* imDrawList, const ProcTerrainNodeGraph& graph, const Node& parentNode) override;

	bool hasConnection = false;
	std::shared_ptr<Connection> connection;
	SlotValueHolder value;
	float sliderStepSize = 0.1f, lowerBound = 0.0f, upperBound = 1.0f;

};

class OutputConnectionSlot : public ConnectionSlot {
public:
	OutputConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type);

	void Draw(ImDrawList* imDrawList, const ProcTerrainNodeGraph& graph, const Node& parentNode) override;

	std::vector<std::shared_ptr<Connection>> connections;

};

class Node {
public:
	Node(std::string name, ConnectionType outputType);

	void SetInternalLink(int index, std::shared_ptr<NewNodeGraph::INode> inode);

	std::string name;
	ImVec2 pos = ImVec2(200,150);
	ImVec2 size = ImVec2(100, 100);
	int id;

	std::vector<InputConnectionSlot> inputSlots;
	OutputConnectionSlot outputSlot;

	void AddInputSlot(ConnectionType type, std::string name);
	void AddInputSlot(ConnectionType type, std::string name, float defaultValue, float sliderStepSize, float lowerBound, float upperBound);
	void AddInputSlot(ConnectionType type, std::string name, int defaultValue, float sliderStepSize, float lowerBound, float upperBound);

	std::shared_ptr<NewNodeGraph::INode> internal_node;
	//void Draw(ImDrawList*  imDrawList, ImVec2 offset);
};

enum class NodeType {
	Output,
	Addition,
	Subtraction,
	Multiplication,
	Division,
	Power,
	Selector,
	Perlin,
	Simplex,
	CellNoise,
	ValueNoise,
	Voroni,
	ConstantInt,
	ConstantFloat,
};

class OutputNode		: public Node { public: OutputNode(NewNodeGraph::TerGenNodeGraph& graph); };

class MathNode			: public Node { public: MathNode(std::string name); };

class AdditionNode		: public MathNode { public: AdditionNode(NewNodeGraph::TerGenNodeGraph& graph); };
class SubtractionNode	: public MathNode { public: SubtractionNode(NewNodeGraph::TerGenNodeGraph& graph); };
class MultiplicationNode: public MathNode { public: MultiplicationNode(NewNodeGraph::TerGenNodeGraph& graph); };
class DivisionNode		: public MathNode { public: DivisionNode(NewNodeGraph::TerGenNodeGraph& graph); };
class PowerNode			: public MathNode { public: PowerNode(NewNodeGraph::TerGenNodeGraph& graph); };

class SelectorNode		: public Node { public: SelectorNode(NewNodeGraph::TerGenNodeGraph& graph); };

class NoiseNode			: public Node { public: NoiseNode(std::string name);
	std::shared_ptr<NewNodeGraph::NoiseSourceNode> internal_node_noise;
};

class PerlinNode		: public NoiseNode { public: PerlinNode(NewNodeGraph::TerGenNodeGraph& graph); };
class SimplexNode		: public NoiseNode { public: SimplexNode(NewNodeGraph::TerGenNodeGraph& graph); };
class CellNoiseNode		: public NoiseNode { public: CellNoiseNode(NewNodeGraph::TerGenNodeGraph& graph); };
class ValueNoiseNode	: public NoiseNode { public: ValueNoiseNode(NewNodeGraph::TerGenNodeGraph& graph); };
class VoroniNode		: public NoiseNode { public: VoroniNode(NewNodeGraph::TerGenNodeGraph& graph); };
class ConstantIntNode	: public Node { public: ConstantIntNode(NewNodeGraph::TerGenNodeGraph& graph); };
class ConstantFloatNode : public Node { public: ConstantFloatNode(NewNodeGraph::TerGenNodeGraph& graph); };


struct HoveredSlotInfo {
	std::shared_ptr<Node> node;
	bool isOutput = false;
	int inputSlotNum = 0;
	HoveredSlotInfo() : node(nullptr), isOutput(false), inputSlotNum(-1) {};
	HoveredSlotInfo(std::shared_ptr<Node> node, bool output, int inSlotNum)
		: node(node), isOutput(output), inputSlotNum(inSlotNum) {

	}
};

class PossibleConnection {
public:
	PossibleConnection();

	static ImVec2 GetActiveSlotPosition(HoveredSlotInfo info);
	static ConnectionType GetActiveSlotType(HoveredSlotInfo info);

	enum class State {
		Default,
		Hover,
		Dragging,

	} state;

	bool isActive = false;

	HoveredSlotInfo slot;
};

class ProcTerrainNodeGraph
{
public:
	ProcTerrainNodeGraph();
	~ProcTerrainNodeGraph();

	void Draw();

	NewNodeGraph::TerGenNodeGraph& GetGraph();

private:
	friend class Node;
	friend class ConnectionSlot;
	friend class InputConnectionSlot;
	friend class OutputConnectionSlot;

	void DrawMenuBar();
	void DrawButtonBar();
	void DrawNodeButtons();
	void DrawNodeCanvas();

	void DrawHermite(ImDrawList* imDrawList, ImVec2 p1, ImVec2 p2, int STEPS);
	void DrawNodes(ImDrawList*  imDrawList);
	void DrawConnections(ImDrawList*  imDrawList);
	void DrawPossibleConnection(ImDrawList* imDrawList);
	
	void SaveGraphFromFile();
	void LoadGraphFromFile();
	void BuildTerGenNodeGraph();

	void DeleteNode(std::shared_ptr<Node> node);
	void DeleteConnection(std::shared_ptr<Connection> con);
	void ResetGraph();

	HoveredSlotInfo GetHoveredSlot();

	void AddNode(NodeType nodeType);
	
	const ImVec2 windowPadding = ImVec2(8.0f, 8.0f);
	ImVec2 windowPos = ImVec2(0, 0);
	ImVec2 graphOffset = ImVec2(0,0);

	bool window_open;
	int curID = 0;
	PossibleConnection posCon;

	NewNodeGraph::TerGenNodeGraph curGraph;

	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<std::shared_ptr<Connection>> connections;

	std::shared_ptr<Node> outputNode;

};

