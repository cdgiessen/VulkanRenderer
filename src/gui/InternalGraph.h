#pragma once

#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "FastNoiseSIMD/FastNoiseSIMD.h"

#include <glm/glm.hpp>

namespace InternalGraph
{

using LinkTypeVariants = std::variant<int, float, glm::vec2, glm::vec3, glm::vec4>;
using NodeID = int;

class Node;

struct NodeHandle
{
	NodeID id = -1;
	Node* handle;
};

// Convinience typedef of the map of nodes
typedef std::map<NodeID, Node> NodeMap;

template <typename T> class NoiseImage2D
{
	public:
	NoiseImage2D ();

	NoiseImage2D (int width);
	~NoiseImage2D ();

	// size, in pixels, of the image;
	const int GetSize () const;

	// dimention of the image
	const int GetImageWidth () const;

	void SetWidth (int width);

	const size_t GetSizeBytes () const;

	// No error look
	const T LookUp (int x, int z) const;

	// Error checked sample, returns -1 if fails
	const T BoundedLookUp (int x, int z) const;

	void SetPixelValue (int x, int z, T value);

	T* GetImageData ();
	void SetImageData (int width, T* data);

	std::vector<T>* GetImageVectorData ();

	std::byte* GetByteDataPtr ();

	private:
	int width = 0;
	T* image = nullptr;
	bool isExternallyAllocated = false;
	std::vector<T> data;
};

template <typename T> std::vector<T>* NoiseImage2D<T>::GetImageVectorData () { return &data; }

template <typename T> std::byte* NoiseImage2D<T>::GetByteDataPtr ()
{
	return static_cast<std::byte*> (data.data ());
}



template <typename T>
static const float BilinearImageSample2D (const NoiseImage2D<T>& noiseImage, const float x, const float z);

enum class LinkType
{
	None, // ErrorType or just no output, like an outputNode....
	Float,
	Int,
	Vec2,
	Vec3,
	Vec4,
};

enum class NodeType
{
	None, // error type
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
	FractalReturnType,
	CellularReturnType,

	WhiteNoise,

	ValueNoise,
	SimplexNoise,
	PerlinNoise,
	CubicNoise,

	CellNoise,
	VoroniNoise,

	ColorCreator,
	MonoGradient,
	Selector,
};

class InputLink
{
	public:
	InputLink ();
	InputLink (float in);
	InputLink (int in);
	InputLink (glm::vec2 in);
	InputLink (glm::vec3 in);
	InputLink (glm::vec4 in);

	void SetInputNode (NodeID id);
	void ResetInputNode ();
	NodeID GetInputNode () const;

	bool HasInputNode () const;

	void SetInputNodePointer (Node* node);
	void ResetInputNodePointer ();

	void SetDataValue (LinkTypeVariants data);

	LinkTypeVariants GetValue () const;
	LinkTypeVariants GetValue (const int x, const int z) const;

	private:
	bool hasInputNode = false;
	NodeHandle handle;
	LinkTypeVariants value;
};

struct NoiseSourceInfo
{
	int seed;
	int cellsWide;
	float scale;
	glm::i32vec2 pos;

	NoiseSourceInfo (int seed, int cellsWide, float scale, glm::i32vec2 pos)
	: seed (seed), cellsWide (cellsWide), scale (scale), pos (pos)
	{
	}
};

class Node
{
	public:
	Node (NodeType type = NodeType::None);

	NodeType GetNodeType () const;
	LinkType GetOutputType () const;

	void SetLinkValue (const int index, const LinkTypeVariants data);

	void SetLinkInput (const int index, const NodeID id);

	void ResetLinkInput (const int index);

	LinkTypeVariants GetValue (const int x, const int z) const;

	LinkTypeVariants GetHeightMapValue (const int x, const int z) const;
	LinkTypeVariants GetSplatMapValue (const int x, const int z) const;
	LinkTypeVariants GetVegetationMapValue (const int x, const int z) const;

	void SetID (NodeID);
	NodeID GetID ();

	void SetIsNoiseNode (bool val);

	void SetupInputLinks (NodeMap* map);
	void SetupNodeForComputation (NoiseSourceInfo info);
	void CleanNoise ();

	std::vector<InputLink> inputLinks;

	private:
	void SetFractalType (int index);
	void SetCellularDistanceFunction (int index);
	void SetCellularReturnType (int index);


	NodeID id = -1;
	NodeType nodeType = NodeType::None;

	LinkType outputType = LinkType::None;

	bool isNoiseNode = false;
	NoiseImage2D<float> noiseImage;
	FastNoiseSIMD* myNoise;
	FastNoiseSIMD::FractalType fractalType;
	FastNoiseSIMD::CellularDistanceFunction cellularDistanceFunction;
	FastNoiseSIMD::CellularReturnType cellularReturnType;
};


class GraphPrototype
{
	public:
	GraphPrototype ();
	~GraphPrototype ();

	NodeID AddNode (Node node);
	NodeID AddNoiseNoide (Node node);

	bool DeleteNode (NodeID id);

	Node& GetNodeByID (NodeID id);
	NodeID GetNextID ();

	NodeID GetOutputNodeID () const;
	void SetOutputNodeID (NodeID id);

	NodeMap GetNodeMap () const; // to copy

	void ResetGraph ();

	private:
	NodeID nodeIDCounter = 0;
	NodeMap nodeMap;

	NodeID outputNodeID;
};

class GraphUser
{
	public:
	GraphUser (const GraphPrototype& graph, int seed, int cellsWide, glm::i32vec2 pos, float scale);

	const float SampleHeightMap (const float x, const float z) const;
	NoiseImage2D<float>& GetHeightMap ();

	std::byte* GetSplatMapPtr ();

	NoiseImage2D<uint8_t>& GetVegetationDensityMap ();

	private:
	NodeMap nodeMap;
	Node* outputNode;

	NoiseSourceInfo info;

	NoiseImage2D<float> outputHeightMap;
	std::vector<std::byte> outputSplatmap;

	NoiseImage2D<uint8_t> vegetationDensityMap;
};
} // namespace InternalGraph