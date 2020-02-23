#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "Sound.h"
#include "Texture.h"

namespace job
{
class TaskManager;
}


namespace Resource
{

class ResourceManager
{
	public:
	ResourceManager (job::TaskManager& task_manager);
	~ResourceManager ();

	private:
	job::TaskManager& task_manager;

	public:
	Mesh::Manager mesh_manager;
	Texture::Manager texture_manager;
	Material::Manager material_manager;
	Shader::Manager shader_manager;
};


} // namespace Resource
