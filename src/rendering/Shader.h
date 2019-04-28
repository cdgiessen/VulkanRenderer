#pragma once

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "util/FileWatcher.h"

class VulkanDevice;

VkShaderModule loadShaderModule (VkDevice device, const std::string& codePath);

enum class ShaderModuleType
{
	vertex,
	tessControl,
	tessEval,
	geometry,
	fragment,
	compute,
	error
};

// Manages Shaderlife times
struct ShaderModule
{
	ShaderModule ();

	ShaderModule (ShaderModuleType type, VkShaderModule module);

	ShaderModule (const ShaderModule& mod) = default;
	ShaderModule& operator= (const ShaderModule& mod) = default;

	VkPipelineShaderStageCreateInfo GetCreateInfo ();

	ShaderModuleType type;
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

struct ShaderDatabaseHandle
{
	std::string name;
	ShaderModuleType type;
};

class ShaderDatabase
{
	public:
	ShaderDatabase (std::string fileName);

	void Load ();
	void Save ();

	// void AddEntry (ShaderDatabaseHandle handle);

	private:
	FileWatcher fileWatch;
};

class ShaderCompiler
{
	public:
	ShaderCompiler ();
	std::vector<uint32_t> const CompileShaderString (
	    std::string const& shader_filename, std::string const& shader_string, ShaderModuleType const shader_type);

	std::vector<uint32_t> const LoadAndCompileShader (std::string const& filename);

	private:
	std::optional<std::string> load_file (std::string const& filename);
};

class ShaderManager
{
	public:
	ShaderManager (VulkanDevice& device);
	~ShaderManager ();

	ShaderModule loadShaderModule (const std::string& codePath, ShaderModuleType type);


	private:
	const VulkanDevice& device;
	std::vector<ShaderModule> shaderModules;
	ShaderCompiler compiler;

	std::optional<std::vector<char>> readShaderFile (const std::string& filename);
};
