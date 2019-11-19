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
	ShaderModule (VkDevice device, ShaderType type, std::vector<uint32_t> const& code);
	~ShaderModule ();

	ShaderModule (ShaderModule const& mod) = delete;
	ShaderModule& operator= (ShaderModule const& mod) = delete;
	ShaderModule (ShaderModule&& mod);
	ShaderModule& operator= (ShaderModule&& mod);

	VkDevice device;
	ShaderType type;
	VkShaderModule module = nullptr;
};

class ShaderModuleSet
{
	public:
	ShaderModuleSet ();
	ShaderModuleSet (VkShaderModule vert, VkShaderModule frag);

	ShaderModuleSet& Vertex (VkShaderModule vert);
	ShaderModuleSet& Fragment (VkShaderModule frag);
	ShaderModuleSet& Geometry (VkShaderModule geom);
	ShaderModuleSet& TessControl (VkShaderModule tess_control);
	ShaderModuleSet& TessEval (VkShaderModule tess_eval);

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos ();

	private:
	VkShaderModule vert = VK_NULL_HANDLE;
	VkShaderModule frag = VK_NULL_HANDLE;
	VkShaderModule geom = VK_NULL_HANDLE;
	VkShaderModule tess_control = VK_NULL_HANDLE;
	VkShaderModule tess_eval = VK_NULL_HANDLE;
};


using ShaderID = uint32_t;
class ShaderManager
{
	public:
	ShaderManager (Resource::Shader::Manager& resource_shader_manager, VulkanDevice& device);
	~ShaderManager ();

	ShaderID CreateModule (std::string const& name, ShaderType type, std::vector<uint32_t> const& code);

	void DeleteModule (ShaderID const& id);

	VkShaderModule GetModule (ShaderID const& id);

	VkShaderModule GetModule (std::string name, ShaderType type);

	private:
	Resource::Shader::Manager& resource_shader_manager;
	VulkanDevice const& device;

	ShaderID cur_id = 0;
	std::unordered_map<ShaderID, ShaderModule> module_map;

	std::optional<std::vector<char>> readShaderFile (const std::string& filename);
};
