#include "Transform.h"

#include "cml/cml.h"

#include "core/Logger.h"



TransformManager::TransformManager () {}

int TransformManager::Allocate () { return transformData.Allocate (); }
void TransformManager::Free (int index) { transformData.Free (index); }

TransformData TransformManager::Get (int index) { return transformData.Read (index); }

void TransformManager::Set (int index, TransformData& data)
{
	data.isDirty = true;
	transformData.Write (index, data);
}

void TransformManager::CalcMatrices (TransformMatrixData* writeLoc)
{
	for (int i = 0; i < MaxTransformCount; i++)
	{
		//		cml::mat4f transform;
		//		cml::mat4f normal;
		//
		//		transform = transform.translate (Get (i).pos);
		//		transform = transform * cml::to_mat4f (Get (i).rot);
		//		transform = cml::scale (transform, Get (i).scale);
		//
		//		normal = cml::transpose (cml::inverse (cml::mat3f (transform)));
		//
		//		writeLoc[i] = { transform, normal };
	}
}