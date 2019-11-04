#include "Descriptor.h"

#include "Device.h"
#include "rendering/Initializers.h"

bool operator== (DescriptorSetLayoutBinding const& a, DescriptorSetLayoutBinding const& b)
{
	return a.type == b.type && a.stage == b.stage && a.bind_point == b.bind_point && a.count == b.count;
}


//// DESCRIPTOR RESOURCE ////

DescriptorResource::DescriptorResource (VkDescriptorType type, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
: type (type)
{
	buffer_info.buffer = buffer;
	buffer_info.offset = offset;
	buffer_info.range = range;
}
DescriptorResource::DescriptorResource (
    VkDescriptorType type, VkSampler sampler, VkImageView imageView, VkImageLayout layout)
: type (type)
{
	image_info.sampler = sampler;
	image_info.imageView = imageView;
	image_info.imageLayout = layout;
}

//// DESCRIPTOR USE ////

DescriptorUse::DescriptorUse (uint32_t bindPoint, uint32_t count, DescriptorResource resource)
: bindPoint (bindPoint), count (count), resource (resource)
{
}

VkWriteDescriptorSet DescriptorUse::GetWriteDescriptorSet (VkDescriptorSet set)
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = set;
	writeDescriptorSet.descriptorType = resource.type;
	writeDescriptorSet.dstBinding = bindPoint;
	writeDescriptorSet.descriptorCount = count;

	if (resource.buffer_info.buffer != VK_NULL_HANDLE)
		writeDescriptorSet.pBufferInfo = &resource.buffer_info;
	else
		writeDescriptorSet.pImageInfo = &resource.image_info;

	return writeDescriptorSet;
}


//// DESCRIPTOR LAYOUT ////

VkDescriptorSetLayout CreateVkDescriptorSetLayout (VkDevice device, DescriptorLayout layout_desc)
{
	std::vector<VkDescriptorSetLayoutBinding> vk_bindings;
	for (auto& b : layout_desc.bindings)
	{
		vk_bindings.push_back (initializers::descriptorSetLayoutBinding (
		    static_cast<VkDescriptorType> (b.type), b.stage, b.bind_point, b.count));
	}
	VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo (vk_bindings);
	VkDescriptorSetLayout layout;
	if (vkCreateDescriptorSetLayout (device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create descriptor set layout!");
	}
	return layout;
}

void FreeVkDescriptorSetLayout (VkDevice device, VkDescriptorSetLayout layout)
{
	vkDestroyDescriptorSetLayout (device, layout, nullptr);
}

bool operator== (DescriptorLayout const& a, DescriptorLayout const& b)
{
	return a.bindings == b.bindings;
}


//// DESCRIPTOR SET ////

DescriptorSet::DescriptorSet (VkDescriptorSet set, LayoutID layout_id, PoolID pool_id)
: set (set), layout_id (layout_id), pool_id (pool_id)
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

DescriptorPool::DescriptorPool (
    VkDevice device, DescriptorLayout const& layout, VkDescriptorSetLayout vk_layout, LayoutID layout_id, uint32_t max_sets)
: device (device), vk_layout (vk_layout), layout_id (layout_id), max_sets (max_sets)
{
	std::unordered_map<DescriptorType, uint32_t> layout_type_count;
	for (auto& binding : layout.bindings)
	{
		layout_type_count[binding.type] += binding.count;
	}
	for (auto& [type, count] : layout_type_count)
	{
		pool_members.push_back (initializers::descriptorPoolSize (static_cast<VkDescriptorType> (type), count));
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

DescriptorPool::DescriptorPool (DescriptorPool&& other)
: device (device),
  vk_layout (vk_layout),
  layout_id (layout_id),
  max_sets (max_sets),
  pool_members (pool_members),
  pools (pools)
{
	other.pools.clear ();
}
DescriptorPool& DescriptorPool::operator= (DescriptorPool&& other)
{
	device = other.device;
	vk_layout = other.vk_layout;
	layout_id = other.layout_id;
	max_sets = other.max_sets;
	pool_members = other.pool_members;
	pools = other.pools;
	other.pools.clear ();
	return *this;
}

DescriptorSet DescriptorPool::Allocate ()
{
	std::lock_guard lg (lock);
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
	std::lock_guard lg (lock);
	if (pools.size () > set.GetPoolID ()) // make sure PoolID is valid
	{
		vkFreeDescriptorSets (device, pools.at (set.GetPoolID ()).pool, 1, &set.GetSet ());
	}
}

uint32_t DescriptorPool::AddNewPool ()
{
	VkDescriptorPool pool;

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo (
	    static_cast<uint32_t> (pool_members.size ()), pool_members.data (), max_sets);

	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool (device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create descriptor pool!");
	}
	Pool p = { pool, 0, max_sets, static_cast<uint16_t> (pools.size ()) };
	pools.push_back (p);
	return pools.size () - 1; // newest index
}

DescriptorPool::OptDescSet DescriptorPool::TryAllocate (Pool& pool)
{
	VkDescriptorSet set;
	VkDescriptorSetLayout layouts[] = { vk_layout };
	VkDescriptorSetAllocateInfo allocInfo = initializers::descriptorSetAllocateInfo (pool.pool, layouts, 1);

	VkResult res = vkAllocateDescriptorSets (device, &allocInfo, &set);
	if (res == VK_SUCCESS)
	{
		pool.allocated++;
		return { true, DescriptorSet (set, layout_id, pool.id) };
	}
	else if (res == VK_ERROR_FRAGMENTED_POOL || res == VK_ERROR_OUT_OF_POOL_MEMORY)
	{
		return { false, DescriptorSet (nullptr, 0, 0) }; // need make a new pool
	}

	else if (res == VK_ERROR_OUT_OF_HOST_MEMORY)
		throw std::runtime_error ("failed to allocate descriptor set! OUT_OF_HOST_MEMORY");
	else if (res == VK_ERROR_OUT_OF_DEVICE_MEMORY)
		throw std::runtime_error ("failed to allocate descriptor set! OUT_OF_DEVICE_MEMORY");

	throw std::runtime_error ("failed to allocate descriptor set! OUT_OF_DEVICE_MEMORY");
}

//// DESCRIPTOR MANAGER ////

DescriptorManager::DescriptorManager (VulkanDevice& device) : device (device) {}

LayoutID DescriptorManager::CreateDescriptorSetLayout (DescriptorLayout layout_desc)
{
	std::lock_guard lg (lock);
	if (layout_descriptions.count (layout_desc) == 0)
	{
		LayoutID new_id = cur_id++;
		VkDescriptorSetLayout layout = CreateVkDescriptorSetLayout (device.device, layout_desc);
		layout_descriptions[layout_desc] = new_id;
		layouts[new_id] = layout;
		pools.emplace (new_id, std::move (DescriptorPool (device.device, layout_desc, layout, new_id, 10)));
		return new_id;
	}
	else
		return layout_descriptions.at (layout_desc);
}

void DescriptorManager::DestroyDescriptorSetLayout (LayoutID id)
{
	std::lock_guard lg (lock);
	FreeVkDescriptorSetLayout (device.device, layouts.at (id));
	layouts.erase (id);
	pools.erase (id);
}

VkDescriptorSetLayout DescriptorManager::GetLayout (LayoutID id)
{
	std::lock_guard lg (lock);
	return layouts.at (id);
}

DescriptorSet DescriptorManager::CreateDescriptorSet (LayoutID id)
{
	std::lock_guard lg (lock);
	return pools.at (id).Allocate ();
}
void DescriptorManager::DestroyDescriptorSet (DescriptorSet const& set)
{
	std::lock_guard lg (lock);
	pools.at (set.GetLayoutID ()).Free (set);
}
