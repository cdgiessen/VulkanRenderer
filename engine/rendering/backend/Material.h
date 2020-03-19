#pragma once

#include "SG14/flat_map.h"

#include "resources/Material.h"

#include "Descriptor.h"

#include "rendering/backend/Buffer.h"

class VulkanDevice;

using MatOutlineID = uint32_t;
using MatInstanceID = uint32_t;

class MatInstance
{
	public:
	MatInstance ();

	MatInstanceID CreateInstance ();
	void RemoveInstance (MatInstanceID id);

	void UpdateMaterialData (MatInstanceID id, int location, Resource::Material::DataMember data);

	void UpdateTexture (MatInstanceID id, int location, VkDescriptorImageInfo info);

	void GpuUpdateBuffer ();

	private:
	MatOutlineID cur_instance = 0;
	stdext::flat_map<MatInstanceID, DescriptorSet> instance_sets;
	std::vector<DoubleBuffer> instance_data;
	std::vector<int> offset_index;
};

class MaterialManager
{
	public:
	MaterialManager (Resource::Material::Manager& mat_man, VulkanDevice& device);

	MatOutlineID CreateMatOutline (Resource::Material::MaterialOutline const& outline);
	MatInstanceID CreateMatInstance (MatOutlineID id, Resource::Material::MaterialInstance const& instance);

	void DestroyMatOutline (MatOutlineID id);
	void DestroyMatInstance (MatInstanceID id);

	private:
	MatOutlineID cur_outline = 0;
	stdext::flat_map<MatOutlineID, MatInstance> outlines;
};