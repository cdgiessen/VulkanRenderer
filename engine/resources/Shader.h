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
		std::filesystem::file_time_type glsl_last_write_time;
		std::filesystem::file_time_type spirv_last_write_time;
	};

	private:
	FileWatcher fileWatch;
	std::string shader_path = "assets/shaders/";
	std::string database_path = "assets/shader_db.json";

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

	std::optional<std::string> load_file_data (std::string const& filename);
};




class Manager
{
	public:
	Manager (job::TaskManager& task_manager);
	job::TaskManager& task_manager;

	std::string GetShaderString (ShaderID id);
	std::string GetShaderString (std::string name);

	ShaderCompiler compiler;
	ShaderDatabase database;

	private:
	std::unordered_map<ShaderID, ShaderInfo> shaders;
};
} // namespace Resource::Shader