#include "Descriptor.h"

#include "Device.h"
#include "rendering/Initializers.h"

bool operator== (DescriptorSetLayoutBinding const& a, DescriptorSetLayoutBinding const& b)
{
	return a.type == b.type && a.stages == b.stages && a.bind_point == b.bind_point && a.count == b.count;
}

//// DESCRIPTOR LAYOUT ////

DescriptorLayout::DescriptorLayout (VkDevice device, std::vector<DescriptorSetLayoutBinding> const& layout_bindings)
{
	std::vector<VkDescriptorSetLayoutBinding> vk_bindings;
	for (auto& b : layout_bindings)
	{
		vk_bindings.push_back (initializers::descriptorSetLayoutBinding (
		    static_cast<VkDescriptorType> (b.type), static_cast<VkShaderStageFlags> (b.stages), b.bind_point, b.count));
	}
	VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo (vk_bindings);
	if (vkCreateDescriptorSetLayout (device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create descriptor set layout!");
	}
}
DescriptorLayout::~DescriptorLayout () { vkDestroyDescriptorSetLayout (device, layout, nullptr); }

DescriptorLayout::DescriptorLayout (DescriptorLayout&& other) noexcept
: device (other.device), layout (other.layout)
{
	other.layout = VK_NULL_HANDLE;
}
DescriptorLayout& DescriptorLayout::operator= (DescriptorLayout&& other) noexcept
{
	device = other.device;
	layout = other.layout;
	other.layout = VK_NULL_HANDLE;
	return *this;
}

VkDescriptorSetLayout DescriptorLayout::Get () const { return layout; }


//// DESCRIPTOR RESOURCE ////

DescriptorResource::DescriptorResource (DescriptorType type, VkDescriptorBufferInfo buffer_info)
: which_info (0), type (static_cast<VkDescriptorType> (type))
{
	info.buffer_info = buffer_info;
}
DescriptorResource::DescriptorResource (DescriptorType type, VkDescriptorImageInfo image_info)
: which_info (1), type (static_cast<VkDescriptorType> (type))
{
	info.image_info = image_info;
}
DescriptorResource::DescriptorResource (DescriptorType type, VkBufferView texel_buffer_view)
: which_info (2), type (static_cast<VkDescriptorType> (type))
{
	info.buffer_view = texel_buffer_view;
}


//// DESCRIPTOR USE ////

DescriptorUse::DescriptorUse (
    uint32_t bindPoint, uint32_t count, VkDescriptorType type, std::vector<VkDescriptorBufferInfo> buffer_infos)
: bindPoint (bindPoint), count (count), type (type), info_type (InfoType::buffer), buffer_infos (buffer_infos)
{
}

DescriptorUse::DescriptorUse (
    uint32_t bindPoint, uint32_t count, VkDescriptorType type, std::vector<VkDescriptorImageInfo> image_infos)
: bindPoint (bindPoint), count (count), type (type), info_type (InfoType::image), image_infos (image_infos)
{
}

DescriptorUse::DescriptorUse (
    uint32_t bindPoint, uint32_t count, VkDescriptorType type, std::vector<VkBufferView> texel_buffer_views)
: bindPoint (bindPoint), count (count), type (type), info_type (InfoType::texel_view), texel_buffer_views (texel_buffer_views)
{
}

VkWriteDescriptorSet DescriptorUse::GetWriteDescriptorSet (VkDescriptorSet set)
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = set;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = bindPoint;
	writeDescriptorSet.descriptorCount = count;
	if (info_type == InfoType::buffer)
		writeDescriptorSet.pBufferInfo = buffer_infos.data ();
	else if (info_type == InfoType::image)
		writeDescriptorSet.pImageInfo = image_infos.data ();
	else if (info_type == InfoType::texel_view)
		writeDescriptorSet.pTexelBufferView = texel_buffer_views.data ();
	return writeDescriptorSet;
}

//// DESCRIPTOR SET ////

DescriptorSet::DescriptorSet (VkDescriptorSet set, uint16_t pool_id) : set (set), pool_id (pool_id)
{
}

void DescriptorSet::Update (VkDevice device, std::vector<DescriptorUse> descriptors) const
{

	std::vector<VkWriteDescriptorSet> writes;
	for (auto& descriptor : descriptors)
	{
		writes.push_back (descriptor.GetWriteDescriptorSet (set));
	}

	vkUpdateDescriptorSets (device, static_cast<uint32_t> (writes.size ()), writes.data (), 0, nullptr);
}


void DescriptorSet::Bind (VkCommandBuffer cmdBuf, VkPipelineLayout layout, uint32_t location) const
{
	vkCmdBindDescriptorSets (cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, location, 1, &set, 0, nullptr);
}


//// DESCRIPTOR POOL ////

DescriptorPool::DescriptorPool (VkDevice device,
    VkDescriptorSetLayout layout,
    std::vector<DescriptorSetLayoutBinding> const& layout_bindings,
    uint16_t max_sets)
: device (device), layout (layout), max_sets (max_sets)
{
	std::unordered_map<DescriptorType, uint32_t> layout_type_count;
	for (auto& binding : layout_bindings)
	{
		layout_type_count[binding.type] += binding.count;
	}
	for (auto& [type, count] : layout_type_count)
	{
		pool_members.push_back (
		    initializers::descriptorPoolSize (static_cast<VkDescriptorType> (type), count * max_sets));
	}

	AddNewPool ();
}
DescriptorPool::~DescriptorPool ()
{
	for (auto& pool : pools)
	{
		vkDestroyDescriptorPool (device, pool.pool, nullptr);
	}
}

DescriptorPool::DescriptorPool (DescriptorPool&& other) noexcept
: device (other.device),
  layout (other.layout),
  max_sets (other.max_sets),
  pool_members (other.pool_members),
  pools (other.pools)
{
	other.pools.clear ();
}
DescriptorPool& DescriptorPool::operator= (DescriptorPool&& other) noexcept
{
	device = other.device;
	layout = other.layout;
	max_sets = other.max_sets;
	pool_members = other.pool_members;
	pools = other.pools;
	other.pools.clear ();
	return *this;
}

DescriptorSet DescriptorPool::Allocate ()
{
	for (int i = 0; i < pools.size (); i++)
	{
		if (pools.at (i).allocated < pools.at (i).max)
		{
			auto set = TryAllocate (pools.at (i));
			if (set.is_valid) return set.set;
		}
	}
	// assume all pools are full
	auto new_pool_id = AddNewPool ();
	auto set = TryAllocate (pools.at (new_pool_id));
	while (!set.is_valid)
	{
		new_pool_id = AddNewPool ();
		set = TryAllocate (pools.at (new_pool_id));
	}
	return set.set;
}

void DescriptorPool::Free (DescriptorSet const& set)
{
	if (pools.size () > set.GetPoolID ()) // make sure PoolID is valid
	{
		vkFreeDescriptorSets (device, pools.at (set.GetPoolID ()).pool, 1, &set.GetSet ());
	}
}

uint32_t DescriptorPool::AddNewPool ()
{
	VkDescriptorPool pool;

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo (pool_members, max_sets);

	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool (device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create descriptor pool!");
	}
	Pool p = { pool, 0, max_sets, static_cast<uint16_t> (pools.size ()) };
	pools.push_back (p);
	return static_cast<uint32_t> (pools.size ()) - 1; // newest index
}

DescriptorPool::OptDescSet DescriptorPool::TryAllocate (Pool& pool)
{
	VkDescriptorSet set;
	std::vector<VkDescriptorSetLayout> layouts = { layout };
	VkDescriptorSetAllocateInfo allocInfo = initializers::descriptorSetAllocateInfo (pool.pool, layouts);

	VkResult res = vkAllocateDescriptorSets (device, &allocInfo, &set);
	if (res == VK_SUCCESS)
	{
		pool.allocated++;
		return { true, DescriptorSet (set, pool.id) };
	}
	else if (res == VK_ERROR_FRAGMENTED_POOL || res == VK_ERROR_OUT_OF_POOL_MEMORY)
	{
		return { false, DescriptorSet (nullptr, 0) }; // need make a new pool
	}

	else if (res == VK_ERROR_OUT_OF_HOST_MEMORY)
		throw std::runtime_error ("failed to allocate descriptor set! OUT_OF_HOST_MEMORY");
	else if (res == VK_ERROR_OUT_OF_DEVICE_MEMORY)
		throw std::runtime_error ("failed to allocate descriptor set! OUT_OF_DEVICE_MEMORY");

	throw std::runtime_error ("failed to allocate descriptor set! OUT_OF_DEVICE_MEMORY");
}