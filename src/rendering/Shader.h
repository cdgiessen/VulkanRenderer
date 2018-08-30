#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Device.h"
#include <vulkan/vulkan.h>

VkShaderModule loadShaderModule (VkDevice device, const std::string& codePath);

enum class ShaderModuleType
{
	vertex,
	fragment,
	geometry,
	tessControl,
	tessEval
};

// Manages Shaderlife times
struct ShaderModule
{
	ShaderModule ();

	ShaderModule (ShaderModuleType type, VkShaderModule module);

	ShaderModule (const ShaderModule& mod) = default;
	ShaderModule& operator= (const ShaderModule& mod) = default;

	ShaderModuleType type;
	VkShaderModule module;
	VkPipelineShaderStageCreateInfo createInfo;
};

class ShaderModuleSet
{
	public:
	// default, no data set.
	ShaderModuleSet ();

	// initializes data members
	ShaderModuleSet (ShaderModule vert,
	    std::optional<ShaderModule> frag = {},
	    std::optional<ShaderModule> geom = {},
	    std::optional<ShaderModule> tessControl = {},
	    std::optional<ShaderModule> tessEval = {});

	ShaderModuleSet (const ShaderModuleSet& set) = default;
	ShaderModuleSet& operator= (const ShaderModuleSet& set) = default;


	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos ();

	private:
	ShaderModule vert;

	std::optional<ShaderModule> frag ;
	std::optional<ShaderModule> geom ;
	std::optional<ShaderModule> tessControl;
	std::optional<ShaderModule> tessEval;

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


	std::optional<std::vector<char>> readShaderFile (const std::string& filename);
};
