#pragma once

#include "resources/Material.h"

#include "Descriptor.h"

class VulkanDevice;

using MatID = uint32_t;


class MatOutline
{
	public:
	MatOutline (VulkanDevice& device, Resource::Material::MaterialOutline const& outline);

	private:
	DescriptorSet descriptorSet;
	VulkanDescriptor descriptor;
};

class MatInstance
{
	public:
	MatInstance (VulkanDevice& device, MatOutline const& outline);

	private:
	DescriptorLayout layout;
};

class MaterialManager
{
	MaterialManager (Resource::Material::Manager& mat_man, VulkanDevice& device);

	std::unordered_map < MatID,
};