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

	ShaderModule (const ShaderModule& mod) = default;
	ShaderModule& operator= (const ShaderModule& mod) = default;

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
	std::optional<std::vector<unsigned int>> const CompileShaderString (
	    std::string const& shader_filename, std::string const& shader_string, ShaderType const shader_type);

	std::vector<uint32_t> const LoadAndCompileShader (std::string const& filename);

	private:
	std::optional<std::string> load_file (std::string const& filename);
};

class ShaderManager
{
	public:
	ShaderManager (VulkanDevice& device);
	~ShaderManager ();

	ShaderModule loadShaderModule (const std::string& codePath, ShaderType type);


	private:
	const VulkanDevice& device;
	std::vector<ShaderModule> shaderModules;
	ShaderCompiler compiler;
	ShaderDatabase database;

	std::optional<std::vector<char>> readShaderFile (const std::string& filename);
};
