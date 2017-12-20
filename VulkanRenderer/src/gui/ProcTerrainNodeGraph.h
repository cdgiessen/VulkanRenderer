#pragma once

#include "..\third-party\ImGui\imgui.h"
#include <stdio.h>

#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <stdint.h>
//#include <jansson.h>
//#include "dialogs.h"

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



class Connection {
public:
	std::string name;

	enum class ConnectionType
	{
		ConnectionType_Color,
		ConnectionType_Vec3,
		ConnectionType_Float,
		ConnectionType_Int,
	} type;

	ImVec2 pos = ImVec2(0.0f,0.0f);

	union {
		float v4[4];
		float v3[3];
		float v2[2];
		float v1;
		int i;
	};

	std::shared_ptr<Connection> input;
	std::vector<std::shared_ptr<Connection>> output;
};

class Node {
public:
	std::string name;
	ImVec2 pos;
	ImVec2 size;
	int id;
	std::vector<std::shared_ptr<Connection>> inputConnections;
	std::vector<std::shared_ptr<Connection>> outputConnections;

	void Draw(ImDrawList*  imDrawList, ImVec2 offset);
};

class ProcTerrainNodeGraph
{
public:
	ProcTerrainNodeGraph();
	~ProcTerrainNodeGraph();

	void Draw();

private:
	bool window_open;
	void DrawMenuBar();
	void DrawNodeButtons();
	void DrawNodeCanvas();

	void DrawNodes(ImDrawList*  imDrawList);

	std::vector<std::shared_ptr<Node>> Nodes;

	std::shared_ptr<Node> DragNode;
	
	enum class DragState
	{
		DragState_Default,
		DragState_Hover,
		DragState_BeginDrag,
		DragState_Draging,
		DragState_Connect,
	} dragState;
};

