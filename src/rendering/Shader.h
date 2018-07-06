#pragma once

#include <vector>
#include <string>
#include <optional>

#include "Device.h"
#include <vulkan/vulkan.h>

VkShaderModule loadShaderModule(VkDevice device, const std::string& codePath);

enum class ShaderModuleType {
	vertex,
	fragment,
	geometry,
	tessControl,
	tessEval
};

//Manages Shaderlife times
struct ShaderModule
{
	ShaderModule();
	
	ShaderModule(ShaderModuleType type,
		VkShaderModule module);

	ShaderModule(const ShaderModule& mod) = default;
	ShaderModule& operator=(const ShaderModule& mod) = default;

	ShaderModuleType type;
	VkShaderModule module;
	VkPipelineShaderStageCreateInfo createInfo;

};

class ShaderModuleSet {
public:
	//default, no data set.
	ShaderModuleSet();

	//initializes data members
	ShaderModuleSet(
		ShaderModule vert,
		ShaderModule frag,
		std::optional<ShaderModule> geom,
		std::optional<ShaderModule> tessControl,
		std::optional<ShaderModule> tessEval);

	ShaderModuleSet(const ShaderModuleSet& set) = default;
	ShaderModuleSet& operator=(const ShaderModuleSet& set) = default;


	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos();

private:
	ShaderModule vertexModule;
	ShaderModule fragmentModule;

	bool geomPresent = false;
	ShaderModule geometryModule;

	bool tessEvalPresent = false;
	ShaderModule tessEvalModule;

	bool tessControlPresent = false;
	ShaderModule tessControlModule;
};

class ShaderManager {
public:
	ShaderManager(VulkanDevice & device);
	~ShaderManager();

	ShaderModule loadShaderModule(const std::string& codePath, ShaderModuleType type);


private:
	const VulkanDevice &device;
	std::vector<ShaderModule> shaderModules;


	std::optional<std::vector<char>> readShaderFile(const std::string& filename);

};
