#include "Transform.h"

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "core/Logger.h"



TransformManager::TransformManager() {

}

int TransformManager::Allocate() {
	return transformData.Allocate();
}
void TransformManager::Free(int index) {
	transformData.Free(index);
}

TransformData TransformManager::Get(int index) {
	return transformData.Read(index);
}

void TransformManager::Set(int index, TransformData& data) {
	data.isDirty = true;
	transformData.Write(index, data);
}

void TransformManager::CalcMatrices(TransformMatrixData* writeLoc) {
	for (int i = 0; i < MaxTransformCount; i++) {
		glm::mat4 transform;
		glm::mat4 normal;

		transform = glm::translate(transform, Get(i).pos);
		transform = transform * glm::mat4_cast(Get(i).rot);
		transform = glm::scale(transform, Get(i).scale);

		normal = glm::transpose(glm::inverse(glm::mat3(transform)));

		writeLoc[i] = { transform, normal };
	}

}