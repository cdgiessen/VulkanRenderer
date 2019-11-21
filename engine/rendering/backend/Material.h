#pragma once

#include "SG14/flat_map.h"

#include "resources/Material.h"

#include "Descriptor.h"

#include "rendering/backend/Buffer.h"

class VulkanDevice;

using MatOutlineID = uint32_t;
using MatInstanceID = uint32_t;

class MaterialManager
{
	public:
	MaterialManager (Resource::Material::Manager& mat_man, VulkanDevice& device, DescriptorManager& descriptor_man);

	MatOutlineID CreateMatOutline (Resource::Material::MaterialOutline const& outline);
	MatInstanceID CreateMatInstance (MatOutlineID id, Resource::Material::MaterialInstance const& instance);

	void CreateMatOutline (MatOutlineID id);
	void CreateMatInstance (MatInstanceID id);

	private:
	MatOutlineID cur_outline = 0;
	MatOutlineID cur_outline = 0;
	stdext::flat_map<MatOutlineID, LayoutID> outlines;
	stdext::flat_map<MatInstanceID, DescriptorSet> instance_sets;

	std::vector<DoubleBuffer> instance_data;
};