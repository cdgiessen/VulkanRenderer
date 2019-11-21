#pragma once

#include "rendering/backend/BackEnd.h"

#include "rendering/backend/Descriptor.h"
#include "rendering/backend/Material.h"
#include "rendering/backend/Model.h"
#include "rendering/backend/Pipeline.h"
#include "rendering/backend/Texture.h"


struct MeshData
{
	ModelID model;
	MatInstanceID mat;
};

class MeshManager
{
	public:
	MeshManager (BackEnd& back_end);

	private:
	BackEnd& back_end;
};