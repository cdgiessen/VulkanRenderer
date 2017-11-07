#include "TerGenNodeGraph.h"


namespace NewNodeGraph {

	INode::~INode() {
		//std::cout << "Inode Deleted" << std::endl;
	}

	template<typename T>
	Link<T>::Link(LinkType type) : linkType(type) {

	}

	template<typename T>
	Link<T>::Link(LinkType type, T value) : linkType(type) , data(value) {

	}

	template<typename T>
	T Link<T>::GetValue(const int x, const int y, const int z) {
		if (input)
			return input->GetValue(x, y, z, data);
		else
			return data;
	}

	template<typename T>
	T Link<T>::GetValue() {
		return GetValue(0, 0, 0);
	}

	template<typename T>
	void Link<T>::SetValue(T val) {
		data = val;
	}

	template<typename T>
	LinkType Link<T>::GetLinkType() {
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
	Node<T>::Node(LinkType outputLinkType) : outputType(outputLinkType) {

	}

	template<typename T>
	Node<T>::~Node() {
		//std::cout << "Node Deleted" << std::endl;
	}

	template<typename T>
	LinkType Node<T>::GetNodeType() {
		return outputType;
	}

	template<typename T> float		Node<T>::GetValue(const int x, const int y, const int z, float dummy)		{ return -1.1f; }
	template<typename T> int		Node<T>::GetValue(const int x, const int y, const int z, int dummy)			{ return -2; }
	template<typename T> double		Node<T>::GetValue(const int x, const int y, const int z, double dummy)		{ return -3.1; }
	template<typename T> glm::vec2	Node<T>::GetValue(const int x, const int y, const int z, glm::vec2 dummy)	{ return glm::vec2(0, -1); }
	template<typename T> glm::vec3	Node<T>::GetValue(const int x, const int y, const int z, glm::vec3 dummy)	{ return glm::vec3(0, -1, -2); }
	template<typename T> glm::vec4	Node<T>::GetValue(const int x, const int y, const int z, glm::vec4 dummy)	{ return glm::vec4(0, -1, -2, -3); }

	OutputNode::OutputNode() : Node<float>(LinkType::Float), input_output(LinkType::Float, 0) { }
	OutputNode::~OutputNode() { /*std::cout << "Output Node Deleted" << std::endl; */ }

	float OutputNode::GetValue(const int x, const int y, const int z, float dummy) {
		return input_output.GetValue(x, y, z);
	}

	bool OutputNode::SetInputLink(int index, std::shared_ptr<INode> node) {
		if (index == 0) {
			if (input_output.GetLinkType() == node->GetNodeType()) {
				input_output.SetInputNode(node);
				return true;
			}
		}
		return false;
	}

	ConstantFloatNode::ConstantFloatNode() : Node<float>(LinkType::Float), value(LinkType::Float) {};
	ConstantFloatNode::ConstantFloatNode(float value) : Node<float>(LinkType::Float), value(LinkType::Float, value) {};
	ConstantFloatNode::~ConstantFloatNode() { /*std::cout << "Constant float Deleted" << std::endl; */ }

	float ConstantFloatNode::GetValue(const int x, const int y, const int z, float dummy)
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

	ConstantIntNode::ConstantIntNode() : Node<int>(LinkType::Float), value(LinkType::Int) {};
	ConstantIntNode::ConstantIntNode(int value) : Node<int>(LinkType::Int), value(LinkType::Int, value) {};
	ConstantIntNode::~ConstantIntNode() { /*std::cout << "Coanstant int Deleted" << std::endl; */}

	int ConstantIntNode::GetValue(const int x, const int y, const int z, int dummy)
	{
		return value.GetValue(x, y, z);
	}
	void ConstantIntNode::SetValue(const int value)
	{
		this->SetValue(value);
	}

	bool ConstantIntNode::SetInputLink(int index, std::shared_ptr<INode> node) {
		return value.SetInputNode(node);
	}

	SelectorNode::~SelectorNode() {}
	
	float SelectorNode::GetValue(const int x, const int y, const int z, float dummy) {
		//alpha * black + (1 - alpha) * red
		float alpha = input_cutoff.GetValue(x, y, z);
		float a = input_a.GetValue(x, y, z);
		float b = input_b.GetValue(x, y, z);

		return alpha * a + (1 - alpha) + b;
	}


	NoiseSourceNode::NoiseSourceNode() : Node<float>(LinkType::Float), input_frequency(LinkType::Float,0.1f), input_persistance(LinkType::Float, 0.5f), input_octaveCount(LinkType::Int, 3) {	};
	NoiseSourceNode::~NoiseSourceNode() {
		//std::cout << "Noise Source Deleted" << std::endl;
		CleanNoiseSet();
	}

	float NoiseSourceNode::GetValue(const int x, const int y, const int z, float dummy) {

		//if (x >= 0 && x < noiseDimention && y >= 0 && y < noiseDimention && z >= 0 && z < noiseDimention) {
		float val = noiseSet[(x * noiseDimention + z)];
			return val;
		//}
		return -1.1f; //not in bounds. shouldn't happen but who knows (fortunately -1 is a very valid value, but its easy to tell if things went awry if everythign is -1
	}

	bool NoiseSourceNode::GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) {
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetValue());
		myNoise->SetFractalOctaves(input_octaveCount.GetValue());
		noiseDimention = numCells; 
		
		noiseSet = myNoise->GetValueSet(pos.x, 0, pos.y, numCells, 1, numCells, scaleModifier);
		return true;
	}

	bool NoiseSourceNode::CleanNoiseSet() {
		myNoise->FreeNoiseSet(noiseSet);
		return true;
	}

	bool NoiseSourceNode::SetInputLink(int index, std::shared_ptr<INode> node) {
		switch (index)
		{
		case 0:
			if (input_frequency.GetLinkType() == node->GetNodeType()) {
				input_frequency.SetInputNode(node);
				return true;
			}
			break;
		case 1:
			if (input_persistance.GetLinkType() == node->GetNodeType()) {
				input_persistance.SetInputNode(node);
				return true;
			}
			break;
		case 2:
			if (input_octaveCount.GetLinkType() == node->GetNodeType()) {
				input_octaveCount.SetInputNode(node);
				return true;
			}
			break;
		default:
			return false;
		}
	}

	TerGenNodeGraph::TerGenNodeGraph(int seed, int numCells, glm::i32vec2 pos, float scaleModifier) : seed(seed), cellsWide(numCells), pos(pos), scale(scaleModifier)
	{
		AddNode(std::make_shared<OutputNode>(outputNode));
	}


	TerGenNodeGraph::~TerGenNodeGraph()
	{

	}

	bool TerGenNodeGraph::AddNode(std::shared_ptr<INode> node)
	{
		nodes.push_back(node);
		return true;
	}

	bool TerGenNodeGraph::AddNoiseSourceNode(std::shared_ptr<NoiseSourceNode> node)
	{
		nodes.push_back(node);
		noiseSources.push_back(node);
		return true;
	}

	//float TerGenNodeGraph::SampleHeight(const double x, const double y, const double z) {
	//	//if (x >= pos.x && x < pos.x + cellsWide && y >= pos.y && y < pos.y + cellsWide && z >= pos.z && z < pos.z + cellsWide)
	//		return outputNode->GetValue(x, y, z, 1.0f);
	//	return -0.1;
	//}

	float TerGenNodeGraph::SampleHeight(const int x, const int y, const int z) {
		if (x >= 0 && x < cellsWide && y == 0 && z >= 0 && z < cellsWide)
			return outputImage[x * cellsWide + z];
		return -1.0;
	}

	//Preps graph to prepare for reading.
	//Should update the noise modules to use latest settings and compute their values
	void TerGenNodeGraph::BuildNoiseGraph() {

		auto noiseSource = std::make_shared<NoiseSourceNode>();
		AddNoiseSourceNode(noiseSource);

		outputNode.SetInputLink(0, noiseSource);

		
	}

	void TerGenNodeGraph::BuildOutputImage(glm::i32vec2 pos, float scale) {
		this->pos = pos;
		this->scale = scale;
		for (auto item : noiseSources)
		{
			item->GenerateNoiseSet(seed, cellsWide, pos, scale);
		}

		outputImage.resize(cellsWide * cellsWide);
		for (int i = 0; i < cellsWide; i++)
		{
			for (int j = 0; j < cellsWide; j++)
			{
				outputImage[i * cellsWide + j] = outputNode.GetValue(i, 0, j, 0.0f);
			}
		}
	}

	std::vector<float>& TerGenNodeGraph::GetOutputGreyscaleImage() {
		return outputImage;
	}
}