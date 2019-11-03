#pragma once

#include "SG14/flat_map.h"

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
};

class MatInstance
{
	public:
	MatInstance (VulkanDevice& device, MatOutline const& outline);

	private:
	DescriptorLayout layout;
};

using MatOutlineID = uint32_t;
using MatInstanceID = uint32_t;

class MaterialManager
{
	MaterialManager (Resource::Material::Manager& mat_man, VulkanDevice& device, DescriptorManager& descriptor_man);

	stdext::flat_map<MatOutlineID, MatOutline> outlines;
	stdext::flat_map<MatInstanceID, MatOutline> outlines;
};