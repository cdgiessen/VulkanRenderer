#pragma once

#include "cml/cml.h"

#include "rendering/RenderStructs.h"

#include "util/DoubleBuffer.h"

struct TransformData
{
	cml::vec3f pos = cml::vec3f (0.0f, 0.0f, 0.0f);
	cml::vec3f scale = cml::vec3f (1.0f, 1.0f, 1.0f);
	cml::quatf rot = cml::quatf (1.0f, 0.0f, 0.0f, 0.0f);
	bool isDirty = false;
	bool isStatic = false; // for future use
};

constexpr int MaxTransformCount = 16384;

class TransformManager
{
	public:
	TransformManager ();

	int Allocate ();
	void Free (int index);

	TransformData Get (int index);
	void Set (int index, TransformData& data);

	void CalcMatrices (TransformMatrixData* writeLoc);

	private:
	DoubleBufferArray<TransformData, MaxTransformCount> transformData;
};