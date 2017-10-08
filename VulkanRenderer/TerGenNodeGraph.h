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
		virtual float GetValue(const int x, const int y, const int z, float dummy) =0;
		virtual float GetValue(const int x, const int y, const int z, int dummy) =0;
		virtual double GetValue(const int x, const int y, const int z, double dummy) =0;
	};

	template<typename T>
	class Link {
	public:

		Link(LinkType type);

		T GetValue(const int x, const int y, const int z);

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
		virtual float GetValue(const int x, const int y, const int z, float dummy);
		virtual float GetValue(const int x, const int y, const int z, int dummy);
		virtual double GetValue(const int x, const int y, const int z, double dummy);
	
		virtual bool SetInputLink(int index, std::shared_ptr<INode> node) = 0;

	private:
		LinkType outputType;
		T outputValue;
	};

	class OutputNode : public Node<float> {
	public:
		OutputNode();

		float GetValue(const int x, const int y, const int z, float dummy);

		bool SetInputLink(int index, std::shared_ptr<INode> node);

	private:
		Link<float> input_output; //lol well some names are just confusing aren't they?
	};

	class ConstantFloatNode : public Node<float> {
	public:
		ConstantFloatNode();

		float GetValue(const int x, const int y, const int z, float dummy);
		void SetValue(const float value);

		bool SetInputLink(int index, std::shared_ptr<INode> node);

	private:
		float constantValue;

	};

	class SelectorNode : Node<float> {
	public:
		float GetValue(const int x, const int y, const int z);
	private:
		Link<float> input_cutoff;
		Link<float> input_blendAmount; //not done currently.
		Link<float> input_a;
		Link<float> input_b;

	};

	class NoiseSourceNode : public Node<float> {
	public:
		NoiseSourceNode();

		float GetValue(const int x, const int y, const int z, float dummy);

		virtual bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier);

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



	class TerGenNodeGraph
	{
	public:
		TerGenNodeGraph(const TerGenNodeGraph&) = default;
		TerGenNodeGraph(int seed, int numCells, glm::i32vec2 pos, float scale);
		~TerGenNodeGraph();

		bool AddNode(std::shared_ptr<INode> node);
		bool AddNoiseSourceNode(std::shared_ptr<NoiseSourceNode> node);

		void BuildNoiseGraph();
		void BuildOutputImage(glm::i32vec2 pos, float scale);

		float SampleHeight(const int x, const int y, const int z);

	private:
		int seed;
		glm::i32vec2 pos;
		float scale;
		int cellsWide;

		std::vector<float> outputImage;

		std::vector<std::shared_ptr<INode>> nodes;
		std::shared_ptr<OutputNode> outputNode;

		std::vector<std::shared_ptr<NoiseSourceNode>> noiseSources;
	};

}