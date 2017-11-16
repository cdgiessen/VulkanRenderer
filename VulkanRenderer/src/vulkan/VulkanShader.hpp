#pragma once

#include <vector>

#include "VulkanDevice.hpp"
#include <vulkan\vulkan.h>
#include "VulkanInitializers.hpp"
#include "VulkanTools.h"


//Manages Shaderlife times
class VulkanShader
{
public:
	VulkanShader(const VulkanDevice &device);
	~VulkanShader();

	enum class ShaderType {
		vertex,
		fragment,
		geometry,
		tesselation
	};

	VkShaderModule loadShaderModule(std::shared_ptr<VulkanDevice> device, const std::string& codePath, VulkanShader::ShaderType type);

private:
	const VulkanDevice &device;
	std::vector<VkShaderModule> shaderModules;
	
	std::vector<char> readShaderFile(const std::string& filename);


	std::vector<char> defaultVertexShader = {
	};

	std::vector<char> defaultFragmentShader = {
	};
};

