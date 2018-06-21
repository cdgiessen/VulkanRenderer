#pragma once

#include <glm/glm.hpp>



#include "../util/DoubleBuffer.h"

struct TransformData {
	glm::vec3 pos;
	glm::vec3 rot;
	glm::vec3 scale;
	bool isDirty = false;
	bool isStatic = false; //for future use


};


class TransformManager {
public:
	TransformManager();



private:
	DoubleBufferArray<TransformData, 16384> transformData;
};