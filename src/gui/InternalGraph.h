#pragma once

#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "FastNoiseSIMD/FastNoiseSIMD.h"

#include <cml/cml.h>

#include <glm/glm.hpp>
#include <glm/packing.hpp>

namespace InternalGraph
{

using LinkTypeVariants = std::variant<int, float, cml::vec2f, cml::vec3f, cml::vec4f>;
using NodeID = int;

class Node;

struct NodeHandle
{
	NodeID id = -1;
	Node* handle;
};

// Convenience typedef of the map of nodes
typedef std::map<NodeID, Node> NodeMap;

template <typename T> class NoiseImage2D
{
	public:
	NoiseImage2D ();

	NoiseImage2D (int width);
	~NoiseImage2D ();

	// size, in pixels, of the image;
	int GetSize () const;

	// dimension of the image
	int GetImageWidth () const;

	void SetWidth (int width);

	size_t GetSizeBytes () const;

	// No error look
	const T LookUp (int x, int z) const;

	// Error checked sample, returns -1 if fails
	const T BoundedLookUp (int x, int z) const;

	void SetPixelValue (int x, int z, T value);

	T* GetImageData ();
	void SetImageData (int width, T* data, std::shared_ptr<FastNoiseSIMD> noiseHolder);

	std::vector<T>* GetImageVectorData ();

	std::byte* GetByteDataPtr ();

	private:
	int width = 0;
	std::shared_ptr<T> image;
	bool isExternallyAllocated = false;
	std::vector<T> data;
};

template <typename T>
static float BilinearImageSample2D (const NoiseImage2D<T>& noiseImage, const float x, const float z)
{
	const int cellScale = noiseImage.GetImageWidth () - 1; // cellsWide - 1;

	const float xScaled = x * (float)cellScale;
	const float zScaled = z * (float)cellScale;

	const int realX = (int)xScaled;
	const int realZ = (int)zScaled;

	const int realXPlus1 = (int)cml::clamp (
	    xScaled + 1, 0.0f, (float)cellScale); // make sure its not greater than the image size
	const int realZPlus1 = (int)cml::clamp (zScaled + 1, 0.0f, (float)cellScale);

	const float UL = noiseImage.BoundedLookUp (realX, realZ);
	const float UR = noiseImage.BoundedLookUp (realX, realZPlus1);
	const float DL = noiseImage.BoundedLookUp (realXPlus1, realZ);
	const float DR = noiseImage.BoundedLookUp (realXPlus1, realZPlus1);

	if (realX == realXPlus1 && realZ == realZPlus1)
	{
		return UL;
	}
	else if (realX == realXPlus1)
	{
		return (UL * ((float)realZPlus1 - zScaled) + UR * (zScaled - (float)realZ)) /
		       ((float)realZPlus1 - (float)realZ);
	}
	else if (realZ == realZPlus1)
	{
		return (UL * ((float)realXPlus1 - xScaled) + DL * (xScaled - (float)realX)) /
		       ((float)realXPlus1 - (float)realX);
	}
	else
	{

		return (UL * ((float)realXPlus1 - xScaled) * ((float)realZPlus1 - zScaled) +
		           DL * (xScaled - (float)realX) * ((float)realZPlus1 - zScaled) +
		           UR * ((float)realXPlus1 - xScaled) * (zScaled - (float)realZ) +
		           DR * (xScaled - (float)realX) * (zScaled - (float)realZ)) /
		       (((float)realXPlus1 - (float)realX) * ((float)realZPlus1 - (float)realZ));
	}
}

// No allocation constructor, meant for images that are externally created (FastNoiseSIMD)
template <typename T> NoiseImage2D<T>::NoiseImage2D () : width (0), isExternallyAllocated (true) {}

// Preallocates based on width, mainly meant for output images
template <typename T>
NoiseImage2D<T>::NoiseImage2D (int width) : width (width), isExternallyAllocated (false)
{

	data = std::vector<T> (width * width);
}

template <typename T> NoiseImage2D<T>::~NoiseImage2D ()
{
	// if (!isExternallyAllocated && image != nullptr) free (image);
	// Log::Error << "Was this image already freed?\n";
}

template <typename T> T* NoiseImage2D<T>::GetImageData ()
{
	if (isExternallyAllocated)
		return image.get ();
	else
		return data.data ();
}

template <typename T>
void NoiseImage2D<T>::SetImageData (int width, T* data, std::shared_ptr<FastNoiseSIMD> noiseHolder)
{
	this->width = width;
	image = std::shared_ptr<T> (data, [=](T* ptr) { noiseHolder->FreeNoiseSet (ptr); });
}

template <typename T> int NoiseImage2D<T>::GetSize () const { return width * width; }

template <typename T> size_t NoiseImage2D<T>::GetSizeBytes () const { return GetSize () * 4; }

template <typename T> int NoiseImage2D<T>::GetImageWidth () const { return width; }

// Changes the width then frees and allocates new image
template <typename T> void NoiseImage2D<T>::SetWidth (int width) { this->width = width; }

template <typename T> const T NoiseImage2D<T>::LookUp (int x, int z) const
{
	if (isExternallyAllocated)
		return image.get ()[x * width + z];
	else
		return data[x * width + z];
}

template <typename T> const T NoiseImage2D<T>::BoundedLookUp (int x, int z) const
{
	if (x >= 0 && x < width && z >= 0 && z < width)
	{
		if (isExternallyAllocated)
			return image.get ()[x * width + z];
		else
			return data[x * width + z];
	}
	else
	{
		throw new std::runtime_error ("out of bounds");
	}
	return -1;
}

template <typename T> void NoiseImage2D<T>::SetPixelValue (int x, int z, T value)
{


	if (x >= 0 && x < width && z >= 0 && z < width) data[x * width + z] = value;
	//	image[x * width + z] = value;
	else
		throw new std::runtime_error ("out of bounds");
}

template <typename T> std::vector<T>* NoiseImage2D<T>::GetImageVectorData () { return &data; }

template <typename T> std::byte* NoiseImage2D<T>::GetByteDataPtr ()
{
	return static_cast<std::byte*> (data.data ());
}

template <typename T>
static float BilinearImageSample2D (const NoiseImage2D<T>& noiseImage, const float x, const float z);

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
	VoronoiNoise,

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
	InputLink (cml::vec2f in);
	InputLink (cml::vec3f in);
	InputLink (cml::vec4f in);

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
	cml::vec2i pos;

	NoiseSourceInfo (int seed, int cellsWide, float scale, cml::vec2i pos)
	: seed (seed), cellsWide (cellsWide), scale (scale), pos (pos)
	{
	}
};

class Node
{
	public:
	Node (NodeType type = NodeType::None);
	//~Node ();

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
	std::shared_ptr<FastNoiseSIMD> myNoise;
	FastNoiseSIMD::FractalType fractalType;
	FastNoiseSIMD::CellularDistanceFunction cellularDistanceFunction;
	FastNoiseSIMD::CellularReturnType cellularReturnType;
};


class GraphPrototype
{
	public:
	GraphPrototype ();
	~GraphPrototype ();

	NodeID AddNode (NodeType type);

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
	GraphUser (const GraphPrototype& graph, int seed, int cellsWide, cml::vec2<int32_t> pos, float scale);

	float SampleHeightMap (const float x, const float z) const;

	int image_length () { return info.cellsWide; }

	std::vector<float>& GetHeightMap ();
	std::vector<cml::vec4<int8_t>>& GetSplatMap ();
	std::vector<cml::vec4<int16_t>>& GetNormalMap ();


	private:
	NodeMap nodeMap;

	NoiseSourceInfo info;

	std::vector<float> outputHeightMap;
	std::vector<cml::vec4<int8_t>> outputSplatMap;
	std::vector<cml::vec4<int16_t>> outputNormalMap;
};
} // namespace InternalGraph