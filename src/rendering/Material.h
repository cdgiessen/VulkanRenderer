#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <glm/glm.hpp>



#include "Device.h"
#include "Texture.h"
#include "Descriptor.h"
#include "Pipeline.h"
#include "Shader.h"
#include "RenderStructs.h"

//VkShaderModule loadShaderModule(VkDevice device, const std::string& codePath);

class Material {
public:



private:
	std::shared_ptr<VulkanDescriptor> descriptor;
	DescriptorSet m_descriptorSet;

	std::vector<std::shared_ptr<VulkanTexture>> textures;



};