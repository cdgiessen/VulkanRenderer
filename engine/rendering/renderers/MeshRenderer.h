#pragma once

#include "rendering/backend/BackEnd.h"

class MeshManager
{
	public:
	MeshManager (BackEnd& back_end);

	private:
	BackEnd& back_end;
};