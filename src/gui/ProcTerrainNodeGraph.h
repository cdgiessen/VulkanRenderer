#pragma once

#include <stdio.h>

#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <variant>
#include <stdint.h>

#include "TerGenNodeGraph.h"

#include "InternalGraph.h"

#include "../../third-party/ImGui/imgui.h"

#include "../../third-party/json/json.hpp"



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
	
	std::variant<int, float, glm::vec2, glm::vec3, glm::vec4> value;
	
	
	ConnectionType type = ConnectionType::Null;

	SlotValueHolder(ConnectionType type) : type(type) {};
};

class Node;
class ProcTerrainNodeGraph;

class Connection {
public:
	Connection(ConnectionType conType, std::shared_ptr<Node> input, std::shared_ptr<Node> output, int output_slot_id);

	ConnectionType conType;

	ImVec2 startPosRelNode = ImVec2(0.0f,0.0f);
	ImVec2 endPosRelNode = ImVec2(0.0f, 0.0f);

	std::shared_ptr<Node> input;
	std::shared_ptr<Node> output;
	
	int output_slot_id = -1;
};

class ConnectionSlot {
public:
	ConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type);
	ConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type, std::string name);

	virtual int Draw(ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset) =0;
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
		std::variant<int, float, glm::vec2, glm::vec3, glm::vec4> defaultValue);
	InputConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type, std::string name,
		std::variant<int, float, glm::vec2, glm::vec3, glm::vec4> defaultValue,
		float sliderStepSize, float lowerBound, float upperBound);

	int Draw(ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset) override;

	std::shared_ptr<Connection> connection;
	SlotValueHolder value;
	float sliderStepSize = 0.1f, lowerBound = 0.0f, upperBound = 1.0f;

};

class OutputConnectionSlot : public ConnectionSlot {
public:
	OutputConnectionSlot(int slotNum, ImVec2 pos, ConnectionType type);

	int Draw(ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset) override;

	std::vector<std::shared_ptr<Connection>> connections;

};

enum class NodeType {
	Output,
	Addition,
	Subtraction,
	Multiplication,
	Division,
	Power,
	Max,
	Min,
	Blend,
	Clamp,
	Perlin,
	Simplex,
	CellNoise,
	ValueNoise,
	Voroni,
	WhiteNoise,
	ConstantInt,
	ConstantFloat,
	ColorCreator,
};

class Node {
public:
	Node(std::string name, ConnectionType outputType);

	//void SetInternalLink(int index, std::shared_ptr<NewNodeGraph::INode> inode);

	std::string name;
	ImVec2 pos = ImVec2(200,150);
	ImVec2 size = ImVec2(100, 100);
	int id;
	NodeType type;

	std::vector<InputConnectionSlot> inputSlots;
	OutputConnectionSlot outputSlot;

	void AddInputSlot(ConnectionType type, std::string name);
	void AddInputSlot(ConnectionType type, std::string name, float defaultValue, float sliderStepSize, float lowerBound, float upperBound);
	void AddInputSlot(ConnectionType type, std::string name, int defaultValue, float sliderStepSize, float lowerBound, float upperBound);
	void AddInputSlot(ConnectionType type, std::string name, glm::vec2 defaultValue);
	void AddInputSlot(ConnectionType type, std::string name, glm::vec3 defaultValue);
	void AddInputSlot(ConnectionType type, std::string name, glm::vec4 defaultValue);

	InternalGraph::NodeID internalNodeID;
	//std::shared_ptr<NewNodeGraph::INode> internal_node;
	//void Draw(ImDrawList*  imDrawList, ImVec2 offset);

	bool hasTextInput = false;
};

class OutputNode		: public Node { public: OutputNode(InternalGraph::GraphPrototype& graph); };

class MathNode			: public Node { public: MathNode(std::string name); };

class AdditionNode		: public MathNode { public: AdditionNode(InternalGraph::GraphPrototype& graph); };
class SubtractionNode	: public MathNode { public: SubtractionNode(InternalGraph::GraphPrototype& graph); };
class MultiplicationNode: public MathNode { public: MultiplicationNode(InternalGraph::GraphPrototype& graph); };
class DivisionNode		: public MathNode { public: DivisionNode(InternalGraph::GraphPrototype& graph); };
class PowerNode			: public MathNode { public: PowerNode(InternalGraph::GraphPrototype& graph); };
class MaxNode			: public MathNode { public: MaxNode(InternalGraph::GraphPrototype& graph); };
class MinNode			: public MathNode { public: MinNode(InternalGraph::GraphPrototype& graph); };



class BlendNode			: public Node { public: BlendNode(InternalGraph::GraphPrototype& graph); };
class ClampNode			: public Node { public: ClampNode(InternalGraph::GraphPrototype& graph); };

class NoiseNode			: public Node { public: NoiseNode(std::string name);
	//std::shared_ptr<NewNodeGraph::NoiseSourceNode> internal_node_noise;
};

class PerlinNode		: public NoiseNode { public: PerlinNode(InternalGraph::GraphPrototype& graph); };
class SimplexNode		: public NoiseNode { public: SimplexNode(InternalGraph::GraphPrototype& graph); };
class CellNoiseNode		: public NoiseNode { public: CellNoiseNode(InternalGraph::GraphPrototype& graph); };
class ValueNoiseNode	: public NoiseNode { public: ValueNoiseNode(InternalGraph::GraphPrototype& graph); };
class VoroniNode		: public NoiseNode { public: VoroniNode(InternalGraph::GraphPrototype& graph); };
class WhiteNoiseNode	: public NoiseNode { public: WhiteNoiseNode(InternalGraph::GraphPrototype& graph); };

class ConstantIntNode	: public Node { public: ConstantIntNode(InternalGraph::GraphPrototype& graph); };
class ConstantFloatNode : public Node { public: ConstantFloatNode(InternalGraph::GraphPrototype& graph); };

class ColorCreator : public Node { public: ColorCreator(InternalGraph::GraphPrototype& graph); };


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

	InternalGraph::GraphPrototype& GetGraph();

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
	void ResetOutputNode();

	HoveredSlotInfo GetHoveredSlot();

	void AddNode(NodeType nodeType, ImVec2 position, int id = -1);

	void SetNodeInternalLinkByID(InternalGraph::NodeID internalNodeID, int index, InternalGraph::NodeID id);
	void ResetNodeInternalLinkByID(InternalGraph::NodeID internalNodeID, int index);

	void SetNodeInternalValueByID(InternalGraph::NodeID internalNodeID, int index, InternalGraph::LinkTypeVariants val);

	std::shared_ptr<Node> GetNodeById(int id);
	
	const ImVec2 windowPadding = ImVec2(8.0f, 8.0f);
	ImVec2 windowPos = ImVec2(0, 0);
	ImVec2 graphOffset = ImVec2(0,0);
	ImVec2 startingNodePos = ImVec2(200, 150);

	bool window_open;
	int curID = 0;
	PossibleConnection posCon;

	//NewNodeGraph::TerGenNodeGraph curGraph;
	InternalGraph::GraphPrototype protoGraph;

	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<std::shared_ptr<Connection>> connections;

	std::shared_ptr<Node> outputNode;

};

