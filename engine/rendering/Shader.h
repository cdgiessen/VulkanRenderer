#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "util/FileWatcher.h"

class VulkanDevice;

VkShaderModule loadShaderModule (VkDevice device, const std::string& codePath);

enum class ShaderType
{
	vertex,
	tessControl,
	tessEval,
	geometry,
	fragment,
	compute,
	error
};

// get corresponding shader type (both vert and .vert)
ShaderType GetShaderStage (const std::string& stage);

// Manages Shaderlife times
struct ShaderModule
{
	ShaderModule ();
	ShaderModule (ShaderType type, VkShaderModule module);

	VkPipelineShaderStageCreateInfo GetCreateInfo ();

	ShaderType type;
	VkShaderModule module;
};

class ShaderModuleSet
{
	public:
	// default, no data set.
	ShaderModuleSet ();

	ShaderModuleSet& Vertex (ShaderModule vert);
	ShaderModuleSet& Fragment (ShaderModule frag);
	ShaderModuleSet& Geometry (ShaderModule geom);
	ShaderModuleSet& TessControl (ShaderModule tesc);
	ShaderModuleSet& TessEval (ShaderModule tese);

	ShaderModuleSet (const ShaderModuleSet& set) = default;
	ShaderModuleSet& operator= (const ShaderModuleSet& set) = default;

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos ();

	private:
	std::optional<ShaderModule> vert;
	std::optional<ShaderModule> frag;
	std::optional<ShaderModule> geom;
	std::optional<ShaderModule> tesc;
	std::optional<ShaderModule> tese;
};

struct ShaderKey
{
	std::string name;
	ShaderType type;

	bool operator== (const ShaderKey& other) const
	{
		return (name == other.name && type == other.type);
	}
};

namespace std
{

template <> struct hash<ShaderKey>
{
	std::size_t operator() (const ShaderKey& k) const
	{
		using std::hash;
		using std::size_t;
		using std::string;

		return ((hash<string> () (k.name) ^ (hash<ShaderType> () (k.type) << 1)) >> 1);
	}
};

} // namespace std

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
	std::optional<std::vector<uint32_t>> const compile_glsl_to_spriv (std::string const& shader_name,
	    std::string const& shader_data,
	    ShaderType const shader_type,
	    std::filesystem::path include_path = std::filesystem::path{});

	std::optional<std::string> load_file_data (std::string const& filename);
};

class ShaderManager
{
	public:
	ShaderManager (VulkanDevice& device);
	~ShaderManager ();

	std::optional<ShaderKey> load_and_compile_module (std::filesystem::path file);
	std::optional<ShaderKey> load_module (std::string const& name, std::string const& codePath, ShaderType type);
	std::optional<ShaderKey> create_module (
	    std::string const& name, ShaderType type, std::vector<uint32_t> const& code);

	void delete_module (ShaderKey const& key);

	std::optional<ShaderModule> get_module (ShaderKey const& key);
	std::optional<ShaderModule> get_module (std::string const& name, ShaderType const type);

	private:
	const VulkanDevice& device;

	ShaderCompiler compiler;
	ShaderDatabase database;

	std::unordered_map<ShaderKey, ShaderModule> module_map;

	std::optional<std::vector<char>> readShaderFile (const std::string& filename);
};
