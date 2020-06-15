#include "Lighting.h"

#include "rendering/backend/Device.h"

Lighting::Lighting (VulkanDevice& device, Textures& textures, FrameData& frame_data)
: device (device),
  directional_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (DirectionalLight))),
  point_gpu_data (device, uniform_array_details (MaxPointLightCount, sizeof (PointLight))),
  spot_gpu_data (device, uniform_array_details (MaxSpotLightCount, sizeof (SpotLight))),
  m_bindings ({ { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 0, MaxDirectionalLightCount },
      { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 1, MaxPointLightCount },
      { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 2, MaxSpotLightCount } }),
  layout (device.device, m_bindings),
  descriptor_stack (layout, frame_data.get_descriptor_stack ()),
  pool (device.device, layout.get (), m_bindings, 2),
  lighting_descriptors ({ pool.allocate (), pool.allocate () }),
  pipeline_layout (device.device, descriptor_stack.get_layouts (), {})
{
	for (int i = 0; i < lighting_descriptors.size (); i++)
	{
		std::vector<VkDescriptorBufferInfo> dir_buf;
		for (int j = 0; j < MaxDirectionalLightCount; j++)
			dir_buf.push_back (directional_gpu_data.get_descriptor_info (i, j));
		std::vector<VkDescriptorBufferInfo> point_buf;
		for (int j = 0; j < MaxPointLightCount; j++)
			point_buf.push_back (point_gpu_data.get_descriptor_info (i, j));
		std::vector<VkDescriptorBufferInfo> spot_buf;
		for (int j = 0; j < MaxSpotLightCount; j++)
			spot_buf.push_back (spot_gpu_data.get_descriptor_info (i, j));

		std::vector<DescriptorUse> writes_0 = {
			{ 0, MaxDirectionalLightCount, directional_gpu_data.get_descriptor_type (), dir_buf },
			{ 0, MaxPointLightCount, point_gpu_data.get_descriptor_type (), point_buf },
			{ 0, MaxSpotLightCount, spot_gpu_data.get_descriptor_type (), spot_buf }
		};
		lighting_descriptors[i].update (device.device, writes_0);
	}
}


void Lighting::update (std::vector<DirectionalLight> directional_lights,
    std::vector<PointLight> point_lights,
    std::vector<SpotLight> spot_lights)
{
	directional_gpu_data.Write ().copy_to_buffer (directional_lights);
	point_gpu_data.Write ().copy_to_buffer (point_lights);
	spot_gpu_data.Write ().copy_to_buffer (spot_lights);
	directional_gpu_data.advance ();
	point_gpu_data.advance ();
	spot_gpu_data.advance ();
}
void Lighting::bind (VkCommandBuffer buffer)
{
	lighting_descriptors.at (cur_index).bind (buffer, pipeline_layout.get (), 1);
}

void Lighting::advance () { cur_index = (cur_index + 1) % 2; }


DescriptorStack const& Lighting::get_descriptor_stack () const { return descriptor_stack; }
