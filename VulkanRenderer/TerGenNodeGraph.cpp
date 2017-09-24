#include "TerGenNodeGraph.h"


namespace NewNodeGraph {

	template<typename T>
	Link<T>::Link(LinkType type) : linkType(type) {

	}

	template<typename T>
	T Link<T>::GetValue(double x, double y, double z) {
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
	LinkType Link<T>::GetLinkType() {
		return linkType;
	}

	template<typename T>
	bool Link<T>::SetInputNode(std::shared_ptr<INode> node) {
		if (node->GetNodeType() == linkType) {
			input = node;
			return true;
		}
		else {
			return false;
		}
	}



	template<typename T>
	Node<T>::Node(LinkType outputLinkType) : outputType(outputLinkType) {

	}

	template<typename T>
	LinkType Node<T>::GetNodeType() {
		return outputType;
	}

	template<typename T>
	float Node<T>::GetValue(const double x, const double y, const  double z, float dummy) {
		return -1;
	}

	template<typename T>
	int Node<T>::GetValue(const double x, const double y, const  double z, int dummy) {
		return -2;
	}

	template<typename T>
	double Node<T>::GetValue(const double x, const double y, const  double z, double dummy) {
		return -3;
	}


	OutputNode::OutputNode() : Node<float>(LinkType::Float), input_output(LinkType::Float) { };

	float OutputNode::GetValue(const double x, const double y, const  double z, float dummy) {
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

	ConstantFloatNode::ConstantFloatNode() : Node<float>(LinkType::Float) {};

	float ConstantFloatNode::GetValue(const double x, const double y, const  double z, float dummy) 
	{
		return constantValue;
	}
	void ConstantFloatNode::SetValue(const float value) 
	{
		constantValue = value;
	}
	bool ConstantFloatNode::SetInputLink(int index, std::shared_ptr<INode> node) { return false; }

	ConstantIntNode::ConstantIntNode() : Node<int>(LinkType::Int) {};

	int ConstantIntNode::GetValue(const double x, const double y, const  double z, int dummy)
	{
		return constantValue;
	}
	void  ConstantIntNode::SetValue(const int value)
	{
		constantValue = value;
	}
	bool ConstantIntNode::SetInputLink(int index, std::shared_ptr<INode> node) { return false; }

	NoiseSourceNode::NoiseSourceNode() : Node<float>(LinkType::Float), input_frequency(LinkType::Float), input_persistance(LinkType::Float), input_octaveCount(LinkType::Int) {	};

	float NoiseSourceNode::GetValue(const double x, const double y, const  double z, float dummy) {

		if (x >= 0 && x < noiseDimention && y >= 0 && y < noiseDimention && z >= 0 && z < noiseDimention) {
			float val = noiseSet[(int)(x * noiseDimention * noiseDimention + y * noiseDimention + z)];
			return val;
		}
		return -1; //not in bounds. shouldn't happen but who knows (fortunately -1 is a very valid value, but its easy to tell if things went awry if everythign is -1
	}

	bool NoiseSourceNode::GenerateNoiseSet(int seed, int numCells, glm::vec3 pos, float scaleModifier) {
		myNoise->SetSeed(seed);
		myNoise->SetFrequency(input_frequency.GetValue());
		myNoise->SetFractalOctaves(input_octaveCount.GetValue());
		noiseDimention = numCells;
		
		noiseSet = myNoise->GetNoiseSet(pos.x, pos.y, pos.z, numCells, numCells, numCells, scaleModifier);
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

	float SelectorNode::GetValue(const double x, const double y, const double z) {
		//alpha * black + (1 - alpha) * red
		float alpha = input_cutoff.GetValue(x, y, z);
		float a = input_a.GetValue(x, y, z);
		float b = input_b.GetValue(x, y, z);

		return alpha * a + (1 - alpha) + b;
	}


	TerGenNodeGraph::TerGenNodeGraph(int seed, int numCells, glm::vec3 pos, float scaleModifier) : seed(seed), cellsWide(numCells), pos(pos), scale(scaleModifier)
	{
		outputNode = std::shared_ptr<OutputNode>(new OutputNode());
		AddNode(outputNode);
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

	float TerGenNodeGraph::SampleHeight(const double x, const double y, const double z) {
		if (x >= pos.x && x < pos.x + cellsWide && y >= pos.y && y < pos.y + cellsWide && z >= pos.z && z < pos.z + cellsWide)
			return outputNode->GetValue(x, y, z, 1.0f);
		return -0.1;
	}

	//Preps graph to prepare for reading.
	//Should update the noise modules to use latest settings and compute their values
	void TerGenNodeGraph::BuildNoiseGraph() {
		float valueOut = 0;

		auto noiseSource = std::shared_ptr<NoiseSourceNode>(new NoiseSourceNode());

		auto freq = std::shared_ptr<ConstantFloatNode>(new ConstantFloatNode());
		freq->SetValue(0.005);

		auto persistance = std::shared_ptr<ConstantFloatNode>(new ConstantFloatNode());
		persistance->SetValue(0.5);

		auto octaveCount = std::shared_ptr<ConstantIntNode>(new ConstantIntNode());
		octaveCount->SetValue(3);

		noiseSource->SetInputLink(0, freq);
		noiseSource->SetInputLink(1, persistance);
		noiseSource->SetInputLink(2, octaveCount);

		outputNode->SetInputLink(0, noiseSource);

		noiseSource->GenerateNoiseSet(seed, cellsWide, pos, scale);

		//Test the noise
		//for (int i = 0; i < 10; i++)
		//{
		//	for (int j = 0; j < 10; j++)
		//	{
		//		valueOut = outputNode->GetValue(i, 0, j, 0.0f);
		//		std::cout << "value of 5 at " << i << " " << 0 << " " << j << " is " << valueOut << std::endl;
		//	}
		//
		//}


	}

}