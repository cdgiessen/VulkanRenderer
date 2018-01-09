#include "TerGenNodeGraph.h"

namespace NewNodeGraph {

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

	void INode::SetID(int ID) {
		this->ID = ID;
	}

	template<typename T>
	Link<T>::Link(LinkType type) : linkType(type) {}

	template<typename T>
	Link<T>::Link(LinkType type, T value) : linkType(type) , data(value) {}

	template<typename T>
	T Link<T>::GetValue(const int x, const int y, const int z) const {
		if (input)
			return input->GetValue(x, y, z, data);
		else
			return data;
	}

	template<typename T>
	T Link<T>::GetData() const {
		return data;
	}

	template<typename T>
	void Link<T>::SetValue(T val) {
		data = val;
	}

	template<typename T>
	const LinkType Link<T>::GetLinkType() const {
		return linkType;
	}

	template<typename T>
	bool Link<T>::SetInputNode(std::shared_ptr<INode> node) {
		if (node->GetNodeType() == linkType) {
			input = node;
			return true;
		}
		return false;
	}

	template<typename T>
	bool Link<T>::ResetInputNode() {
		input.reset();
		return true;
	}

	template<typename T>
	std::string Link<T>::GetJsonStringRep() const
	{

	}



	INode::~INode() {

	}

	template<typename T>
	Node<T>::Node(LinkType outputLinkType) : outputType(outputLinkType) {

	}

	template<typename T>
	Node<T>::~Node() {}

	template<typename T>
	LinkType Node<T>::GetNodeType() const {
		return outputType;
	}

	template<typename T>
	void Node<T>::SaveToJson(nlohmann::json & json)
	{
	}

	//template<typename T> T			Node<T>::GetValue(const int x, const int y, const int z, T dummy)		const { return dummy; }

	template<typename T> float		Node<T>::GetValue(const int x, const int y, const int z, float dummy)		const { return -1.1f; }
	template<typename T> int		Node<T>::GetValue(const int x, const int y, const int z, int dummy)			const { return -2; }
	template<typename T> double		Node<T>::GetValue(const int x, const int y, const int z, double dummy)		const { return -3.1; }
	template<typename T> glm::vec2	Node<T>::GetValue(const int x, const int y, const int z, glm::vec2 dummy)	const { return glm::vec2(0, -1); }
	template<typename T> glm::vec3	Node<T>::GetValue(const int x, const int y, const int z, glm::vec3 dummy)	const { return glm::vec3(0, -1, -2); }
	template<typename T> glm::vec4	Node<T>::GetValue(const int x, const int y, const int z, glm::vec4 dummy)	const { return glm::vec4(0, -1, -2, -3); }

	template<typename T> void Node<T>::SetValue(const int index, float value)	{};
	template<typename T> void Node<T>::SetValue(const int index, int value)		{};
	template<typename T> void Node<T>::SetValue(const int index, double value)	{};
	template<typename T> void Node<T>::SetValue(const int index, glm::vec2 value){};
	template<typename T> void Node<T>::SetValue(const int index, glm::vec3 value){};
	template<typename T> void Node<T>::SetValue(const int index, glm::vec4 value){};

	OutputNode::OutputNode() : Node<float>(LinkType::Float), input_output(LinkType::Float, 0) { }
	OutputNode::~OutputNode() { /*Log::Debug << "Output Node Deleted" << "\n"; */ }

	float OutputNode::GetValue(const int x, const int y, const int z, float dummy) const {
		return input_output.GetValue(x, y, z);
	}

	bool OutputNode::SetInputLink(int index, std::shared_ptr<INode> node)  {
		if (index == 0) 
			if (input_output.GetLinkType() == node->GetNodeType())
				return input_output.SetInputNode(node);
		return false;
	}

	bool OutputNode::ResetInputLink(int index) {
		if (index == 0)
			return input_output.ResetInputNode();
	}

	void OutputNode::SetValue(const int index, float value) {
		if(index == 0)
			input_output.SetValue(value);
	}

	void OutputNode::SaveToJson(nlohmann::json & json)
	{
		nlohmann::json j;

		j[std::to_string(ID)] = { {} };
	}

	ConstantFloatNode::ConstantFloatNode(float value) : Node<float>(LinkType::Float), value(LinkType::Float, value) {};
	ConstantFloatNode::~ConstantFloatNode() { /*Log::Debug << "Constant float Deleted" << "\n"; */ }

	float ConstantFloatNode::GetValue(const int x, const int y, const int z, float dummy) const
	{
		return value.GetValue(x,y,z);
	}
	void ConstantFloatNode::SetValue(const float value) 
	{
		this->value.SetValue(value);
	}

	bool ConstantFloatNode::SetInputLink(int index, std::shared_ptr<INode> node) {
		return value.SetInputNode(node);
	}

	bool ConstantFloatNode::ResetInputLink(int index) {
		return value.ResetInputNode();
	}

	void ConstantFloatNode::SetValue(const int index, float value) {
		this->value.SetValue(value);
	}

	void ConstantFloatNode::SaveToJson(nlohmann::json & json)
	{
		// TODO : fill out json
	}

	ConstantIntNode::ConstantIntNode(int value) : Node<int>(LinkType::Float), value(LinkType::Int, value) {};
	ConstantIntNode::~ConstantIntNode() { /*Log::Debug << "Coanstant int Deleted" << "\n"; */}

	int ConstantIntNode::GetValue(const int x, const int y, const int z, int dummy) const
	{
		return value.GetValue(x, y, z);
	}
	void ConstantIntNode::SetValue(const int value)
	{
		this->value.SetValue(value);
	}

	bool ConstantIntNode::SetInputLink(int index, std::shared_ptr<INode> node) {
		return value.SetInputNode(node);
	}

	void ConstantIntNode::SetValue(const int index, int value) {
		this->value.SetValue(value);
	}

	bool ConstantIntNode::ResetInputLink(int index) {
		return value.ResetInputNode();
	}

	void ConstantIntNode::SaveToJson(nlohmann::json & json)
	{
		// TODO : fill out json
	}

	MathNode::MathNode() : Node<float>(LinkType::Float), input_a(LinkType::Float), input_b(LinkType::Float) {}
	MathNode::~MathNode() { }
	
	float MathNode::GetValue(const int x, const int y, const int z, float dummy) const
	{
		return 0.0f;
	}

	bool MathNode::SetInputLink(int index, std::shared_ptr<INode> node)
	{
		if (index == 0) 
			return input_a.SetInputNode(node);
		else if (index == 1) 
			return input_b.SetInputNode(node);
		return false;
	}

	bool MathNode::ResetInputLink(int index)
	{
		if (index == 0)
			return input_a.ResetInputNode();
		else if (index == 1)
			return input_b.ResetInputNode();
		return false;
	}

	void MathNode::SetValue(const int index, float value) {
		if(index == 0)
			input_a.SetValue(value);
		else if(index == 1)
			input_b.SetValue(value);
	}


	void MathNode::SaveToJson(nlohmann::json & json)
	{
		// TODO : fill out json
	}
	
	float AdditionNode::GetValue(const int x, const int y, const int z, float dummy)const 	{ return input_a.GetValue(x, y, z) + input_b.GetValue(x, y, z); }
	float SubtractNode::GetValue(const int x, const int y, const int z, float dummy)const 	{ return input_a.GetValue(x, y, z) - input_b.GetValue(x, y, z); }
	float MultiplyNode::GetValue(const int x, const int y, const int z, float dummy)const 	{ return input_a.GetValue(x, y, z) * input_b.GetValue(x, y, z); }
	float DivideNode::GetValue(const int x, const int y, const int z, float dummy)	const 	{ return input_a.GetValue(x, y, z) / input_b.GetValue(x, y, z); }
	float PowerNode::GetValue(const int x, const int y, const int z, float dummy)	const 	{ return pow(input_a.GetValue(x, y, z),  input_b.GetValue(x, y, z)); }


	SelectorNode::SelectorNode() : Node<float>(LinkType::Float), input_a(LinkType::Float), input_b(LinkType::Float), input_blendAmount(LinkType::Float), input_cutoff(LinkType::Float)  {}


	SelectorNode::~SelectorNode() {}
	
	float SelectorNode::GetValue(const int x, const int y, const int z, float dummy) const {
		//alpha * black + (1 - alpha) * red
		float alpha = input_cutoff.GetValue(x, y, z);
		float a = input_a.GetValue(x, y, z);
		float b = input_b.GetValue(x, y, z);

		return alpha * a + (1 - alpha) + b;
	}

	bool SelectorNode::SetInputLink(int index, std::shared_ptr<INode> node) {
		if (index == 0)
			return input_a.SetInputNode(node);
		else if (index == 1)
			return input_b.SetInputNode(node);
		else if (index == 2)
			return input_cutoff.SetInputNode(node);
		else if (index == 3)
			return input_blendAmount.SetInputNode(node);
		return false;
	}
	
	bool SelectorNode::ResetInputLink(int index) {
		if (index == 0)
			return input_a.ResetInputNode();
		else if (index == 1)
			return input_b.ResetInputNode();
		else if (index == 2)
			return input_cutoff.ResetInputNode();
		else if (index == 3)
			return input_blendAmount.ResetInputNode();
		return false;
	}

	void SelectorNode::SetValue(const int index, float value) {
		if (index == 0)
			input_a.SetValue(value);
		else if (index == 1)
			input_b.SetValue(value);
		else if (index == 2)
			input_cutoff.SetValue(value);
		else if (index == 3)
			input_blendAmount.SetValue(value);
	}

	void SelectorNode::SaveToJson(nlohmann::json & json)
	{
		// TODO : fill out json
	}


	NoiseSourceNode::NoiseSourceNode()
		: Node<float>(LinkType::Float), input_frequency(LinkType::Float, 0.1f)
		, input_persistance(LinkType::Float, 0.5f), input_octaveCount(LinkType::Int, 3) {	
	};

	NoiseSourceNode::~NoiseSourceNode() {
		//Log::Debug << "Noise Source Deleted" << "\n";
		CleanNoiseSet();
	}

	float NoiseSourceNode::GetValue(const int x, const int y, const int z, float dummy) const {
		int realX = x;//*/ (int)(x * (float)(noiseDimention));
		int realY = y;//*/ (int)(y * (float)(noiseDimention));
		int realZ = z;//*/ (int)(z * (float)(noiseDimention));
		
		if (realX >= 0 && realX < noiseDimention && realY >= 0 && realY < noiseDimention && realZ >= 0 && realZ < noiseDimention) {
			float val = noiseSet.BoundedLookUp(realX, realZ);// [(realX * noiseDimention + realZ)];
			return val;
			//Log::Debug << val << "\n";
		}
		return -1.1f; //not in bounds. shouldn't happen but who knows (fortunately -1 is a very valid value, but its easy to tell if things went awry if everythign is -1
	}

	bool NoiseSourceNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		noiseDimention = numCells; 
		noiseSet.SetWidth(numCells);

		noiseSet.SetImageData(myNoise->GetEmptySet(numCells * numCells));

		return true;
	}

	bool NoiseSourceNode::CleanNoiseSet() {
		myNoise->FreeNoiseSet(noiseSet.GetImageData());
		noiseSet.SetImageData(nullptr);
		return true;
	}

	bool NoiseSourceNode::SetInputLink(int index, std::shared_ptr<INode> node) {

		if (index == 0 && input_octaveCount.GetLinkType() == node->GetNodeType())
			return input_octaveCount.SetInputNode(node);
		else if(index == 1 && input_persistance.GetLinkType() == node->GetNodeType())
			return input_persistance.SetInputNode(node);
		else if(index == 2 && input_frequency.GetLinkType() == node->GetNodeType())
			return input_frequency.SetInputNode(node);

		return false;
	}

	bool NoiseSourceNode::ResetInputLink(int index)	{
		if (index == 0)
			return input_octaveCount.ResetInputNode();
		else if (index == 1)
			return input_persistance.ResetInputNode();
		else if (index == 2)
			return input_frequency.ResetInputNode();

		return false;
	}

	void NoiseSourceNode::SetValue(const int index, int value) {
		if (index == 0)
			input_octaveCount.SetValue(value);
	}

	void NoiseSourceNode::SetValue(const int index, float value) {
		if (index == 1)
			input_persistance.SetValue(value);
		else if (index == 2)
			input_frequency.SetValue(value);
	}


	void NoiseSourceNode::SaveToJson(nlohmann::json & json)
	{
		// TODO : fill out json
	}

	bool ValueFractalNoiseNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		CleanNoiseSet();
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		myNoise->SetFractalGain(input_persistance.GetData());
		noiseDimention = numCells;
		noiseSet.SetWidth(numCells);
		noiseSet.SetImageData(myNoise->GetValueFractalSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier));
		return true;
	}

	bool SimplexFractalNoiseNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		CleanNoiseSet();
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		myNoise->SetFractalGain(input_persistance.GetData());
		noiseDimention = numCells;
		noiseSet.SetWidth(numCells);
		noiseSet.SetImageData(myNoise->GetSimplexFractalSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier));
		return true;
	}

	bool PerlinFractalNoiseNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		CleanNoiseSet();
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		myNoise->SetFractalGain(input_persistance.GetData());
		noiseDimention = numCells;
		noiseSet.SetWidth(numCells);
		noiseSet.SetImageData(myNoise->GetPerlinFractalSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier));
		return true;
	}

	bool CellularNoiseNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		CleanNoiseSet();
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		myNoise->SetFractalGain(input_persistance.GetData());
		noiseDimention = numCells;
		noiseSet.SetWidth(numCells);
		noiseSet.SetImageData(myNoise->GetCellularSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier));
		return true;
	}

	bool CubicFractalNoiseNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		CleanNoiseSet();
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		myNoise->SetFractalGain(input_persistance.GetData());
		noiseDimention = numCells;
		noiseSet.SetWidth(numCells);
		noiseSet.SetImageData(myNoise->GetCubicFractalSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier));
		return true;
	}

	bool WhiteNoiseNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		CleanNoiseSet();
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		myNoise->SetFractalGain(input_persistance.GetData());
		noiseDimention = numCells;
		noiseSet.SetWidth(numCells);
		noiseSet.SetImageData(myNoise->GetWhiteNoiseSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier));
		return true;
	}

	bool VoironiFractalNoiseNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		CleanNoiseSet();
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetData());
		myNoise->SetFractalOctaves(input_octaveCount.GetData());
		myNoise->SetFractalGain(input_persistance.GetData());
		noiseDimention = numCells;
		noiseSet.SetWidth(numCells);
		myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::CellValue);
		noiseSet.SetImageData(myNoise->GetCellularSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier));
		return true;
	}


	 

	TerGenGraphUser::TerGenGraphUser(const TerGenNodeGraph& sourceGraph, int seed, int numCells, glm::i32vec2 pos, float scale)
		: sourceGraph(sourceGraph), seed(seed), cellsWide(numCells), pos(pos), scale(scale), outputImage(numCells)
	{           
	}

	float TerGenGraphUser::SampleHeight(const float x, const float y, const float z) {
		return BilinearImageSample2D(outputImage, x, z);
	/*
		int cellScale = cellsWide - 1;

		float xScaled = x*(float)cellScale;
		float yScaled = y*(float)cellScale;
		float zScaled = z*(float)cellScale;
		
		int realX = (int)glm::clamp(xScaled, 0.0f, (float)cellScale);
		int realY = (int)glm::clamp(yScaled, 0.0f, (float)cellScale);
		int realZ = (int)glm::clamp(zScaled, 0.0f, (float)cellScale);

		int realXPlus1 = (int)glm::clamp(xScaled + 1, 0.0f, (float)cellScale); //make sure its not greater than the image size
		int realYPlus1 = (int)glm::clamp(yScaled + 1, 0.0f, (float)cellScale);
		int realZPlus1 = (int)glm::clamp(zScaled + 1, 0.0f, (float)cellScale);

		float UL = outputImage[realX * cellsWide + realZ];
		float UR = outputImage[realX * cellsWide + realZPlus1];
		float DL = outputImage[realXPlus1 * cellsWide + realZ];
		float DR = outputImage[realXPlus1 * cellsWide + realZPlus1];
		
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
	*/
	}

	TerGenNodeGraph::TerGenNodeGraph()
	{
		outputNode = std::make_shared<OutputNode>();
		AddNode(outputNode);
	}


	TerGenNodeGraph::~TerGenNodeGraph()
	{

	}

	int TerGenNodeGraph::GetNextID() {
		return IDCounter++;
	}

	bool TerGenNodeGraph::AddNode(std::shared_ptr<INode> node)
	{
		node->SetID(GetNextID());
		nodes.push_back(node);
		return true;
	}

	bool TerGenNodeGraph::AddNoiseSourceNode(std::shared_ptr<NoiseSourceNode> node)
	{
		node->SetID(GetNextID());
		nodes.push_back(node);
		noiseSources.push_back(node);
		return true;
	}

	bool TerGenNodeGraph::DeleteNode(std::shared_ptr<INode> node) {
		auto found = std::find(nodes.begin(), nodes.end(), node);
		if (found != nodes.end()) {


			nodes.erase(found);
		}
		return true;
	}

	const std::shared_ptr<OutputNode> TerGenNodeGraph::GetOutputNode() const {
		return outputNode;
	}

	const std::vector<std::shared_ptr<NoiseSourceNode>>& TerGenNodeGraph::GetNoiseSources() const {
		return noiseSources;
	};

	//Creates the image to sample from
	void TerGenGraphUser::BuildOutputImage(glm::i32vec2 pos, float scale) {
		this->pos = pos;
		this->scale = scale;
		
		for (auto noise : sourceGraph.GetNoiseSources()) {
			noise->GenerateNoiseSet(seed, cellsWide, glm::ivec2(pos.x * (cellsWide)/scale, pos.y * (cellsWide) / scale), scale / (cellsWide));
		}

		for (int i = 0; i < cellsWide; i++)	{
			for (int j = 0; j < cellsWide; j++)	{
				outputImage.SetPixelValue(i, j, sourceGraph.GetOutputNode()->GetValue(i, 0, j, 0.0f));
			}
		}
	}

	float* TerGenGraphUser::GetOutputGreyscaleImage() {
		return outputImage.GetImageData();
	}

	//Creates the graph (for now the graph is hardcoded here)
	//Should update the noise modules to use latest settings and compute their values
	void TerGenNodeGraph::BuildNoiseGraph() {

		auto nSource1 = std::make_shared<SimplexFractalNoiseNode>();
		auto nSource2 = std::make_shared<PerlinFractalNoiseNode>();
		auto addNode = std::make_shared<AdditionNode>();

		AddNoiseSourceNode(nSource1);
		AddNoiseSourceNode(nSource2);

		addNode->SetInputLink(0, nSource1);
		addNode->SetInputLink(1, nSource2);

		outputNode->SetInputLink(0, nSource1);
	}

	nlohmann::json TerGenNodeGraph::SaveGraphToJson()
	{
		nlohmann::json outjson;



		return outjson;
	}

	void TerGenNodeGraph::LoadGraphFromJson(nlohmann::json inputjson)
	{

	}

}