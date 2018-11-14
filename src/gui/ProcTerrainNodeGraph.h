#pragma once

#include <stdio.h>

#include <algorithm>
#include <math.h>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "InternalGraph.h"

#include "ImGui/imgui.h"


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

using SlotValueVariant = std::variant<int, float, glm::vec2, glm::vec3, glm::vec4>;

struct SlotValueHolder
{

	SlotValueVariant value;


	ConnectionType type = ConnectionType::Null;

	SlotValueHolder (ConnectionType type) : type (type){};
};

using NodeId = int;
using ConId = int;

class Node;
class ProcTerrainNodeGraph;

class Connection
{
	public:
	Connection (ConnectionType conType, NodeId input, NodeId output, int output_slot_id);

	ConnectionType conType;

	ImVec2 startPosRelNode = ImVec2 (0.0f, 0.0f);
	ImVec2 endPosRelNode = ImVec2 (0.0f, 0.0f);

	NodeId input = -1;
	NodeId output = -1;

	int output_slot_id = -1;
};

class ConnectionSlot
{
	public:
	ConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type);
	ConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type, std::string name);

	virtual int Draw (
	    ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset) = 0;
	bool IsHoveredOver (ImVec2 parentPos) const;
	ConnectionType GetType ();

	int slotNum;
	ImVec2 pos;
	float nodeSlotRadius = 5.0f;
	ImColor slotColor = ImColor (150, 150, 150);

	std::string name;

	ConnectionType conType;
};

class InputConnectionSlot : public ConnectionSlot
{
	public:
	InputConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type, std::string name);
	InputConnectionSlot (
	    int slotNum, ImVec2 pos, ConnectionType type, std::string name, SlotValueVariant defaultValue);
	InputConnectionSlot (int slotNum,
	    ImVec2 pos,
	    ConnectionType type,
	    std::string name,
	    SlotValueVariant defaultValue,
	    float sliderStepSize,
	    float lowerBound,
	    float upperBound);

	int Draw (ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset) override;

	ConId connection = -1;
	SlotValueHolder value;
	float sliderStepSize = 0.1f, lowerBound = 0.0f, upperBound = 1.0f;
};

class OutputConnectionSlot : public ConnectionSlot
{
	public:
	OutputConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type);

	int Draw (ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset) override;

	std::vector<ConId> connections;
};

enum class NodeType
{
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
	Selector,

	WhiteNoise,

	ValueNoise,
	SimplexNoise,
	PerlinNoise,
	CubicNoise,

	CellNoise,
	VoroniNoise,

	ConstantInt,
	ConstantFloat,
	Invert,
	TextureIndex,
	FractalReturnType,
	CellularReturnType,

	ColorCreator,
	MonoGradient,
};

class Node
{
	public:
	Node (NodeType type, NodeId id, ImVec2 position, InternalGraph::GraphPrototype& graph);

	// void SetInternalLink(int index, std::shared_ptr<NewNodeGraph::INode> inode);

	std::string name;
	ImVec2 pos = ImVec2 (200, 150);
	ImVec2 size = ImVec2 (100, 100);
	NodeId id = -1;
	NodeType nodeType;

	std::vector<InputConnectionSlot> inputSlots;
	OutputConnectionSlot outputSlot;

	void AddInputSlot (ConnectionType type, std::string name);
	void AddInputSlot (
	    ConnectionType type, std::string name, float defaultValue, float sliderStepSize, float lowerBound, float upperBound);
	void AddInputSlot (
	    ConnectionType type, std::string name, int defaultValue, float sliderStepSize, float lowerBound, float upperBound);
	void AddInputSlot (ConnectionType type, std::string name, glm::vec2 defaultValue);
	void AddInputSlot (ConnectionType type, std::string name, glm::vec3 defaultValue);
	void AddInputSlot (ConnectionType type, std::string name, glm::vec4 defaultValue);

	InternalGraph::NodeID internalNodeID;
	// std::shared_ptr<NewNodeGraph::INode> internal_node;
	// void Draw(ImDrawList*  imDrawList, ImVec2 offset);

	bool hasTextInput = false;
};

// class OutputNode : public Node
// {
// 	public:
// 	OutputNode (InternalGraph::GraphPrototype& graph);
// };

// class MathNode : public Node
// {
// 	public:
// 	MathNode (std::string name);
// };

// class AdditionNode : public MathNode
// {
// 	public:
// 	AdditionNode (InternalGraph::GraphPrototype& graph);
// };
// class SubtractionNode : public MathNode
// {
// 	public:
// 	SubtractionNode (InternalGraph::GraphPrototype& graph);
// };
// class MultiplicationNode : public MathNode
// {
// 	public:
// 	MultiplicationNode (InternalGraph::GraphPrototype& graph);
// };
// class DivisionNode : public MathNode
// {
// 	public:
// 	DivisionNode (InternalGraph::GraphPrototype& graph);
// };
// class PowerNode : public MathNode
// {
// 	public:
// 	PowerNode (InternalGraph::GraphPrototype& graph);
// };
// class MaxNode : public MathNode
// {
// 	public:
// 	MaxNode (InternalGraph::GraphPrototype& graph);
// };
// class MinNode : public MathNode
// {
// 	public:
// 	MinNode (InternalGraph::GraphPrototype& graph);
// };



// class BlendNode : public Node
// {
// 	public:
// 	BlendNode (InternalGraph::GraphPrototype& graph);
// };
// class ClampNode : public Node
// {
// 	public:
// 	ClampNode (InternalGraph::GraphPrototype& graph);
// };
// class SelectorNode : public Node
// {
// 	public:
// 	SelectorNode (InternalGraph::GraphPrototype& graph);
// };

// class NoiseNode : public Node
// {
// 	public:
// 	NoiseNode (std::string name);
// };

// class FractalNoiseNode : public NoiseNode
// {
// 	public:
// 	FractalNoiseNode (std::string name);
// };
// class CellularNoiseNode : public NoiseNode
// {
// 	public:
// 	CellularNoiseNode (std::string name);
// };

// class WhiteNoiseNode : public NoiseNode
// {
// 	public:
// 	WhiteNoiseNode (InternalGraph::GraphPrototype& graph);
// };

// class PerlinNode : public FractalNoiseNode
// {
// 	public:
// 	PerlinNode (InternalGraph::GraphPrototype& graph);
// };
// class SimplexNode : public FractalNoiseNode
// {
// 	public:
// 	SimplexNode (InternalGraph::GraphPrototype& graph);
// };
// class ValueNode : public FractalNoiseNode
// {
// 	public:
// 	ValueNode (InternalGraph::GraphPrototype& graph);
// };
// class CubicNode : public FractalNoiseNode
// {
// 	public:
// 	CubicNode (InternalGraph::GraphPrototype& graph);
// };

// class FractalReturnType : public Node
// {
// 	public:
// 	FractalReturnType (InternalGraph::GraphPrototype& graph);
// };

// class CellNoiseNode : public CellularNoiseNode
// {
// 	public:
// 	CellNoiseNode (InternalGraph::GraphPrototype& graph);
// };
// class VoroniNode : public CellularNoiseNode
// {
// 	public:
// 	VoroniNode (InternalGraph::GraphPrototype& graph);
// };

// class CellularReturnType : public Node
// {
// 	public:
// 	CellularReturnType (InternalGraph::GraphPrototype& graph);
// };

// class ConstantIntNode : public Node
// {
// 	public:
// 	ConstantIntNode (InternalGraph::GraphPrototype& graph);
// };
// class ConstantFloatNode : public Node
// {
// 	public:
// 	ConstantFloatNode (InternalGraph::GraphPrototype& graph);
// };
// class TextureIndexNode : public Node
// {
// 	public:
// 	TextureIndexNode (InternalGraph::GraphPrototype& graph);
// };
// class InvertNode : public Node
// {
// 	public:
// 	InvertNode (InternalGraph::GraphPrototype& graph);
// };


// class ColorCreator : public Node
// {
// 	public:
// 	ColorCreator (InternalGraph::GraphPrototype& graph);
// };
// class MonoGradient : public Node
// {
// 	public:
// 	MonoGradient (InternalGraph::GraphPrototype& graph);
// };

struct HoveredSlotInfo
{
	NodeId node = -1;
	bool isOutput = false;
	int inputSlotNum = 0;
	HoveredSlotInfo () : node (-1), isOutput (false), inputSlotNum (-1){};
	HoveredSlotInfo (NodeId node, bool output, int inSlotNum)
	: node (node), isOutput (output), inputSlotNum (inSlotNum)
	{
	}
};

class PossibleConnection
{
	public:
	PossibleConnection ();

	static ImVec2 GetActiveSlotPosition (HoveredSlotInfo info, std::unordered_map<NodeId, Node> nodes);
	static ConnectionType GetActiveSlotType (HoveredSlotInfo info, std::unordered_map<NodeId, Node> nodes);

	enum class State
	{
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
	ProcTerrainNodeGraph ();
	~ProcTerrainNodeGraph ();

	void Draw ();

	InternalGraph::GraphPrototype& GetGraph ();

	private:
	friend class Node;
	friend class ConnectionSlot;
	friend class InputConnectionSlot;
	friend class OutputConnectionSlot;

	void DrawMenuBar ();
	void DrawButtonBar ();
	void DrawNodeButtons ();
	void DrawNodeCanvas ();

	void DrawHermite (ImDrawList* imDrawList, ImVec2 p1, ImVec2 p2, int STEPS);
	void DrawNodes (ImDrawList* imDrawList);
	void DrawConnections (ImDrawList* imDrawList);
	void DrawPossibleConnection (ImDrawList* imDrawList);

	void SaveGraphFromFile ();
	void SaveGraphFromFile (std::string fileName);
	void LoadGraphFromFile ();
	void LoadGraphFromFile (std::string fileName);
	void BuildTerGenNodeGraph ();

	// NodeId NewNode (NodeType type, ConnectionType outputType);
	ConId NewCon (ConnectionType conType, NodeId input, NodeId output, int output_slot_id);

	void DeleteNode (NodeId node);
	void DeleteConnection (ConId con);
	void ResetGraph ();
	void RecreateOutputNode ();

	HoveredSlotInfo GetHoveredSlot ();

	NodeId AddNode (NodeType nodeType, ImVec2 position, int id = -1);

	void SetNodeInternalLinkByID (InternalGraph::NodeID internalNodeID, int index, InternalGraph::NodeID id);
	void ResetNodeInternalLinkByID (InternalGraph::NodeID internalNodeID, int index);

	void SetNodeInternalValueByID (
	    InternalGraph::NodeID internalNodeID, int index, InternalGraph::LinkTypeVariants val);

	// NodeId GetNodeById (int id);

	const ImVec2 windowPadding = ImVec2 (8.0f, 8.0f);
	ImVec2 windowPos = ImVec2 (0, 0);
	ImVec2 graphOffset = ImVec2 (0, 0);
	ImVec2 startingNodePos = ImVec2 (200, 150);

	bool window_open;
	int curID = 0;
	PossibleConnection posCon;

	// NewNodeGraph::TerGenNodeGraph curGraph;
	InternalGraph::GraphPrototype protoGraph;

	int nextNodeId = 0;
	std::unordered_map<NodeId, Node> nodes;
	int nextConId = 0;
	std::unordered_map<ConId, Connection> connections;

	// std::vector<std::shared_ptr<Node>> nodes;
	// std::vector<std::shared_ptr<Connection>> connections;

	NodeId outputNode;
};
