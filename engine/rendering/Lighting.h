#pragma once
#include <array>

#include "cml/cml.h"

#include "rendering/backend/Buffer.h"
#include "rendering/backend/Descriptor.h"
#include "rendering/backend/Texture.h"

class VulkanDevice;

struct DirectionalLight
{
	cml::vec3f direction;
	cml::vec3f color;
	float intensity = 0.0f;
};

struct PointLight
{
	cml::vec3f position;
	cml::vec3f color;
	float attenuation = 0.0f;
	float cufOff = 0.0f;
};

struct SpotLight
{
	cml::vec3f position;
	cml::vec3f direction;
	cml::vec3f color;
	float attenuation = 0.0f;
	float cutoff = 0.0f;
	float outerCutOff = 0.0f;
};

const uint32_t MaxDirectionalLightCount = 4;
const uint32_t MaxPointLightCount = 16;
const uint32_t MaxSpotLightCount = 16;

class LightingManager
{
	public:
	LightingManager (VulkanDevice& device, DescriptorManager& descriptor_manager, TextureManager& texture_manager);

	void Update (std::vector<DirectionalLight> directional_lights,
	    std::vector<PointLight> point_lights,
	    std::vector<SpotLight> spot_lights);

	void Bind (VkCommandBuffer buffer, VkPipelineLayout layout);
	void Advance ();

	private:
	VulkanDevice& device;
	DoubleBuffer directional_gpu_data;
	DoubleBuffer point_gpu_data;
	DoubleBuffer spot_gpu_data;

	int cur_index = 0;
	std::array<DescriptorSet, 2> lighting_descriptors;
};