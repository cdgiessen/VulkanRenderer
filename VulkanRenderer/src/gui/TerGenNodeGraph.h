#pragma once



#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include <glm\common.hpp>


#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\third-party\ImGui\imgui.h"

#include "..\third-party\FastNoiseSIMD\FastNoiseSIMD.h"

namespace NewNodeGraph {

	enum class LinkType {
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
		virtual ~INode();

		virtual LinkType GetNodeType() =0;
		virtual float GetValue(const int x, const int y, const int z, float dummy) =0;
		virtual int GetValue(const int x, const int y, const int z, int dummy) =0;
		virtual double GetValue(const int x, const int y, const int z, double dummy) =0;
		virtual glm::vec2 GetValue(const int x, const int y, const int z, glm::vec2 dummy) = 0;
		virtual glm::vec3 GetValue(const int x, const int y, const int z, glm::vec3 dummy) = 0;
		virtual glm::vec4 GetValue(const int x, const int y, const int z, glm::vec4 dummy) = 0;
	};

	template<typename T>
	class Link {
	public:

		Link(LinkType type);
		Link(LinkType type, T data);

		T GetValue(const int x, const int y, const int z);

		T GetValue();
		void SetValue(T);

		LinkType GetLinkType();

		bool SetInputNode(std::shared_ptr<INode> node);

	private:
		LinkType linkType = LinkType::None;
		T data;
		std::shared_ptr<INode> input = nullptr;
	};

	template<typename T>
	class Node : public INode {
	public:

		Node(LinkType outputLinkType);
		virtual ~Node() =0;

		virtual LinkType GetNodeType();

		//virtual T GetValue(const double x, const double y, const  double z);
		virtual float	GetValue(const int x, const int y, const int z, float dummy);
		virtual int		GetValue(const int x, const int y, const int z, int dummy);
		virtual double	GetValue(const int x, const int y, const int z, double dummy);
		virtual glm::vec2 GetValue(const int x, const int y, const int z, glm::vec2 dummy);
		virtual glm::vec3 GetValue(const int x, const int y, const int z, glm::vec3 dummy);
		virtual glm::vec4 GetValue(const int x, const int y, const int z, glm::vec4 dummy);

		virtual bool SetInputLink(int index, std::shared_ptr<INode> node) = 0;

	private:
		LinkType outputType;
		T outputValue;
	};

	class OutputNode : public Node<float> {
	public:
		OutputNode();
		~OutputNode() override;

		float GetValue(const int x, const int y, const int z, float dummy) override;

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;

	private:
		Link<float> input_output; //lol well some names are just confusing aren't they?
	};

	class ConstantFloatNode : public Node<float> {
	public:
		ConstantFloatNode();
		ConstantFloatNode(float value);
		~ConstantFloatNode() override;

		float GetValue(const int x, const int y, const int z, float dummy) override;
		void SetValue(const float value);

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;

	private:
		Link<float> value;
	};

	class ConstantIntNode : public Node<int> {
	public:
		ConstantIntNode();
		ConstantIntNode(int value);
		~ConstantIntNode() override;

		int GetValue(const int x, const int y, const int z, int dummy) override;
		void SetValue(const int value);

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;
	private:
		Link<int> value;
	};

	class SelectorNode : Node<float> {
	public:
		~SelectorNode();

		float GetValue(const int x, const int y, const int z, float dummy) override;
	private:
		Link<float> input_cutoff;
		Link<float> input_blendAmount; //not done currently.
		Link<float> input_a;
		Link<float> input_b;

	};

	class NoiseSourceNode : public Node<float> {
	public:
		NoiseSourceNode();
		~NoiseSourceNode() override;

		float GetValue(const int x, const int y, const int z, float dummy) override;

		virtual bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier);

		bool CleanNoiseSet();

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;

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

		float SampleHeight(const float x, const float y, const float z);
		std::vector<float>& GetOutputGreyscaleImage();

	private:
		int seed;
		glm::i32vec2 pos;
		float scale;
		int cellsWide;

		std::vector<float> outputImage;

		std::vector<std::shared_ptr<INode>> nodes;
		OutputNode outputNode;

		std::vector<std::shared_ptr<NoiseSourceNode>> noiseSources;
	};

	class TEMPTerGenNodeGraph
	{
	public:
		TEMPTerGenNodeGraph(std::vector<float>* data, int cells) {
			cellsWide = cells;
			outputImage = data;
		}

		int cellsWide;
		std::vector<float>* outputImage;

		float SampleHeight(const float x, const float y, const float z) {
			float xScaled = x*cellsWide;
			float yScaled = y*cellsWide;
			float zScaled = z*cellsWide;

			int realX = (int)glm::clamp(x * (float)(cellsWide), 0.0f, (float)cellsWide - 1);
			int realY = (int)glm::clamp(y * (float)(cellsWide), 0.0f, (float)cellsWide - 1);
			int realZ = (int)glm::clamp(z * (float)(cellsWide), 0.0f, (float)cellsWide - 1);

			int realXPlus1 = (int)glm::clamp(x * (float)(cellsWide)+1, 0.0f, (float)cellsWide - 1); //make sure its not greater than the image size
			int realYPlus1 = (int)glm::clamp(y * (float)(cellsWide)+1, 0.0f, (float)cellsWide - 1);
			int realZPlus1 = (int)glm::clamp(z * (float)(cellsWide)+1, 0.0f, (float)cellsWide - 1);



			if (realX >= 0 && realX < cellsWide && realY >= 0 && realY < cellsWide && realZ >= 0 && realZ < cellsWide) {

				float UL = outputImage->at(realX * cellsWide + realZ);
				float UR = outputImage->at(realX * cellsWide + realZPlus1);
				float DL = outputImage->at(realXPlus1 * cellsWide + realZ);
				float DR = outputImage->at(realXPlus1 * cellsWide + realZPlus1);


				return (
					UL * ((float)realXPlus1 - xScaled)	* ((float)realZPlus1 - zScaled)
					+ DL * (xScaled - (float)realX)		* ((float)realZPlus1 - zScaled)
					+ UR * ((float)realXPlus1 - xScaled)	* (zScaled - (float)realZ)
					+ DR * (xScaled - (float)realX)		* (zScaled - (float)realZ)
					)
					/ (((float)realXPlus1 - (float)realX) * ((float)realZPlus1 - (float)realZ));

				//return outputImage[realX * cellsWide + realZ];
			}
			return -1.0;
		}
	};

}