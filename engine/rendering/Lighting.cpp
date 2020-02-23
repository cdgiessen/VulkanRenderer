#include "Lighting.h"

#include "rendering/backend/Buffer.h"
#include "rendering/backend/Descriptor.h"
#include "rendering/backend/Device.h"

LightingManager::LightingManager (
    VulkanDevice& device, DescriptorManager& descriptor_manager, TextureManager& texture_manager)
: device (device),
  directional_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (DirectionalLight))),
  point_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (PointLight))),
  spot_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (SpotLight)))
{
	std::vector<DescriptorSetLayoutBinding> m_bindings = {
		{ DescriptorType::uniform_buffer, ShaderStage::all_graphics, 0, MaxDirectionalLightCount },
		{ DescriptorType::uniform_buffer, ShaderStage::all_graphics, 1, MaxPointLightCount },
		{ DescriptorType::uniform_buffer, ShaderStage::all_graphics, 2, MaxSpotLightCount }
	};
	auto descriptor_layout = descriptor_manager.CreateDescriptorSetLayout (m_bindings);

	for (int i = 0; i < lighting_descriptors.size (); i++)
	{
		lighting_descriptors[i] = descriptor_manager.CreateDescriptorSet (descriptor_layout);

		std::vector<DescriptorUse> writes_0 = { { 0,
			                                        MaxDirectionalLightCount,
			                                        directional_gpu_data.GetDescriptorType (),
			                                        { directional_gpu_data.GetDescriptorInfo (i) } },
			{ 0, MaxPointLightCount, point_gpu_data.GetDescriptorType (), { point_gpu_data.GetDescriptorInfo (i) } },
			{ 0, MaxSpotLightCount, spot_gpu_data.GetDescriptorType (), { spot_gpu_data.GetDescriptorInfo (i) } } };
		lighting_descriptors[i].Update (device.device, writes_0);
	}
}


void LightingManager::Update (std::vector<DirectionalLight> directional_lights,
    std::vector<PointLight> point_lights,
    std::vector<SpotLight> spot_lights)
{
	directional_gpu_data.Write ().CopyToBuffer (directional_lights);
	point_gpu_data.Write ().CopyToBuffer (point_lights);
	spot_gpu_data.Write ().CopyToBuffer (spot_lights);
	directional_gpu_data.Advance ();
	point_gpu_data.Advance ();
	spot_gpu_data.Advance ();
	cur_index = (cur_index + 1) % 2;
}
void LightingManager::Bind (VkCommandBuffer buffer, VkPipelineLayout layout)
{
	lighting_descriptors.at (cur_index).Bind (buffer, layout, 1)
}