#include "VulkanShader.hpp"

VulkanShader::VulkanShader(const VulkanDevice &device) : device(device)
{
	shaderModules.reserve(32);
}


VulkanShader::~VulkanShader()
{
	for (auto item : shaderModules) {

		vkDestroyShaderModule(device.device, item, nullptr);
	}
}


VkShaderModule VulkanShader::loadShaderModule(std::shared_ptr<VulkanDevice> device, const std::string& codePath, VulkanShader::ShaderType type) {
	std::vector<char> shaderCode;
	try {
		shaderCode = readShaderFile(codePath);
	}
	catch (std::runtime_error){
		switch (type) {
		case(VulkanShader::ShaderType::vertex):
			shaderCode = defaultVertexShader;
			break;
		case(VulkanShader::ShaderType::fragment):
			shaderCode = defaultVertexShader;
			break;
		case(VulkanShader::ShaderType::geometry):
			shaderCode = defaultVertexShader;
			break;
		case(VulkanShader::ShaderType::tesselation):
			shaderCode = defaultVertexShader;
			break;
		default:
			throw std::runtime_error("shader type does not exist!");
		}
	}

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();

	std::vector<uint32_t> codeAligned(shaderCode.size() / 4 + 1);
	memcpy(codeAligned.data(), shaderCode.data(), shaderCode.size());

	createInfo.pCode = codeAligned.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	shaderModules.push_back(shaderModule);

	return shaderModule;
}

std::vector<char> VulkanShader::readShaderFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}
