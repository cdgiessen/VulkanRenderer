#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "util/FileWatcher.h"

namespace job
{
class TaskManager;
}
namespace Resource::Shader
{

enum class ShaderType
{
	vertex,
	tess_control,
	tess_eval,
	geometry,
	fragment,
	compute,
	error,
};

struct ShaderInfo
{
	std::string name;
	ShaderType type;
	std::vector<uint32_t> spirv_data;
};

using ShaderID = uint32_t;

class ShaderDatabase
{
	public:
	ShaderDatabase ();

	void Load ();
	void Save ();
	void Refresh ();
	void Discover ();
	std::vector<std::filesystem::path> StaleHandles ();

	// void AddEntry (ShaderDatabaseHandle handle);

	struct DBHandle
	{
		ShaderID id;
		std::string filename;
		ShaderType type;
	};

	private:
	std::vector<DBHandle> entries;
};

class ShaderCompiler
{
	public:
	ShaderCompiler ();
	std::optional<std::vector<uint32_t>> const compile_glsl_to_spirv (std::string const& shader_name,
	    std::string const& shader_data,
	    ShaderType const shader_type,
	    std::filesystem::path include_path = std::filesystem::path{});

	std::optional<std::string> LoadFileData (std::string const& filename);
};


class Manager
{
	public:
	Manager (job::TaskManager& task_manager);

	ShaderID AddShader (std::string name);

	std::vector<uint32_t> GetSpirVData (ShaderID id);
	std::vector<uint32_t> GetSpirVData (std::string const& name, ShaderType type);


	ShaderID GetShaderIDByName (std::string s);


	ShaderCompiler compiler;
	ShaderDatabase database;

	private:
	std::vector<uint32_t> AlignData (std::vector<char> const& code);

	job::TaskManager& task_manager;
	std::mutex lock;
	ShaderID cur_id = 0;
	std::unordered_map<ShaderID, ShaderInfo> shaders;
};
} // namespace Resource::Shader