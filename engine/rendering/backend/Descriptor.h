#pragma once

#include <vector>

#include <vulkan/vulkan.h>

enum class DescriptorType
{
	sampler,
	combined_image_sampler,
	sampled_image,
	storage_image,
	uniform_texel_buffer,
	storage_texel_buffer,
	uniform_buffer,
	storage_buffer,
	uniform_buffer_dynamic,
	storage_buffer_dynamic,
	input_attachment
};

enum ShaderStage
{
	vertex = 0x00000001,
	tess_control = 0x00000002,
	tess_eval = 0x00000004,
	geometry = 0x00000008,
	fragment = 0x00000010,
	compute = 0x00000020,
	all_graphics = 0x0000001F,
	all = 0x7FFFFFFF,
};

struct DescriptorSetLayoutBinding
{
	DescriptorType type;
	ShaderStage stages;
	uint32_t bind_point;
	uint32_t count;
};

bool operator== (DescriptorSetLayoutBinding const& a, DescriptorSetLayoutBinding const& b);

class DescriptorLayout
{
	public:
	DescriptorLayout (VkDevice device, std::vector<DescriptorSetLayoutBinding> const& layout_bindings);
	~DescriptorLayout ();
	DescriptorLayout (DescriptorLayout const& other) = delete;
	DescriptorLayout& operator= (DescriptorLayout const& other) = delete;
	DescriptorLayout (DescriptorLayout&& other) noexcept;
	DescriptorLayout& operator= (DescriptorLayout&& other) noexcept;

	VkDescriptorSetLayout Get () const;

	private:
	VkDevice device = VK_NULL_HANDLE;
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
};

VkDescriptorSetLayout CreateVkDescriptorSetLayout (
    VkDevice device, std::vector<DescriptorSetLayoutBinding> layout_bindings);

class DescriptorResource
{
	public:
	const int which_info;
	union
	{
		VkDescriptorBufferInfo buffer_info;
		VkDescriptorImageInfo image_info;
		VkBufferView buffer_view;
	} info;
	VkDescriptorType type = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;

	DescriptorResource (DescriptorType type, VkDescriptorBufferInfo buffer_info);
	DescriptorResource (DescriptorType type, VkDescriptorImageInfo image_info);
	DescriptorResource (DescriptorType type, VkBufferView view);
};

class DescriptorUse
{
	public:
	DescriptorUse (uint32_t bindPoint, uint32_t count, VkDescriptorType type, std::vector<VkDescriptorBufferInfo> buffer_infos);
	DescriptorUse (uint32_t bindPoint, uint32_t count, VkDescriptorType type, std::vector<VkDescriptorImageInfo> image_infos);
	DescriptorUse (uint32_t bindPoint, uint32_t count, VkDescriptorType type, std::vector<VkBufferView> texel_buffer_views);

	VkWriteDescriptorSet GetWriteDescriptorSet (VkDescriptorSet set);

	private:
	uint32_t bindPoint;
	uint32_t count;

	VkDescriptorType type = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
	enum class InfoType
	{
		buffer,
		image,
		texel_view
	} info_type;
	std::vector<VkDescriptorBufferInfo> buffer_infos;
	std::vector<VkDescriptorImageInfo> image_infos;
	std::vector<VkBufferView> texel_buffer_views;
};

class DescriptorSet
{
	public:
	DescriptorSet (VkDescriptorSet set, uint16_t pool_id);

	void Update (VkDevice device, std::vector<DescriptorUse> descriptors) const;

	void Bind (VkCommandBuffer cmdBuf, VkPipelineLayout layout, uint32_t location) const;

	VkDescriptorSet const& GetSet () const { return set; }
	uint16_t GetPoolID () const { return pool_id; }

	private:
	VkDescriptorSet set; // non owning
	uint16_t pool_id;
};


class DescriptorPool
{
	public:
	DescriptorPool (VkDevice device,
	    VkDescriptorSetLayout layout,
	    std::vector<DescriptorSetLayoutBinding> const& layout_bindings,
	    uint16_t max_sets);
	~DescriptorPool ();
	DescriptorPool (DescriptorPool const& other) = delete;
	DescriptorPool& operator= (DescriptorPool const& other) = delete;
	DescriptorPool (DescriptorPool&& other) noexcept;
	DescriptorPool& operator= (DescriptorPool&& other) noexcept;

	DescriptorSet Allocate ();
	void Free (DescriptorSet const& set);

	private:
	struct Pool
	{
		VkDescriptorPool pool;
		uint16_t allocated = 0;
		uint16_t max = 10;
		uint16_t id;
	};
	struct OptDescSet
	{
		bool is_valid = false;
		DescriptorSet set = { VK_NULL_HANDLE, 0 };
	};

	OptDescSet TryAllocate (Pool& pool);
	uint32_t AddNewPool ();

	VkDevice device;
	VkDescriptorSetLayout layout;
	uint16_t max_sets;
	std::vector<VkDescriptorPoolSize> pool_members;
	std::vector<Pool> pools;
};
