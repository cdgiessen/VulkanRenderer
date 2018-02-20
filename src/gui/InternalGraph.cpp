#include "InternalGraph.h"

#include "../core/Logger.h"


namespace InternalGraph {

	static float BilinearImageSample2D(const NoiseImage2D& noiseImage, const float x, const float z) {
		int cellScale = noiseImage.GetImageWidth() - 1; // cellsWide - 1;

		float xScaled = x * (float)cellScale;
		float zScaled = z * (float)cellScale;

		int realX = (int)glm::clamp(xScaled, 0.0f, (float)cellScale);
		int realZ = (int)glm::clamp(zScaled, 0.0f, (float)cellScale);

		int realXPlus1 = (int)glm::clamp(xScaled + 1, 0.0f, (float)cellScale); //make sure its not greater than the image size
		int realZPlus1 = (int)glm::clamp(zScaled + 1, 0.0f, (float)cellScale);

		float UL = noiseImage.BoundedLookUp(realX, realZ);
		float UR = noiseImage.BoundedLookUp(realX, realZPlus1);
		float DL = noiseImage.BoundedLookUp(realXPlus1, realZ);
		float DR = noiseImage.BoundedLookUp(realXPlus1, realZPlus1);

		if (realX == realXPlus1 && realZ == realZPlus1) {
			return DR;
		}
		else if (realX == realXPlus1) {
			return (DR * (realZPlus1 - zScaled) + DL * (zScaled - realZ)) / (realZPlus1 - realZ);
		}
		else if (realZ == realZPlus1) {
			return (DR * (realXPlus1 - xScaled) + UR * (xScaled - realX)) / (realXPlus1 - realX);
		}
		else {

			return (
				UL * (realXPlus1 - xScaled)	* (realZPlus1 - zScaled)
				+ DL * (xScaled - realX)	* (realZPlus1 - zScaled)
				+ UR * (realXPlus1 - xScaled)	* (zScaled - realZ)
				+ DR * (xScaled - realX)	* (zScaled - realZ)
				)
				/ ((realXPlus1 - realX) * (realZPlus1 - realZ));
		}
	}

	//No allocation constructor, meant for images that are externally created (FastNoiseSIMD)
	NoiseImage2D::NoiseImage2D() : width(0) {}

	//Preallocates based on width, mainly meant for output images
	NoiseImage2D::NoiseImage2D(int width) : width(width) {
		image = (float*)malloc(GetSizeBytes());
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < width; j++)
			{
				SetPixelValue(i, j, -1);
			}
		}
	}

	NoiseImage2D::~NoiseImage2D() {
		if (image == nullptr)
			Log::Error << "Was this image already freed?\n";
		free(image);
	}

	void NoiseImage2D::PrepareImage() {
		free(image);
		image = (float*)malloc(GetSizeBytes());
	}

	float* NoiseImage2D::GetImageData() {
		return image;
	}

	void NoiseImage2D::SetImageData(float* data) {
		image = data;
	}

	const int NoiseImage2D::GetSize() const {
		return width * width;
	}

	const size_t NoiseImage2D::GetSizeBytes() const {
		return GetSize() * 4;
	}

	const int NoiseImage2D::GetImageWidth() const {
		return width;
	}

	//Changes the width then frees and allocates new image 
	void NoiseImage2D::SetWidth(int width) {
		this->width = width;
	}

	const float NoiseImage2D::LookUp(int x, int z) const {
		return image[x * width + z];
	}

	const float NoiseImage2D::BoundedLookUp(int x, int z) const {
		if (x >= 0 && x < width && z >= 0 && z < width)
			return image[x * width + z];
		else
			throw new std::runtime_error("out of bounds");
		return -1;
	}

	void NoiseImage2D::SetPixelValue(int x, int z, float value) {
		if (x >= 0 && x < width && z >= 0 && z < width)
			image[x * width + z] = value;
		else
			throw new std::runtime_error("out of bounds");
	}


	template<typename T>
	void InputLink<T>::SetInputNode(NodeID id) {
		handle.id = id;
		hasInputNode = true;
	}

	template<typename T>
	void InputLink<T>::ResetInputNode() {
		handle.id = -1;
		hasInputNode = false;
		handle.handle = nullptr;
	}

	template<typename T>
	void InputLink<T>::SetInputNodePointer(Node* node) {
		hasInputNode = true;
		handle.handle = node;
	}

	template<typename T>
	void InputLink<T>::ResetInputNodePointer() {
		hasInputNode = false;
		handle.handle = nullptr;
	}

	template<typename T>
	void InputLink<T>::SetDataValue(T data) {
		value = data;
	}

	template<typename T>
	T InputLink<T>::GetValue(const int x, const int z, T dummy) {
		if (hasInputNode)
			return handle.handle->GetValue(x, z, dummy);
		else
			return value;
	}

	Node::Node(NodeType in_type) : nodeType(in_type){

		switch (in_type)
		{
		case InternalGraph::NodeType::Output:
			inputLinks.push_back(InputLink<float>());
			break;

		case InternalGraph::NodeType::Addition:
		case InternalGraph::NodeType::Subtraction:
		case InternalGraph::NodeType::Multiplication:
		case InternalGraph::NodeType::Division:
		case InternalGraph::NodeType::Power:
			inputLinks.push_back(InputLink<float>());
			inputLinks.push_back(InputLink<float>());
			break;

		case InternalGraph::NodeType::Blend:
			break;
		case InternalGraph::NodeType::Clamp:
			break;
		case InternalGraph::NodeType::ConstantInt:
			inputLinks.push_back(InputLink<int>());
			break;
		case InternalGraph::NodeType::ConstantFloat:
			inputLinks.push_back(InputLink<float>());
			break;

		case InternalGraph::NodeType::ValueFractalNoise:
			isNoiseNode = true;
			break;
		case InternalGraph::NodeType::SimplexFractalNoise:
			isNoiseNode = true;
			break;
		case InternalGraph::NodeType::PerlinFractalNoise:
			isNoiseNode = true;
			break;
		case InternalGraph::NodeType::WhiteNoise:
			isNoiseNode = true;
			break;
		case InternalGraph::NodeType::CellularNoise:
			isNoiseNode = true;
			break;
		case InternalGraph::NodeType::CubicFractalNoise:
			isNoiseNode = true;
			break;
		case InternalGraph::NodeType::VoroniFractalNoise:
			isNoiseNode = true;
			break;
		default:
			break;
		}


	}

	LinkType Node::GetOutputType() {
		return outputType;
	}


	void Node::SetLinkInput(const int index, const NodeID id) {
		//try {
		//	//std::visit(SetInputLinkDataValue{}, inputLinks.at(index));
		///*	int varIndex = inputLinks.at(index).index;
		//
		//	std::get<InputLink<float>>(inputLinks.at(index)).SetInputNode(id);
		//	std::get<InputLink<int>>(inputLinks.at(index)).SetInputNode(id);
		//
		//
		//	inputLinks.at(index).SetInputNode(id);*/
		//}
		//catch (std::out_of_range& e) {
		//	Log::Error << "Tried accessing illegal input node link\n";
		//}
	}

	void Node::ResetLinkInput(const int index) {

	}

	void Node::SetID(NodeID id) {
		this->id = id;
	}
	NodeID Node::GetID() {
		return id;
	}

	void Node::SetIsNoiseNode(bool val) {
		isNoiseNode = val;
	}

	void Node::SetupNodeForComputation(NodeMap& map) {
		
	}

	GraphPrototype::GraphPrototype() {
		Node outputNode(NodeType::Output);
		outputNodeID = AddNode(outputNode);

	}
	GraphPrototype::~GraphPrototype() {
	}

	NodeID GraphPrototype::AddNode(Node node) {
		node.SetID(GetNextID());
		nodeMap[node.GetID()] = node;

		return node.GetID();
	}
	NodeID GraphPrototype::AddNoiseNoide(Node node) {
		node.SetID(GetNextID());
		node.SetIsNoiseNode(true);
		nodeMap[node.GetID()] = node;

		return node.GetID();
	}

	bool GraphPrototype::DeleteNode(NodeID id) {
		auto val = nodeMap.find(id);
		if (val != nodeMap.end()) {
			nodeMap.erase(id);
			return true;
		} 
		else return false;
	}

	Node& GraphPrototype::GetNodeByID(NodeID id) {
		return nodeMap.at(id);

	}
	NodeID GraphPrototype::GetNextID() {
		return nodeIDCounter++;
	}

	NodeID GraphPrototype::GetOutputNodeID() const {
		return outputNodeID;
	}

	NodeMap GraphPrototype::GetNodeMap() const {
		return nodeMap;
	}


	GraphUser::GraphUser(const GraphPrototype& graph, 
		int seed, int cellsWide, glm::i32vec2 pos, float scale) {

		this->nodeMap = graph.GetNodeMap();

		for (auto node : nodeMap) {
			node.second.SetupNodeForComputation(nodeMap);
		}

	}

	float GraphUser::SampleGraph(const float x, const float z) {
		return BilinearImageSample2D(outputImage, x, z);
	}
	float* GraphUser::GetGraphSourceImage() {
		return outputImage.GetImageData();

	}

}