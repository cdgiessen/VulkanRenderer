#pragma once
#include <array>

#include "cml/cml.h"

#include "rendering/backend/Buffer.h"
#include "rendering/backend/Descriptor.h"

class FrameData
{
	public:
	FrameData (VulkanDevice& device);

	void Update (double time);

	void Bind (VkCommandBuffer buffer);
	void Advance ();

	DescriptorStack const& GetDescriptorStack () const;

	private:
	VulkanDevice& device;
	DoubleBuffer frame_data;

	int cur_index = 0;
	std::vector<DescriptorSetLayoutBinding> m_bindings;
	DescriptorLayout layout;
	DescriptorStack descriptor_stack;
	DescriptorPool pool;
	std::array<DescriptorSet, 2> frame_descriptors;

	VkPipelineLayout frame_layout;

	struct Data
	{
		float cur_time = 0.f;
		float delta_time = 0.f;
		uint32_t frame_index = 0;
	} current_data;
};