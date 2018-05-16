#include "Material.h"

#include "Initializers.h"

#include "../core/CoreTools.h"

#include <json.hpp>

//VkShaderModule loadShaderModule(VkDevice device, const std::string& codePath) {
//	auto shaderCode = readFile(codePath);
//
//	VkShaderModuleCreateInfo createInfo = {};
//	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//	createInfo.codeSize = shaderCode.size();
//
//	std::vector<uint32_t> codeAligned(shaderCode.size() / 4 + 1);
//	memcpy(codeAligned.data(), shaderCode.data(), shaderCode.size());
//
//	createInfo.pCode = codeAligned.data();
//
//	VkShaderModule shaderModule;
//	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create shader module!");
//	}
//
//	return shaderModule;
//}

