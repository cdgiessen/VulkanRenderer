#pragma once

#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <memory>

#include "../../third-party/FastNoiseSIMD/FastNoiseSIMD.h"

#include "../resources/Texture.h"

#include <glm/glm.hpp>

namespace InternalGraph {

	typedef std::variant<int, float, glm::vec2, glm::vec3, glm::vec4> LinkTypeVariants;
	typedef int NodeID;

	class Node;

	struct NodeHandle {
		NodeID id = -1;
		Node* handle;
	};

	//Convinience typedef of the map of nodes
	typedef std::map<NodeID, Node> NodeMap;

	template<typename T>
	class NoiseImage2D {
	public:
		NoiseImage2D();

		NoiseImage2D(int width);
		~NoiseImage2D();

		//size, in pixels, of the image;
		const int GetSize() const;

		//dimention of the image
		const int GetImageWidth() const;

		void SetWidth(int width);

		const size_t GetSizeBytes() const;

		//No error look
		const T LookUp(int x, int z) const;

		//Error checked sample, returns -1 if fails
		const T BoundedLookUp(int x, int z) const;

		void SetPixelValue(int x, int z, T value);

		T* GetImageData();
		std::vector<T>* GetImageVectorData();
		void SetImageData(int width, T* data);

	private:
		int width = 0;
		T* image = nullptr;
		bool isExternallyAllocated = false;
		std::vector<T> data;
	};

	template<typename T>
	std::vector<T>* NoiseImage2D<T>::GetImageVectorData() {
		return &data;
	}

	template<typename T>
	static float BilinearImageSample2D(const NoiseImage2D<T>& noiseImage, const float x, const float z);

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
		Invert,
		TextureIndex,

		ValueFractalNoise,
		SimplexFractalNoise,
		PerlinFractalNoise,
		WhiteNoise,
		CellularNoise,
		CubicFractalNoise,
		VoroniFractalNoise,

		ColorCreator,
		MonoGradient,
	};

	class InputLink {
	public:
		InputLink();
		InputLink(float in);
		InputLink(int in);
		InputLink(glm::vec2 in);
		InputLink(glm::vec3 in);
		InputLink(glm::vec4 in);

		void SetInputNode(NodeID id);
		void ResetInputNode();
		NodeID GetInputNode();

		bool HasInputNode();

		void SetInputNodePointer(Node* node);
		void ResetInputNodePointer();

		void SetDataValue(LinkTypeVariants data);

		LinkTypeVariants GetValue();
		LinkTypeVariants GetValue(const int x, const int z);

	private:
		bool hasInputNode = false;
		NodeHandle handle;
		LinkTypeVariants value;
	};

	struct NoiseSourceInfo {
		int seed;
		int cellsWide;
		float scale;
		glm::i32vec2 pos;

		NoiseSourceInfo(int seed, int cellsWide, float scale, glm::i32vec2 pos) :
			seed(seed), cellsWide(cellsWide), scale(scale), pos(pos)
		{ }
	};

	class Node {
	public:
		Node(NodeType type = NodeType::None);

		NodeType GetNodeType();
		LinkType GetOutputType();

		void SetLinkValue(const int index, const LinkTypeVariants data);

		void SetLinkInput(const int index, const NodeID id);

		void ResetLinkInput(const int index);

		LinkTypeVariants GetValue(const int x, const int z);

		LinkTypeVariants GetHeightMapValue(const int x, const int z);
		LinkTypeVariants GetSplatMapValue(const int x, const int z);

		void SetID(NodeID);
		NodeID GetID();

		void SetIsNoiseNode(bool val);

		void SetupInputLinks(NodeMap* map);
		void SetupNodeForComputation(NoiseSourceInfo info);
		void CleanUp();

		std::vector <InputLink> inputLinks;
	
	private:
		NodeID id = -1;
		NodeType nodeType = NodeType::None;

		LinkType outputType = LinkType::None;

		bool isNoiseNode = false;
		NoiseImage2D<float> noiseImage;
		FastNoiseSIMD* myNoise;
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
		void SetOutputNodeID(NodeID id);

		NodeMap GetNodeMap() const; //to copy

		void ResetGraph();

	private:
		NodeID nodeIDCounter = 0;
		NodeMap nodeMap;

		NodeID outputNodeID;
	};

	class GraphUser {
	public:
		GraphUser(const GraphPrototype& graph, int seed, int cellsWide, glm::i32vec2 pos, float scale);

		float SampleHeightMap(const float x, const float z);
		NoiseImage2D<float>& GetHeightMap();

		NoiseImage2D<RGBA_pixel>& GetSplatMap();

	private:
		NodeMap nodeMap;
		Node* outputNode;

		NoiseSourceInfo info;

		NoiseImage2D<float> outputHeightMap;
		NoiseImage2D<RGBA_pixel> outputSplatmap;
	};
}