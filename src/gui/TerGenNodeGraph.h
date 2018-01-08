#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include <glm/glm.hpp>

#include "../../third-party/json/json.hpp"

#include "../../third-party/ImGui/imgui.h"

#include "../../third-party/FastNoiseSIMD/FastNoiseSIMD.h"

namespace NewNodeGraph {

	class NoiseImage2D {
	public:
		NoiseImage2D();

		NoiseImage2D(int width);
		~NoiseImage2D();
		
		//resize underlying vector to appropriate size
		void PrepareImage();

		//size, in pixels, of the image;
		const int GetSize() const;

		//dimention of the image
		const int GetImageWidth() const;

		void SetWidth(int width);

		const size_t GetSizeBytes() const;

		//No error look
		const float LookUp(int x, int z) const;

		//Error checked sample, returns -1 if fails
		const float BoundedLookUp(int x, int z) const;

		void SetPixelValue(int x, int z, float value);

		float* GetImageData();
		void SetImageData(float* data);

	private:
		int width;
		float* image;
	};

	static float BilinearImageSample2D(const NoiseImage2D& noiseImage, const float x, const float z);

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

	class INode;

	template<typename T>
	class Link {
	public:

		Link(LinkType type);
		Link(LinkType type, T data);

		T GetValue(const int x, const int y, const int z) const;

		T GetData() const;
		void SetValue(T);

		const LinkType GetLinkType() const;

		bool SetInputNode(std::shared_ptr<INode> node);
		bool ResetInputNode();

		std::string GetJsonStringRep() const;

	private:
		LinkType linkType = LinkType::None;
		T data;
		std::shared_ptr<INode> input = nullptr;
	};

	class INode {
	public:
		virtual ~INode() = 0;

		virtual LinkType GetNodeType() const = 0;

		virtual float		GetValue(const int x, const int y, const int z, float dummy) const = 0;
		virtual int			GetValue(const int x, const int y, const int z, int dummy) const = 0;
		virtual double		GetValue(const int x, const int y, const int z, double dummy) const = 0;
		virtual glm::vec2	GetValue(const int x, const int y, const int z, glm::vec2 dummy) const = 0;
		virtual glm::vec3	GetValue(const int x, const int y, const int z, glm::vec3 dummy) const = 0;
		virtual glm::vec4	GetValue(const int x, const int y, const int z, glm::vec4 dummy) const = 0;

		virtual void SaveToJson(nlohmann::json& json) = 0;

		virtual bool SetInputLink(int index, std::shared_ptr<INode> node) = 0;
		virtual bool ResetInputLink(int index) =0;

		virtual void SetValue(const int index, float value) = 0;
		virtual void SetValue(const int index, int value) = 0;
		virtual void SetValue(const int index, double value) = 0;
		virtual void SetValue(const int index, glm::vec2 value) = 0;
		virtual void SetValue(const int index, glm::vec3 value) = 0;
		virtual void SetValue(const int index, glm::vec4 value) = 0;

		void SetID(int ID);
	protected:
		int ID;
	};

	template<typename T>
	class Node : public INode {
	public:

		Node(LinkType outputLinkType);
		virtual ~Node() override;

		virtual LinkType GetNodeType() const;

		//virtual T GetValue(const int x, const int y, const  int z, T dummy) const;
		virtual float	GetValue(const int x, const int y, const int z, float dummy) const ;
		virtual int		GetValue(const int x, const int y, const int z, int dummy) const ;
		virtual double	GetValue(const int x, const int y, const int z, double dummy) const ;
		virtual glm::vec2 GetValue(const int x, const int y, const int z, glm::vec2 dummy) const;
		virtual glm::vec3 GetValue(const int x, const int y, const int z, glm::vec3 dummy) const;
		virtual glm::vec4 GetValue(const int x, const int y, const int z, glm::vec4 dummy) const;

		virtual bool SetInputLink(int index, std::shared_ptr<INode> node) = 0;
		virtual bool ResetInputLink(int index) = 0;

		virtual void SetValue(const int index, float value);
		virtual void SetValue(const int index, int value);
		virtual void SetValue(const int index, double value);
		virtual void SetValue(const int index, glm::vec2 value);
		virtual void SetValue(const int index, glm::vec3 value);
		virtual void SetValue(const int index, glm::vec4 value);

		virtual void SaveToJson(nlohmann::json& json);

	private:
		LinkType outputType;
		T outputValue;
	};

	class OutputNode : public Node<float> {
	public:
		OutputNode();
		~OutputNode() override;

		float GetValue(const int x, const int y, const int z, float dummy) const override;

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;
		bool ResetInputLink(int index) override;

		void SetValue(const int index, float value) override;

		void SaveToJson(nlohmann::json& json) override;
	private:
		Link<float> input_output; //lol well some names are just confusing aren't they?
	};

	class ConstantFloatNode : public Node<float> {
	public:
		ConstantFloatNode(float value);
		~ConstantFloatNode() override;

		float GetValue(const int x, const int y, const int z, float dummy) const override;
		void SetValue(const float value);

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;
		bool ResetInputLink(int index) override;

		void SetValue(const int index, float value) override;

		void SaveToJson(nlohmann::json& json) override;
	private:
		Link<float> value;
	};

	class ConstantIntNode : public Node<int> {
	public:
		ConstantIntNode(int value);
		~ConstantIntNode() override;

		int GetValue(const int x, const int y, const int z, int dummy) const override;
		void SetValue(const int value);

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;
		bool ResetInputLink(int index) override;

		void SetValue(const int index, int value) override;

		void SaveToJson(nlohmann::json& json) override;
	private:
		Link<int> value;
	};

	class MathNode : public Node<float> {
	public:
		MathNode();
		virtual ~MathNode() override;

		virtual float GetValue(const int x, const int y, const int z, float dummy) const override;

		virtual bool SetInputLink(int index, std::shared_ptr<INode> node) override;
		bool ResetInputLink(int index) override;

		void SetValue(const int index, float value) override;

		void SaveToJson(nlohmann::json& json) override;
	protected:
		Link<float> input_a;
		Link<float> input_b;
	};

	class AdditionNode : public MathNode {
	public:
		float GetValue(const int x, const int y, const int z, float dummy) const override;
	};

	class SubtractNode : public MathNode {
	public:
		float GetValue(const int x, const int y, const int z, float dummy) const override;
	};

	class MultiplyNode : public MathNode {
	public:
		float GetValue(const int x, const int y, const int z, float dummy) const override;
	};

	class DivideNode : public MathNode {
	public:
		float GetValue(const int x, const int y, const int z, float dummy) const override;
	};

	class PowerNode : public MathNode {
	public:
		float GetValue(const int x, const int y, const int z, float dummy) const override;
	};

	class SelectorNode : public Node<float> {
	public:
		SelectorNode();
		~SelectorNode();

		float GetValue(const int x, const int y, const int z, float dummy) const override;

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;
		bool ResetInputLink(int index) override;

		void SetValue(const int index, float value) override;

		void SaveToJson(nlohmann::json& json) override;
	private:
		Link<float> input_a;
		Link<float> input_b;
		Link<float> input_cutoff;
		Link<float> input_blendAmount; //not done currently.

	};

	class NoiseSourceNode : public Node<float> {
	public:
		NoiseSourceNode();
		~NoiseSourceNode() override;

		float GetValue(const int x, const int y, const int z, float dummy) const override;

		virtual bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier);
		bool CleanNoiseSet();

		bool SetInputLink(int index, std::shared_ptr<INode> node) override;
		bool ResetInputLink(int index) override;

		void SetValue(const int index, float value) override;
		void SetValue(const int index, int value) override;

		void SaveToJson(nlohmann::json& json) override;
	protected:
		Link<int> input_octaveCount;
		Link<float> input_persistance;
		Link<float> input_frequency;

		FastNoiseSIMD* myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
		NoiseImage2D noiseSet;
		int noiseDimention;
	};

	class ValueFractalNoiseNode : public NoiseSourceNode {
	public:
		bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) override;
	};

	class SimplexFractalNoiseNode : public NoiseSourceNode {
	public:
		bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) override;
	};

	class PerlinFractalNoiseNode : public NoiseSourceNode {
	public:
		bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) override;
	};

	class WhiteNoiseNode : public NoiseSourceNode {
	public:
		bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) override;
	};

	class CellularNoiseNode : public NoiseSourceNode {
	public:
		bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) override;
	};

	class CubicFractalNoiseNode : public NoiseSourceNode {
	public:
		bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) override;
	};

	class VoironiFractalNoiseNode : public NoiseSourceNode {
	public:
		bool GenerateNoiseSet(int seed, int numCells, glm::ivec2 pos, float scaleModifier) override;
	};


	class TerGenNodeGraph
	{
	public:
		//TerGenNodeGraph(const TerGenNodeGraph&) = default;
		TerGenNodeGraph();
		~TerGenNodeGraph();

		bool AddNode(std::shared_ptr<INode> node);
		bool AddNoiseSourceNode(std::shared_ptr<NoiseSourceNode> node);

		bool DeleteNode(std::shared_ptr<INode> node);

		void BuildNoiseGraph();

		nlohmann::json  SaveGraphToJson();
		void LoadGraphFromJson(nlohmann::json inputjson);

		const std::shared_ptr<OutputNode> GetOutputNode() const;

		int GetNextID();

		const std::vector<std::shared_ptr<NoiseSourceNode>>& GetNoiseSources()const ;

	private:
		int IDCounter = 0;
		std::vector<std::shared_ptr<INode>> nodes;

		std::shared_ptr<OutputNode> outputNode;

		std::vector<std::shared_ptr<NoiseSourceNode>> noiseSources;


	};

	class TerGenGraphUser {
	public:
		TerGenGraphUser(const TerGenNodeGraph& sourceGraph, int seed, int numCells, glm::i32vec2 pos, float scale);

		void BuildOutputImage(glm::i32vec2 pos, float scale);

		float SampleHeight(const float x, const float y, const float z);
		float* GetOutputGreyscaleImage();

	private:
		const TerGenNodeGraph& sourceGraph;

		int seed;
		glm::i32vec2 pos;
		float scale;
		int cellsWide;

		NoiseImage2D outputImage;
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
