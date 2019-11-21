#pragma once

#include "cml/cml.h"

#include "rendering/backend/Buffer.h"

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

const uint32_t MaxDirectionalLightCount = 8;
const uint32_t MaxPointLightCount = 256;
const uint32_t MaxSpotLightCount = 256;

class LightingManager
{
	public:
	LightingManager (VulkanDevice& device);



	VulkanDevice& device;
	std::vector<DirectionalLight> directional_lights;
	std::vector<PointLight> point_lights;
	std::vector<SpotLight> spot_lights;
	DoubleBuffer directional_gpu_data;
	DoubleBuffer point_gpu_data;
	DoubleBuffer spot_gpu_data;
};