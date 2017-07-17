#pragma once

#include "Mesh.h"
#include "Texture.h"
#include <glm\common.hpp>

class Terrain {
public:
	int numCells; //
	glm::vec3 pos; //position of corner
	glm::vec3 size; //width and length
	Mesh* mesh;

	Terrain(int numCells, float posX, float posY, int sizeX, int sizeY) : numCells(numCells), pos(glm::vec3(posX, 0, posY)), size(glm::vec3(sizeX, 0, sizeY)) {
		mesh = generateTerrainMesh(numCells, pos.x, pos.z, size.x, size.z);

	}
};