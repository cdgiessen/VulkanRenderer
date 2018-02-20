#pragma once

#include <vector>
#include <map>
#include <variant>
#include <memory>

#include "../../third-party/FastNoiseSIMD/FastNoiseSIMD.h"

#include <glm/glm.hpp>

namespace InternalGraph {

	typedef std::variant<float, int, glm::vec2, glm::vec3, glm::vec4> LinkTypeVariants;
	typedef int NodeID;


	class NoiseImage2D {
	public:
		NoiseImage2D();

		NoiseImage2D(int width);
		~NoiseImage2D();

		//resize underlying vector to appropriate size
		void PrepareImage();

		//size, in pixels, of the image;
		const int GetSize() const;

		//dimention of the image
		const int GetImageWidth() const;

		void SetWidth(int width);

		const size_t GetSizeBytes() const;

		//No error look
		const float LookUp(int x, int z) const;

		//Error checked sample, returns -1 if fails
		const float BoundedLookUp(int x, int z) const;

		void SetPixelValue(int x, int z, float value);

		float* GetImageData();
		void SetImageData(float* data);

	private:
		int width;
		float* image;
	};

	static float BilinearImageSample2D(const NoiseImage2D& noiseImage, const float x, const float z);

	enum class LinkType {
		None, //ErrorType or just no output, like an outputNode....
		Float,
		Int,
		Vec2,
		Vec3,
		Vec4,
	};

	enum class NodeType {
		None, //error type
		Output,
		Addition,
		Subtraction,
		Multiplication,
		Division,
		Power,
		Blend,
		Clamp,
		Max,
		Min,

		ConstantInt,
		ConstantFloat,

		ValueFractalNoise,
		SimplexFractalNoise,
		PerlinFractalNoise,
		WhiteNoise,
		CellularNoise,
		CubicFractalNoise,
		VoroniFractalNoise


	};

	class Node;

	struct NodeHandle {
		NodeID id = -1;
		Node* handle;
	};

	template<typename T>
	class InputLink {
	public:

		void SetInputNode(NodeID id);
		void ResetInputNode();
		void SetInputNodePointer(Node* node);
		void ResetInputNodePointer();

		void SetDataValue(T data);

		T GetValue(const int x, const int z, T dummy);

	private:
		bool hasInputNode;
		NodeHandle handle;
		T value;
	};

	//Defines all the possible input link types. This is so templates can be used and the input links can be put in a vector for easy storage
	typedef std::variant < InputLink<float>, InputLink<int>, InputLink < glm::vec2>, InputLink<glm::vec3>, InputLink<glm::vec4>> InputLinkTypeVariant;

	//Convinience typedef of the map of nodes
	typedef std::map<NodeID, Node> NodeMap;

	class Node {
	public:
		Node(NodeType type = NodeType::None);

		LinkType GetOutputType();

		template<typename T>
		void SetLinkValue(const int index, const T data);

		void SetLinkInput(const int index, const NodeID id);

		void ResetLinkInput(const int index);

		template<typename T>
		T GetValue(const int x, const int z, T dummy);

		void SetID(NodeID);
		NodeID GetID();

		void SetIsNoiseNode(bool val);

		void SetupNodeForComputation(NodeMap& map);

	private:
		NodeID id = -1;
		NodeType nodeType = NodeType::None;

		std::vector <InputLinkTypeVariant> inputLinks;
		LinkType outputType = LinkType::None;

		bool isNoiseNode = false;
	};


	class GraphPrototype {
	public:
		GraphPrototype();
		~GraphPrototype();

		NodeID AddNode(Node node);
		NodeID AddNoiseNoide(Node node);

		bool DeleteNode(NodeID id);

		Node& GetNodeByID(NodeID id);
		NodeID GetNextID();

		NodeID GetOutputNodeID() const;

		NodeMap GetNodeMap() const; //to copy

	private:
		NodeID nodeIDCounter = 0;
		NodeMap nodeMap;

		NodeID outputNodeID;
	};

	class GraphUser {
	public:
		GraphUser(const GraphPrototype& graph, int seed, int cellsWide, glm::i32vec2 pos, float scale);

		float SampleGraph(const float x, const float z);
		float* GetGraphSourceImage();

	private:
		NodeMap nodeMap;

		int seed;
		glm::i32vec2 pos;
		float scale;
		int cellsWide;

		NoiseImage2D outputImage;
	};

	template<typename T>
	void Node::SetLinkValue(const int index, const T data) {
		//inputLinks.at(index).SetDataValue(data);
	}

	template<typename T>
	T Node::GetValue(const int x, const int z, T dummy) {

	}
}