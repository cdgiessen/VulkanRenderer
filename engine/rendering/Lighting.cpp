#include "Lighting.h"

#include "rendering/backend/Buffer.h"
#include "rendering/backend/Descriptor.h"
#include "rendering/backend/Device.h"

Lighting::Lighting (VulkanDevice& device, Textures& textures)
: device (device),
  directional_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (DirectionalLight))),
  point_gpu_data (device, uniform_array_details (MaxPointLightCount, sizeof (PointLight))),
  spot_gpu_data (device, uniform_array_details (MaxSpotLightCount, sizeof (SpotLight))),
  m_bindings ({ { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 0, MaxDirectionalLightCount },
      { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 1, MaxPointLightCount },
      { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 2, MaxSpotLightCount } }),
  layout (device.device, m_bindings),
  pool (device.device, layout.Get (), m_bindings, 2),
  lighting_descriptors ({ pool.Allocate (), pool.Allocate () })
{
	for (int i = 0; i < lighting_descriptors.size (); i++)
	{
		std::vector<VkDescriptorBufferInfo> dir_buf;
		for (int j = 0; j < MaxDirectionalLightCount; j++)
			dir_buf.push_back (directional_gpu_data.GetDescriptorInfo (i, j));
		std::vector<VkDescriptorBufferInfo> point_buf;
		for (int j = 0; j < MaxPointLightCount; j++)
			point_buf.push_back (point_gpu_data.GetDescriptorInfo (i, j));
		std::vector<VkDescriptorBufferInfo> spot_buf;
		for (int j = 0; j < MaxSpotLightCount; j++)
			spot_buf.push_back (spot_gpu_data.GetDescriptorInfo (i, j));

		std::vector<DescriptorUse> writes_0 = {
			{ 0, MaxDirectionalLightCount, directional_gpu_data.GetDescriptorType (), dir_buf },
			{ 0, MaxPointLightCount, point_gpu_data.GetDescriptorType (), point_buf },
			{ 0, MaxSpotLightCount, spot_gpu_data.GetDescriptorType (), spot_buf }
		};
		lighting_descriptors[i].Update (device.device, writes_0);
	}
}


void Lighting::Update (std::vector<DirectionalLight> directional_lights,
    std::vector<PointLight> point_lights,
    std::vector<SpotLight> spot_lights)
{
	directional_gpu_data.Write ().CopyToBuffer (directional_lights);
	point_gpu_data.Write ().CopyToBuffer (point_lights);
	spot_gpu_data.Write ().CopyToBuffer (spot_lights);
	directional_gpu_data.Advance ();
	point_gpu_data.Advance ();
	spot_gpu_data.Advance ();
}
void Lighting::Bind (VkCommandBuffer buffer)
{
	lighting_descriptors.at (cur_index).Bind (buffer, lighting_layout, 1);
}

void Lighting::Advance () { cur_index = (cur_index + 1) % 2; }