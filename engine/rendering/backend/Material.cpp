#include "Material.h"

#include "Device.h"

MatOutlineID Materials::create_material_outline (Resource::Material::MaterialOutline const& outline)
{
	// DescriptorLayout layout;
	// layout.push_back ({ DescriptorType::uniform_buffer, ShaderStage::vertex |
	// ShaderStage::fragment, 0, 1 }); layout.push_back ({ DescriptorType::combined_image_sampler,
	//    ShaderStage::vertex | ShaderStage::fragment,
	//    1,
	//    outline.texture_members.size () });
	// layout = descriptor_man.CreateDescriptorSetLayout (layout);
	return 0;
}

MatInstanceID Materials::create_material_instance (
    MatOutlineID id, Resource::Material::MaterialInstance const& instance)

{
	// set = descriptor_man.CreateDescriptorSet (outline.layout);

	return 0;
}

//// MATERIALS ////

Materials::Materials (Resource::Material::Materials& materials, VulkanDevice& device) {}