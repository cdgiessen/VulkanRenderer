#pragma once

#include <vector>
#include <map>

#include "../resources/MeshManager.h"
#include "../resources/TextureManager.h"

#include "Entity.h"
#include "Transform.h"

using CompID = int;

class Component {
	CompID ID;


};

class Comp_Renderer : Component {

};

class Comp_Mesh : Component {

};

class Comp_Texture : Component {

};

class Comp_Material : Component {

};

class Comp_Renderer : Component {
	std::array<CompID, 8> material;
	std::array<CompID, 8> meshes;
};

class ComponentManager {
public:
	ComponentManager();



private:
	TransformManager transformMan;



};