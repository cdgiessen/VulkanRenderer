#include "FrameData.h"

#include "rendering/backend/Device.h"

FrameData::FrameData (VulkanDevice& device)
: device (device),
  frame_data (device, uniform_details (sizeof (Data))),
  m_bindings ({ { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 0, 1 },
      { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 1, 1 } }),
  layout (device.device, m_bindings),
  descriptor_stack (layout),
  pool (device.device, layout.get (), m_bindings, 2),
  frame_descriptors ({ pool.allocate (), pool.allocate () })
{
}

void FrameData::update (double time) {}

void FrameData::bind (VkCommandBuffer buffer) {}
void FrameData::advance () {}

DescriptorStack const& FrameData::get_descriptor_stack () const { return descriptor_stack; }