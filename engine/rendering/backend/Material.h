#pragma once

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

	MatInstanceID create_instance ();
	void remove_instance (MatInstanceID id);

	void update_material_data (MatInstanceID id, int location, Resource::Material::DataMember data);

	void update_texture (MatInstanceID id, int location, VkDescriptorImageInfo info);

	void gpu_update_buffer ();

	private:
	MatOutlineID cur_instance = 0;
	std::unordered_map<MatInstanceID, DescriptorSet> instance_sets;
	std::vector<DoubleBuffer> instance_data;
	std::vector<int> offset_index;
};

class Materials
{
	public:
	Materials (Resource::Material::Materials& materials, VulkanDevice& device);

	MatOutlineID create_material_outline (Resource::Material::MaterialOutline const& outline);
	MatInstanceID create_material_instance (MatOutlineID id, Resource::Material::MaterialInstance const& instance);

	void destroy_material_outline (MatOutlineID id);
	void destroy_material_instance (MatInstanceID id);

	private:
	MatOutlineID cur_outline = 0;
	std::unordered_map<MatOutlineID, MatInstance> outlines;
};