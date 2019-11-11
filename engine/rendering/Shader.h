#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "util/FileWatcher.h"

namespace Resource::Shader
{
class Manager;
}

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

class ShaderManager
{
	public:
	ShaderManager (Resource::Shader::Manager& resource_shader_manager, VulkanDevice& device);
	~ShaderManager ();

	std::optional<ShaderKey> load_and_compile_module (std::filesystem::path file);
	std::optional<ShaderKey> load_module (std::string const& name, std::string const& codePath, ShaderType type);
	std::optional<ShaderKey> create_module (
	    std::string const& name, ShaderType type, std::vector<uint32_t> const& code);

	void delete_module (ShaderKey const& key);

	std::optional<ShaderModule> get_module (ShaderKey const& key);
	std::optional<ShaderModule> get_module (std::string const& name, ShaderType const type);

	private:
	Resource::Shader::Manager& resource_shader_manager;
	VulkanDevice const& device;

	std::unordered_map<ShaderKey, ShaderModule> module_map;

	std::optional<std::vector<char>> readShaderFile (const std::string& filename);
};
