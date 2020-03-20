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
#include "gltf.h"

namespace job
{
class ThreadPool;
}


namespace Resource
{

class Resources
{
	public:
	Resources (job::ThreadPool& thread_pool);
	~Resources ();

	private:
	job::ThreadPool& thread_pool;

	public:
	Mesh::Meshes meshes;
	Texture::Textures textures;
	Material::Materials materials;
	Shader::Shaders shaders;
};


} // namespace Resource
