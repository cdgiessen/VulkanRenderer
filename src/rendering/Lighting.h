#pragma once

#include "cml/cml.h"

#include "DoubleBuffer.h"

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

struct LightingData
{
	VulkanBuffer dir_lights;
	VulkanBuffer point_lights;
	VulkanBuffer spot_lights;

	DescriptorSet lightingDescriptorSet;
};

class LightingManager
{
	public:
	LightingManager (VulkanDevice& device);



	VulkanDevice& device;
	std::vector<LightingData> data;
	DoubleBuffer gpu_data;
};