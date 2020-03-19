#include "FrameData.h"

#include "rendering/backend/Device.h"

FrameData::FrameData (VulkanDevice& device)
: device (device),
  frame_data (device, uniform_details (sizeof (Data))),
  m_bindings ({ { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 0, 1 },
      { DescriptorType::uniform_buffer, ShaderStage::all_graphics, 1, 1 } }),
  layout (device.device, m_bindings),
  pool (device.device, layout.Get (), m_bindings, 2),
  frame_descriptors ({ pool.Allocate (), pool.Allocate () })
{
}

void FrameData::Update (double time) {}

void FrameData::Bind (VkCommandBuffer buffer) {}
void FrameData::Advance () {}
