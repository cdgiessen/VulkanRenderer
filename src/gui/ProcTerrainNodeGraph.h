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

namespace SampleNodeGraphWindow {
	// ImGui - standalone example application for Glfw + OpenGL 3, using programmable pipeline

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define sizeof_array(t) (sizeof(t) / sizeof(t[0]))
	const float NODE_SLOT_RADIUS = 5.0f;
	const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);
#define MAX_CONNECTION_COUNT 32

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
	static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static void error_callback(int error, const char* description)
	{
		fprintf(stderr, "Error %d: %s\n", error, description);
	}

	static uint32_t s_id = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum ConnectionType
	{
		ConnectionType_Color,
		ConnectionType_Vec3,
		ConnectionType_Float,
		ConnectionType_Int,
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct ConnectionDesc
	{
		const char* name;
		ConnectionType type;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct NodeType
	{
		const char* name;
		ConnectionDesc inputConnections[MAX_CONNECTION_COUNT];
		ConnectionDesc outputConnections[MAX_CONNECTION_COUNT];
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct Connection
	{
		ImVec2 pos;
		ConnectionDesc desc;

		inline Connection()
		{
			pos.x = pos.y = 0.0f;
			input = 0;
		}

		union {
			float v3[3];
			float v;
			int i;
		};

		struct Connection* input;
		std::vector<Connection*> output;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Node types

	static struct NodeType s_nodeTypes[] =
	{
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Math

		{
			"Multiply",
			// Input connections
			{
				{ "Input1", ConnectionType_Float },
				{ "Input2", ConnectionType_Float },
			},
			// Output
			{
				{ "Out", ConnectionType_Float },
			},
		},

		{
			"Add",
			// Input connections
			{
				{ "Input1", ConnectionType_Float },
				{ "Input2", ConnectionType_Float },
			},
			// Output
			{
				{ "Out", ConnectionType_Float },
				{ "Out2", ConnectionType_Float },
			},
		},
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct Node
	{
		ImVec2 pos;
		ImVec2 size;
		int id;
		const char* name;
		std::vector<Connection*> inputConnections;
		std::vector<Connection*> outputConnections;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum DragState
	{
		DragState_Default,
		DragState_Hover,
		DragState_BeginDrag,
		DragState_Draging,
		DragState_Connect,
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct DragNode
	{
		ImVec2 pos;
		Connection* con;
	};

	static DragNode s_dragNode;
	static DragState s_dragState = DragState_Default;

	static std::vector<Node*> s_nodes;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}

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
	ConnectionSlot(ImVec2 pos, ConnectionType type);
	ConnectionSlot(ImVec2 pos, ConnectionType type, std::string name);

	bool IsHoveredOver(ImVec2 parentPos);
	ConnectionType GetType();

	ImVec2 pos;

	float nodeSlotRadius = 5.0f;

	std::string name;

	ConnectionType conType;
};

class InputConnectionSlot : public ConnectionSlot {
public:
	InputConnectionSlot(ImVec2 pos, ConnectionType type, std::string name);

	bool hasConnection = false;
	std::shared_ptr<Connection> connection;
	SlotValueHolder value;
};

class OutputConnectionSlot : public ConnectionSlot {
public:
	OutputConnectionSlot(ImVec2 pos, ConnectionType type);

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

