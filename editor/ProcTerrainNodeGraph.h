#pragma once

#include <stdio.h>

#include <algorithm>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "InternalGraph.h"

#include "imgui.hpp"

#include "cml/cml.h"


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

using SlotValueVariant = std::variant<int, float, cml::vec2f, cml::vec3f, cml::vec4f>;

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

	// int Draw (ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset) = 0;
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

	int Draw (ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset);

	ConId connection = -1;
	SlotValueHolder value;
	float sliderStepSize = 0.1f, lowerBound = 0.0f, upperBound = 1.0f;
};

class OutputConnectionSlot : public ConnectionSlot
{
	public:
	OutputConnectionSlot (int slotNum, ImVec2 pos, ConnectionType type);

	int Draw (ImDrawList* imDrawList, ProcTerrainNodeGraph& graph, const Node& parentNode, const int verticalOffset);

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
	VoronoiNoise,

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

	std::string name;
	NodeType nodeType;
	NodeId id = -1;
	ImVec2 pos = ImVec2 (200, 150);
	ImVec2 size = ImVec2 (100, 100);

	std::vector<InputConnectionSlot> inputSlots;
	OutputConnectionSlot outputSlot;

	void AddInputSlot (ConnectionType type, std::string name);
	void AddInputSlot (
	    ConnectionType type, std::string name, float defaultValue, float sliderStepSize, float lowerBound, float upperBound);
	void AddInputSlot (
	    ConnectionType type, std::string name, int defaultValue, float sliderStepSize, float lowerBound, float upperBound);
	void AddInputSlot (ConnectionType type, std::string name, cml::vec2f defaultValue);
	void AddInputSlot (ConnectionType type, std::string name, cml::vec3f defaultValue);
	void AddInputSlot (ConnectionType type, std::string name, cml::vec4f defaultValue);

	InternalGraph::NodeID internalNodeID;

	bool hasTextInput = false;
};

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

namespace Input
{
class InputDirector;
}
class ProcTerrainNodeGraph
{
	public:
	ProcTerrainNodeGraph (Input::InputDirector& input);
	~ProcTerrainNodeGraph ();

	void Draw ();

	InternalGraph::GraphPrototype& GetGraph ();

	private:
	Input::InputDirector& input;
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
	void SaveGraphFromFile (std::string file_name);
	void LoadGraphFromFile ();
	void LoadGraphFromFile (std::string file_name);
	void BuildTerGenNodeGraph ();

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

	const ImVec2 windowPadding = ImVec2 (8.0f, 8.0f);
	ImVec2 windowPos = ImVec2 (0, 0);
	ImVec2 graphOffset = ImVec2 (0, 0);
	ImVec2 startingNodePos = ImVec2 (200, 150);

	bool window_open;
	int curID = 0;
	PossibleConnection posCon;

	InternalGraph::GraphPrototype protoGraph;

	std::unordered_map<NodeId, Node> nodes;
	int nextConId = 0;
	std::unordered_map<ConId, Connection> connections;

	NodeId outputNode;
};
