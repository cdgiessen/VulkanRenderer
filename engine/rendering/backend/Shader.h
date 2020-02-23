#pragma once

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

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
class ShaderModule
{
	public:
	ShaderModule (VkDevice device, ShaderType type, std::vector<uint32_t> const& code);
	~ShaderModule ();

	ShaderModule (ShaderModule const& mod) = delete;
	ShaderModule& operator= (ShaderModule const& mod) = delete;
	ShaderModule (ShaderModule&& mod);
	ShaderModule& operator= (ShaderModule&& mod);

	VkShaderModule get () const { return module; }

	private:
	VkDevice device;
	ShaderType type;
	VkShaderModule module = nullptr;
};

class ShaderModuleSet
{
	public:
	ShaderModuleSet ();
	ShaderModuleSet (ShaderModule const& vert, ShaderModule const& frag);

	ShaderModuleSet& Vertex (ShaderModule const& vert);
	ShaderModuleSet& Fragment (ShaderModule const& frag);
	ShaderModuleSet& Geometry (ShaderModule const& geom);
	ShaderModuleSet& TessControl (ShaderModule const& tess_control);
	ShaderModuleSet& TessEval (ShaderModule const& tess_eval);

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos ();

	private:
	VkShaderModule vert = VK_NULL_HANDLE;
	VkShaderModule frag = VK_NULL_HANDLE;
	VkShaderModule geom = VK_NULL_HANDLE;
	VkShaderModule tess_control = VK_NULL_HANDLE;
	VkShaderModule tess_eval = VK_NULL_HANDLE;
};

namespace Resource::Shader
{
class Manager;
}
class ShaderManager
{
	public:
	ShaderManager (Resource::Shader::Manager& resource_shader_manager, VkDevice device);

	std::optional<ShaderModule> GetModule (std::string name, ShaderType type);

	private:
	Resource::Shader::Manager& resource_shader_manager;
	VkDevice device;
	std::optional<std::vector<char>> readShaderFile (const std::string& filename);
};
