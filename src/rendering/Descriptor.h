#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

class VulkanDevice;

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

namespace std
{
template <> struct hash<DescriptorSetLayoutBinding>
{
	std::size_t operator() (DescriptorSetLayoutBinding const& s) const noexcept
	{
		std::size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.type));
		std::size_t h2 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.stages));
		std::size_t h3 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.bind_point));
		std::size_t h4 = std::hash<uint32_t>{}(static_cast<uint32_t> (s.count));
		return h1 ^ (h2 << 8) ^ (h3 << 16) ^ (h4 << 24);
	}
};
} // namespace std

using DescriptorLayout = std::vector<DescriptorSetLayoutBinding>;

VkDescriptorSetLayout CreateVkDescriptorSetLayout (VkDevice device, DescriptorLayout layout_desc);

namespace std
{
template <> struct hash<DescriptorLayout>
{
	std::size_t operator() (DescriptorLayout const& s) const noexcept
	{
		int i = 0;
		std::size_t out;
		for (auto& binding : s)
		{
			out ^= (std::hash<DescriptorSetLayoutBinding>{}(binding) << i++);
		}
		return out;
	}
};
} // namespace std

class DescriptorResource
{
	public:
	VkDescriptorBufferInfo buffer_info;
	VkDescriptorImageInfo image_info;
	VkDescriptorType type = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;

	DescriptorResource (VkDescriptorType type, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	DescriptorResource (VkDescriptorType type, VkSampler sampler, VkImageView imageView, VkImageLayout layout);
};

class DescriptorUse
{
	public:
	DescriptorUse (uint32_t bindPoint, uint32_t count, DescriptorResource resource);

	VkWriteDescriptorSet GetWriteDescriptorSet (VkDescriptorSet set);

	private:
	uint32_t bindPoint;
	uint32_t count;

	DescriptorResource resource;
};
using LayoutID = uint16_t; // also indexes for the pool
using PoolID = uint16_t;   // which vkPool to index in the pool
class DescriptorSet
{
	public:
	DescriptorSet (VkDescriptorSet set, LayoutID layout_id, PoolID pool_id);

	void Update (VkDevice device, std::vector<DescriptorUse> descriptors) const;

	void Bind (VkCommandBuffer cmdBuf, VkPipelineLayout layout, uint32_t location) const;

	VkDescriptorSet const& GetSet () const { return set; }
	LayoutID GetLayoutID () const { return layout_id; }
	PoolID GetPoolID () const { return pool_id; }

	private:
	VkDescriptorSet set; // non owning
	LayoutID layout_id;
	PoolID pool_id;
};


class DescriptorPool
{
	public:
	DescriptorPool (
	    VkDevice device, DescriptorLayout const& layout, VkDescriptorSetLayout vk_layout, LayoutID layout_id, uint32_t max_sets);
	~DescriptorPool ();
	DescriptorPool (DescriptorPool const& other) = delete;
	DescriptorPool& operator= (DescriptorPool const& other) = delete;
	DescriptorPool (DescriptorPool&& other);
	DescriptorPool& operator= (DescriptorPool&& other);

	DescriptorSet Allocate ();
	void Free (DescriptorSet const& set);

	private:
	struct Pool
	{
		VkDescriptorPool pool;
		uint16_t allocated = 0;
		uint16_t max = 10;
		PoolID id;
	};
	struct OptDescSet
	{
		bool is_valid = false;
		DescriptorSet set = { VK_NULL_HANDLE, 0, 0 };
	};

	OptDescSet TryAllocate (Pool& pool);
	uint32_t AddNewPool ();

	std::mutex lock;
	VkDevice device;
	VkDescriptorSetLayout vk_layout;
	LayoutID layout_id;
	uint32_t max_sets;
	std::vector<VkDescriptorPoolSize> pool_members;
	std::vector<Pool> pools;
};

class DescriptorManager
{

	public:
	DescriptorManager (VulkanDevice& device);
	~DescriptorManager ();

	LayoutID CreateDescriptorSetLayout (DescriptorLayout layout);
	void DestroyDescriptorSetLayout (LayoutID id);

	VkDescriptorSetLayout GetLayout (LayoutID id);

	DescriptorSet CreateDescriptorSet (LayoutID layout);
	void DestroyDescriptorSet (DescriptorSet const& set);

	private:
	std::mutex lock;
	VulkanDevice& device;
	LayoutID cur_id = 0;
	std::unordered_map<DescriptorLayout, LayoutID> layout_descriptions;
	std::unordered_map<LayoutID, VkDescriptorSetLayout> layouts;
	std::unordered_map<LayoutID, DescriptorPool> pools;
};