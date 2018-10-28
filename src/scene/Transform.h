#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "rendering/RenderStructs.h"

#include "util/DoubleBuffer.h"

struct TransformData {
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	bool isDirty = false;
	bool isStatic = false; //for future use


};

constexpr int MaxTransformCount = 16384;

class TransformManager {
public:
	TransformManager();

	int Allocate();
	void Free(int index);

	TransformData Get(int index);
	void Set(int index, TransformData& data);

	void CalcMatrices(TransformMatrixData* writeLoc);

private:
	DoubleBufferArray<TransformData, MaxTransformCount> transformData;
};