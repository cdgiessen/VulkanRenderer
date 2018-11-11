#pragma once

#include <vector>
#include <map>

#include "resources/ResourceManager.h"

#include "Entity.h"
#include "Transform.h"

using CompID = int;

struct Component {
	CompID ID;


};

struct Comp_Transform : Component {

};

struct Comp_Collider : Component {

};

struct Comp_Rigidbody : Component {

};

struct Comp_Mesh : Component {

};

struct Comp_Texture : Component {

};

struct Comp_Material : Component {

};

struct Comp_Renderer : Component {
	std::array<CompID, 8> material;
	std::array<CompID, 8> meshes;
};

class ComponentManager {
public:
	ComponentManager(Resource::AssetManager& resourceMan);



private:
	TransformManager transformMan;
	Resource::AssetManager& resourceMan;


};