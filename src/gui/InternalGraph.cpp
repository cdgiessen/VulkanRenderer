#include "InternalGraph.h"

#include "../core/Logger.h"


namespace InternalGraph {

	template<typename T>
	static float BilinearImageSample2D(const NoiseImage2D<T>& noiseImage, const float x, const float z) {
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
				+ DL * (xScaled - realX)		* (realZPlus1 - zScaled)
				+ UR * (realXPlus1 - xScaled)	* (zScaled - realZ)
				+ DR * (xScaled - realX)		* (zScaled - realZ)
				)
				/ ((realXPlus1 - realX) * (realZPlus1 - realZ));
		}
	}

	//No allocation constructor, meant for images that are externally created (FastNoiseSIMD)
	template<typename T>
	NoiseImage2D<T>::NoiseImage2D() : width(0), isExternallyAllocated(true) {}

	//Preallocates based on width, mainly meant for output images
	template<typename T>
	NoiseImage2D<T>::NoiseImage2D(int width) : width(width), isExternallyAllocated(false) {
		
		data = std::vector<T>(width*width);
		/*
		image = (float*)malloc(GetSizeBytes());
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < width; j++)
			{
				SetPixelValue(i, j, -1);
			}
		}*/
	}

	template<typename T>
	NoiseImage2D<T>::~NoiseImage2D() {
		if (!isExternallyAllocated && image != nullptr)
			free(image);
		//Log::Error << "Was this image already freed?\n";
	}

	template<typename T>
	T* NoiseImage2D<T>::GetImageData() {
		if (isExternallyAllocated)
			return image;
		else
			return data.data();
	}

	template<typename T>
	void NoiseImage2D<T>::SetImageData(int width, T* data) {
		this->width = width;
		image = data;
	}

	template<typename T>
	const int NoiseImage2D<T>::GetSize() const {
		return width * width;
	}

	template<typename T>
	const size_t NoiseImage2D<T>::GetSizeBytes() const {
		return GetSize() * 4;
	}

	template<typename T>
	const int NoiseImage2D<T>::GetImageWidth() const {
		return width;
	}

	//Changes the width then frees and allocates new image 
	template<typename T>
	void NoiseImage2D<T>::SetWidth(int width) {
		this->width = width;
	}

	template<typename T>
	const T NoiseImage2D<T>::LookUp(int x, int z) const {
		if(isExternallyAllocated)
			return image[x * width + z];
		else 
			return data[x * width + z];
	}

	template<typename T>
	const T NoiseImage2D<T>::BoundedLookUp(int x, int z) const {
		if (x >= 0 && x < width && z >= 0 && z < width){
			if (isExternallyAllocated)
				return image[x * width + z];
			else
				return data[x * width + z];
		} else {
			throw new std::runtime_error("out of bounds");
		}
		return -1;
	}

	template<typename T>
	void NoiseImage2D<T>::SetPixelValue(int x, int z, T value) {
		
		
		if (x >= 0 && x < width && z >= 0 && z < width)
			data[x * width + z] = value;		
		//	image[x * width + z] = value;
		else
			throw new std::runtime_error("out of bounds");
	}

	InputLink::InputLink() : value(-1.0f) {}
	InputLink::InputLink(float in): value(in) {}
	InputLink::InputLink(int in) : value(in) {}
	InputLink::InputLink(glm::vec2 in): value(in){}
	InputLink::InputLink(glm::vec3 in): value(in){}
	InputLink::InputLink(glm::vec4 in): value(in){}


	void InputLink::SetInputNode(NodeID id) {
		handle.id = id;
		hasInputNode = true;
	}

	void InputLink::ResetInputNode() {
		handle.id = -1;
		hasInputNode = false;
		handle.handle = nullptr;
	}

	NodeID InputLink::GetInputNode() {
		return handle.id;
	}

	bool InputLink::HasInputNode() {
		return hasInputNode;
	}

	void InputLink::SetInputNodePointer(Node* node) {
		hasInputNode = true;
		handle.handle = node;
	}

	void InputLink::ResetInputNodePointer() {
		hasInputNode = false;
		handle.handle = nullptr;
	}

	void InputLink::SetDataValue(LinkTypeVariants data) {
		value = data;
	}

	LinkTypeVariants InputLink::GetValue() {
		return value;
	}

	LinkTypeVariants InputLink::GetValue(const int x, const int z) {
	
		if (hasInputNode)
			return handle.handle->GetValue(x, z);
		else {
			
			return value;

		}
	}

	void AddNodeInputLinks(std::vector<InputLink>& links, std::vector<LinkType> types) {
		for (auto t : types) {
			switch (t) {
			case LinkType::Float:
				links.push_back(0.0f);
				break;
			case LinkType::Int:
				links.push_back(0);
				break;
			case LinkType::Vec2:
				links.push_back(glm::vec2(0));
				break;
			case LinkType::Vec3:
				links.push_back(glm::vec3(0));
				break;
			case LinkType::Vec4:
				links.push_back(glm::vec4(0));
				break;
			default:
				break;
			}
		}

	}

	Node::Node(NodeType in_type) : nodeType(in_type){
		outputType = LinkType::Float;

		switch (nodeType)
		{
		case InternalGraph::NodeType::Output:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Float, LinkType::Vec4 });
			break;

		case InternalGraph::NodeType::Addition:
		case InternalGraph::NodeType::Subtraction:
		case InternalGraph::NodeType::Multiplication:
		case InternalGraph::NodeType::Division:
		case InternalGraph::NodeType::Power:
		case InternalGraph::NodeType::Max:
		case InternalGraph::NodeType::Min:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Float, LinkType::Float });

			//inputLinks.push_back(InputLink(0.0f));
			//inputLinks.push_back(InputLink(0.0f));
			break;

		case InternalGraph::NodeType::Blend:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Float, LinkType::Float, LinkType::Float });
			break;
		case InternalGraph::NodeType::Clamp:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Float, LinkType::Float, LinkType::Float }); 
			break;

		case InternalGraph::NodeType::ConstantInt:
			outputType = LinkType::Int;
			inputLinks.push_back(InputLink(0));
			break;
		case InternalGraph::NodeType::ConstantFloat:
			inputLinks.push_back(InputLink(0.0f));
			break;

		case InternalGraph::NodeType::ValueFractalNoise:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Int, LinkType::Float, LinkType::Float });
			isNoiseNode = true;
			myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
			break;

		case InternalGraph::NodeType::SimplexFractalNoise:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Int, LinkType::Float, LinkType::Float });
			isNoiseNode = true;
			myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
			break;

		case InternalGraph::NodeType::PerlinFractalNoise:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Int, LinkType::Float, LinkType::Float });
			isNoiseNode = true;
			myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
			break;

		case InternalGraph::NodeType::WhiteNoise:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Int, LinkType::Float, LinkType::Float });
			isNoiseNode = true;
			myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
			break;

		case InternalGraph::NodeType::CellularNoise:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Int, LinkType::Float, LinkType::Float });
			isNoiseNode = true;
			myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
			break;

		case InternalGraph::NodeType::CubicFractalNoise:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Int, LinkType::Float, LinkType::Float });
			isNoiseNode = true;
			myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
			break;

		case InternalGraph::NodeType::VoroniFractalNoise:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Int, LinkType::Float, LinkType::Float });
			isNoiseNode = true;
			myNoise = FastNoiseSIMD::NewFastNoiseSIMD();

			break;

		case NodeType::ColorCreator:
			outputType = LinkType::Vec4;
			AddNodeInputLinks(inputLinks,
				{ LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float });
			break;

		case NodeType::MonoGradient:
			AddNodeInputLinks(inputLinks,
				{ LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float, LinkType::Float });

			break;
		
		default:
			break;
		}


	}

	void Node::SetLinkValue(const int index, const LinkTypeVariants data) {
		inputLinks.at(index).SetDataValue(data);
	}

	LinkTypeVariants Node::GetHeightMapValue(const int x, const int z) {
		return inputLinks.at(0).GetValue(x,z);
	}

	LinkTypeVariants Node::GetSplatMapValue(const int x, const int z) {
		return inputLinks.at(1).GetValue(x, z);
	}

	LinkTypeVariants Node::GetValue(const int x, const int z) {
		LinkTypeVariants retVal;
		//LinkTypeVariants reA, reB;
		float a, b, c, d, alpha;
		float value, lower, upper, smooth;

		auto val = (inputLinks.at(0));

		switch (nodeType)
		{
		case InternalGraph::NodeType::None:
			break;

		case InternalGraph::NodeType::Addition:

			switch (outputType)
			{
			case InternalGraph::LinkType::None:
				break;
			case InternalGraph::LinkType::Float:
				return std::get<float>(inputLinks.at(0).GetValue(x, z)) + std::get<float>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Int:
				return std::get<int>(inputLinks.at(0).GetValue(x, z)) + std::get<int>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec2:
				return std::get<glm::vec2>(inputLinks.at(0).GetValue(x, z)) + std::get<glm::vec2>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec3:
				return std::get<glm::vec3>(inputLinks.at(0).GetValue(x, z)) + std::get<glm::vec3>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec4:
				return std::get<glm::vec4>(inputLinks.at(0).GetValue(x, z)) + std::get<glm::vec4>(inputLinks.at(1).GetValue(x, z));
				break;
			default:
				break;
			}
			break;

		case InternalGraph::NodeType::Subtraction:
			switch (outputType)
			{
			case InternalGraph::LinkType::None:
				break;
			case InternalGraph::LinkType::Float:
				return std::get<float>(inputLinks.at(0).GetValue(x, z)) - std::get<float>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Int:
				return std::get<int>(inputLinks.at(0).GetValue(x, z)) - std::get<int>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec2:
				return std::get<glm::vec2>(inputLinks.at(0).GetValue(x, z)) - std::get<glm::vec2>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec3:
				return std::get<glm::vec3>(inputLinks.at(0).GetValue(x, z)) - std::get<glm::vec3>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec4:
				return std::get<glm::vec4>(inputLinks.at(0).GetValue(x, z)) - std::get<glm::vec4>(inputLinks.at(1).GetValue(x, z));
				break;
			default:
				break;
			}
			break;
		case InternalGraph::NodeType::Multiplication:
			switch (outputType)
			{
			case InternalGraph::LinkType::None:
				break;
			case InternalGraph::LinkType::Float:
				return std::get<float>(inputLinks.at(0).GetValue(x, z)) * std::get<float>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Int:
				return std::get<int>(inputLinks.at(0).GetValue(x, z)) * std::get<int>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec2:
				return std::get<glm::vec2>(inputLinks.at(0).GetValue(x, z)) * std::get<glm::vec2>(inputLinks.at(1).GetValue(x, z));
				break;														
			case InternalGraph::LinkType::Vec3:								
				return std::get<glm::vec3>(inputLinks.at(0).GetValue(x, z)) * std::get<glm::vec3>(inputLinks.at(1).GetValue(x, z));
				break;														
			case InternalGraph::LinkType::Vec4:								
				return std::get<glm::vec4>(inputLinks.at(0).GetValue(x, z)) * std::get<glm::vec4>(inputLinks.at(1).GetValue(x, z));
				break;
			default:
				break;
			}
			break;
		case InternalGraph::NodeType::Division:
			switch (outputType)
			{
			case InternalGraph::LinkType::None:
				break;
			case InternalGraph::LinkType::Float:
				return std::get<float>(inputLinks.at(0).GetValue(x, z)) / std::get<float>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Int:
				return std::get<int>(inputLinks.at(0).GetValue(x, z)) / std::get<int>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec2:
				return std::get<glm::vec2>(inputLinks.at(0).GetValue(x, z)) / std::get<glm::vec2>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec3:
				return std::get<glm::vec3>(inputLinks.at(0).GetValue(x, z)) / std::get<glm::vec3>(inputLinks.at(1).GetValue(x, z));
				break;
			case InternalGraph::LinkType::Vec4:
				return std::get<glm::vec4>(inputLinks.at(0).GetValue(x, z)) / std::get<glm::vec4>(inputLinks.at(1).GetValue(x, z));
				break;
			default:
				break;
			}
			break;
		case InternalGraph::NodeType::Power:
			switch (outputType)
			{
			case InternalGraph::LinkType::None:
				break;
			case InternalGraph::LinkType::Float:
				return glm::pow(std::get<float>(inputLinks.at(0).GetValue(x, z)), std::get<float>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Int:
				return glm::pow((float)std::get<int>(inputLinks.at(0).GetValue(x, z)), (float)std::get<int>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec2:
				return glm::pow(std::get<glm::vec2>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec2>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec3:
				return glm::pow(std::get<glm::vec3>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec3>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec4:
				return glm::pow(std::get<glm::vec4>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec4>(inputLinks.at(1).GetValue(x, z)));
				break;
			default:
				break;
			}
			break;
		case InternalGraph::NodeType::Max:
			switch (outputType)
			{
			case InternalGraph::LinkType::None:
				break;
			case InternalGraph::LinkType::Float:
				return glm::max(std::get<float>(inputLinks.at(0).GetValue(x, z)), std::get<float>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Int:
				return glm::max(std::get<int>(inputLinks.at(0).GetValue(x, z)), std::get<int>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec2:
				return glm::max(std::get<glm::vec2>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec2>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec3:
				return glm::max(std::get<glm::vec3>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec3>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec4:
				return glm::max(std::get<glm::vec4>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec4>(inputLinks.at(1).GetValue(x, z)));
				break;
			default:
				break;
			}
			break;

		case InternalGraph::NodeType::Min:
			switch (outputType)
			{
			case InternalGraph::LinkType::None:
				break;
			case InternalGraph::LinkType::Float:
				return glm::min(std::get<float>(inputLinks.at(0).GetValue(x, z)), std::get<float>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Int:
				return glm::min(std::get<int>(inputLinks.at(0).GetValue(x, z)), std::get<int>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec2:
				return glm::min(std::get<glm::vec2>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec2>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec3:
				return glm::min(std::get<glm::vec3>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec3>(inputLinks.at(1).GetValue(x, z)));
				break;
			case InternalGraph::LinkType::Vec4:
				return glm::min(std::get<glm::vec4>(inputLinks.at(0).GetValue(x, z)), std::get<glm::vec4>(inputLinks.at(1).GetValue(x, z)));
				break;
			default:
				break;
			}
			break;
		case InternalGraph::NodeType::Blend:
			switch (outputType)
			{
			case InternalGraph::LinkType::Float:
				a = std::get<float>(inputLinks.at(0).GetValue(x, z));
				b = std::get<float>(inputLinks.at(1).GetValue(x, z));
				alpha = std::get<float>(inputLinks.at(2).GetValue(x, z));
				retVal = alpha * b + (1 - alpha) * a;
				return retVal;
			default:
				break;
			}
			break;
		case InternalGraph::NodeType::Clamp:
			switch (outputType)
			{
			case InternalGraph::LinkType::Float:
				value = std::get<float>(inputLinks.at(0).GetValue(x, z));
				lower = std::get<float>(inputLinks.at(1).GetValue(x, z));
				upper = std::get<float>(inputLinks.at(2).GetValue(x, z));
				retVal = glm::clamp(value, lower, upper);
				return retVal;
			default:
				break;
			}
			break;
		case InternalGraph::NodeType::ConstantInt:
			return inputLinks.at(0).GetValue(x, z);

			break;
		case InternalGraph::NodeType::ConstantFloat:
			return inputLinks.at(0).GetValue(x, z);

			break;

		case InternalGraph::NodeType::ValueFractalNoise:
		case InternalGraph::NodeType::SimplexFractalNoise:
		case InternalGraph::NodeType::PerlinFractalNoise:
		case InternalGraph::NodeType::WhiteNoise:
		case InternalGraph::NodeType::CellularNoise:
		case InternalGraph::NodeType::CubicFractalNoise:
		case InternalGraph::NodeType::VoroniFractalNoise:

			retVal = noiseImage.BoundedLookUp(x, z);
			return retVal;
			//Log::Debug << val << "\n";
			
			break;

		case NodeType::ColorCreator:
			
			a = std::get<float>(inputLinks.at(0).GetValue(x, z));
			b = std::get<float>(inputLinks.at(1).GetValue(x, z));
			c = std::get<float>(inputLinks.at(2).GetValue(x, z));
			d = std::get<float>(inputLinks.at(3).GetValue(x, z));
			
			retVal = glm::vec4(a, b, c, d);

			return retVal;
			break;

		case NodeType::MonoGradient:
			value = std::get<float>(inputLinks.at(0).GetValue(x, z));
			lower = std::get<float>(inputLinks.at(1).GetValue(x, z));
			upper = std::get<float>(inputLinks.at(2).GetValue(x, z));
			smooth = std::get<float>(inputLinks.at(3).GetValue(x, z));
			//value = glm::clamp(value, lower, upper);
			retVal = lower + value * (upper - lower);
			//retVal = ((upper - value) + (value - lower)) / (upper - lower);

			return retVal;

			break;

		default:
			break;
		}
	}

	NodeType Node::GetNodeType() {
		return nodeType;
	}

	LinkType Node::GetOutputType() {
		return outputType;
	}


	void Node::SetLinkInput(const int index, const NodeID id) {
		inputLinks.at(index).SetInputNode(id);
	}

	void Node::ResetLinkInput(const int index) {
		inputLinks.at(index).ResetInputNode();
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

	void Node::SetupInputLinks(NodeMap* map) {
		for (auto it = inputLinks.begin(); it != inputLinks.end(); it++) {
			if (it->HasInputNode()) {
				Node* ptr = &(map->at(it->GetInputNode()));
				it->SetInputNodePointer(ptr);
			}
		}
	}

	void Node::SetupNodeForComputation(NoiseSourceInfo info) {
		if (isNoiseNode) {
			noiseImage.SetWidth(info.cellsWide);

			myNoise->SetFractalOctaves(std::get<int>(inputLinks.at(0).GetValue()));
			myNoise->SetFrequency(std::get<float>(inputLinks.at(1).GetValue()));
			myNoise->SetFractalGain(std::get<float>(inputLinks.at(2).GetValue()));
			//myNoise->SetAxisScales(info.scale, info.scale, info.scale);
			switch (nodeType)
			{
			case InternalGraph::NodeType::ValueFractalNoise:
				noiseImage.SetImageData(info.cellsWide, myNoise->GetValueFractalSet(info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::SimplexFractalNoise:
				noiseImage.SetImageData(info.cellsWide, myNoise->GetSimplexFractalSet(info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::PerlinFractalNoise:
				noiseImage.SetImageData(info.cellsWide, myNoise->GetPerlinFractalSet(info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::WhiteNoise:
				noiseImage.SetImageData(info.cellsWide, myNoise->GetWhiteNoiseSet(info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::CellularNoise:
				noiseImage.SetImageData(info.cellsWide, myNoise->GetCellularSet(info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::CubicFractalNoise:
				noiseImage.SetImageData(info.cellsWide, myNoise->GetCubicFractalSet(info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			case InternalGraph::NodeType::VoroniFractalNoise:
				myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::CellValue);
				noiseImage.SetImageData(info.cellsWide, myNoise->GetCellularSet(info.pos.x, 0, info.pos.y, info.cellsWide, 1, info.cellsWide, info.scale));
				break;

			default:
				break;
			}


		}
	}

	void Node::CleanUp() {
		if(isNoiseNode)
			myNoise->FreeNoiseSet(noiseImage.GetImageData());
	}

	GraphPrototype::GraphPrototype() {
		//Node outputNode(NodeType::Output);
		//outputNodeID = AddNode(outputNode);

	}
	GraphPrototype::~GraphPrototype() {
	}

	void GraphPrototype::ResetGraph() {
		nodeMap.clear();
		nodeIDCounter = 0;
		outputNodeID = 0;
	}

	NodeID GraphPrototype::AddNode(Node node) {
		node.SetID(GetNextID());
		nodeMap[node.GetID()] = node;
		if (node.GetNodeType() == NodeType::Output) {
			outputNodeID = node.GetID();
		}
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

	void GraphPrototype::SetOutputNodeID(NodeID id) {
		outputNodeID = id;
	}

	NodeMap GraphPrototype::GetNodeMap() const {
		return nodeMap;
	}


	GraphUser::GraphUser(const GraphPrototype& graph, 
		int seed, int cellsWide, glm::i32vec2 pos, float scale):
	info(seed, cellsWide, scale, pos)
	{
		//glm::ivec2(pos.x * (cellsWide) / scale, pos.y * (cellsWide) / scale), scale / (cellsWide)

		//NoiseSourceInfo info = NoiseSourceInfo(seed, cellsWide, scale, pos);
		this->nodeMap = graph.GetNodeMap();

		for (auto it = nodeMap.begin(); it != nodeMap.end(); it++)
		{
			for (auto linkIt = it->second.inputLinks.begin(); linkIt != it->second.inputLinks.end(); linkIt++) {
				if (linkIt->HasInputNode()) {
					Node* n = &nodeMap.at(linkIt->GetInputNode());
					linkIt->SetInputNodePointer(n);
				}
			}
		}

		for (auto it = nodeMap.begin(); it != nodeMap.end(); it++) {
			it->second.SetupNodeForComputation(info);
		}

		outputNode = &nodeMap[graph.GetOutputNodeID()];
		
		outputHeightMap = NoiseImage2D<float>(cellsWide);
		for (int x = 0; x < cellsWide; x++)
		{
			for (int z = 0; z < cellsWide; z++)
			{
				float val = std::get<float>(outputNode->GetHeightMapValue(x, z));
				outputHeightMap.SetPixelValue(x, z, val);
			}
		}

		outputSplatmap = NoiseImage2D<RGBA_pixel>(cellsWide);

		for (int x = 0; x < cellsWide; x++)
		{
			for (int z = 0; z < cellsWide; z++)
			{
				glm::vec4 val = std::get<glm::vec4>(outputNode->GetSplatMapValue(x, z));
				RGBA_pixel pixel = RGBA_pixel((stbi_uc)(val.x * 255), (stbi_uc)(val.y * 255), (stbi_uc)(val.z * 255), (stbi_uc)(val.w * 255));
				
				outputSplatmap.SetPixelValue(z, x, pixel);
			}
		}
		

		for (auto node : nodeMap) {
			node.second.CleanUp();
		}
	}

	float GraphUser::SampleHeightMap(const float x, const float z) {
		return BilinearImageSample2D(outputHeightMap, x, z);
	}
	NoiseImage2D<float>& GraphUser::GetHeightMap() {
		return outputHeightMap;

	}

	NoiseImage2D<RGBA_pixel>& GraphUser::GetSplatMap() {
		return outputSplatmap;

	}

}