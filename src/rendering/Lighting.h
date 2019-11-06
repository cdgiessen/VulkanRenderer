#pragma once


#include "Buffer.h"
#include "Descriptor.h"

#include "cml/cml.h"

class VulkanDevice;

struct DirectionalLight
{
	cml::vec3f direction;
	cml::vec3f color;
	float intensity = 0.0f;

	DirectionalLight (){};
	DirectionalLight (cml::vec3f dir, cml::vec3f color, float intensity)
	: direction (dir), color (color), intensity (intensity)
	{
	}
};

struct PointLight
{
	cml::vec3f position;
	cml::vec3f color;
	float attenuation = 0.0f;
	float cufOff = 0.0f;

	PointLight (){};
	PointLight (cml::vec3f position, cml::vec3f color, float attenuation)
	: position (position), color (color), attenuation (attenuation)
	{
	}
};

struct SpotLight
{
	cml::vec3f position;
	cml::vec3f direction;
	cml::vec3f color;
	float attenuation = 0.0f;
	float cutoff = 0.0f;
	float outerCutOff = 0.0f;


	SpotLight (){};
	SpotLight (cml::vec3f position, cml::vec3f dir, cml::vec3f color, float attenuation, float cutoff, float outerCutOff)
	: position (position), direction (dir), color (color), attenuation (attenuation), cutoff (cutoff), outerCutOff (outerCutOff)
	{
	}
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
};