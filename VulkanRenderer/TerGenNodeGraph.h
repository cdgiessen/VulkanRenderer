#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include <glm\common.hpp>

#include "ImGui\imgui.h"

#include "FastNoiseSIMD\FastNoiseSIMD.h"

namespace NewNodeGraph {

	enum LinkType {
		None = 0,
		Float = 1,
		Double = 2,
		Int = 3,
		Bool = 4,
		Vector2f = 5,
		Vector3f = 6,
		Vector4f = 7,
		Color3f = 8,
		Color4f = 9,
		String = 10,
	};

	class INode {
	public:
		virtual LinkType GetNodeType() =0;
		virtual float GetValue(const double x, const double y, const  double z, float dummy) =0;
		virtual int GetValue(const double x, const double y, const  double z, int dummy) =0;
		virtual double GetValue(const double x, const double y, const  double z, double dummy) =0;
	};

	template<typename T>
	class Link {
	public:

		Link(LinkType type);

		T GetValue(double x, double y, double z);

		T GetValue();

		LinkType GetLinkType();

		bool SetInputNode(std::shared_ptr<INode> node);

	private:
		LinkType linkType;
		T data;
		std::shared_ptr<INode> input;
	};

	template<typename T>
	class Node : public INode {
	public:

		Node(LinkType outputLinkType);

		LinkType GetNodeType();

		//virtual T GetValue(const double x, const double y, const  double z);
		virtual float GetValue(const double x, const double y, const  double z, float dummy);
		virtual int GetValue(const double x, const double y, const  double z, int dummy);
		virtual double GetValue(const double x, const double y, const  double z, double dummy);
		virtual bool SetInputLink(int index, std::shared_ptr<INode> node) = 0;

	private:
		LinkType outputType;
		T outputValue;
	};

	class OutputNode : public Node<float> {
	public:
		OutputNode();

		float GetValue(const double x, const double y, const  double z, float dummy);

		bool SetInputLink(int index, std::shared_ptr<INode> node);

	private:
		Link<float> input_output; //lol well some names are just confusing aren't they?
	};

	class ConstantFloatNode : public Node<float> {
	public:
		ConstantFloatNode();

		float GetValue(const double x, const double y, const  double z, float dummy);
		void SetValue(const float value);

		bool SetInputLink(int index, std::shared_ptr<INode> node);

	private:
		float constantValue;

	};

	class ConstantIntNode : public Node<int> {
	public:
		ConstantIntNode();

		int GetValue(const double x, const double y, const  double z, int dummy);
		void SetValue(const int value);

		bool SetInputLink(int index, std::shared_ptr<INode> node);

	private:
		int constantValue;

	};

	class NoiseSourceNode : public Node<float> {
	public:
		NoiseSourceNode();

		float GetValue(const double x, const double y, const  double z, float dummy);

		virtual bool GenerateNoiseSet(int seed, int numCells, glm::vec3 pos, float scaleModifier);

		bool CleanNoiseSet();

		bool SetInputLink(int index, std::shared_ptr<INode> node);

	private:
		Link<float> input_frequency;
		Link<float> input_persistance;
		Link<int> input_octaveCount;

		FastNoiseSIMD* myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
		float* noiseSet;
		int noiseDimention;
	};


	class SelectorNode : Node<float> {
	public:
		float GetValue(const double x, const double y, const double z);
	private:
		Link<float> input_cutoff;
		Link<float> input_blendAmount; //not done currently.
		Link<float> input_a;
		Link<float> input_b;

	};




	class TerGenNodeGraph
	{
	public:
		TerGenNodeGraph(int seed, int numCells, glm::vec3 pos, float scaleModifier);
		~TerGenNodeGraph();

		bool AddNode(std::shared_ptr<INode> node);
		bool AddNoiseSourceNode(std::shared_ptr<NoiseSourceNode> node);

		void BuildNoiseGraph();

		float SampleHeight(const double x, const double y, const double z);

	private:
		int seed;
		glm::vec3 pos;
		float scale;
		int cellsWide;

		std::vector<std::shared_ptr<INode>> nodes;
		std::shared_ptr<OutputNode> outputNode;

		std::vector<std::shared_ptr<NoiseSourceNode>> noiseSources;
	};

}