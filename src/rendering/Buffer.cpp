#include "Buffer.h"

#include "Device.h"


const uint32_t VERTEX_BUFFER_BIND_ID = 0;
const uint32_t INSTANCE_BUFFER_BIND_ID = 1;

VulkanBuffer::VulkanBuffer(VulkanDevice& device)
	: device(&device), resource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
}

VulkanBuffer::VulkanBuffer(VulkanDevice& device, VkDescriptorType type)
	: device(&device), resource(type) {
}

void VulkanBuffer::CleanBuffer() {
	if (created) {
		if (persistantlyMapped) {
			Unmap();
		}

		vmaDestroyBuffer(buffer.allocator, buffer.buffer, buffer.allocation);
		//device->DestroyVmaAllocatedBuffer(buffer);

		created = false;
	}
	else {
		//Log::Debug << "Cleaned up buffer!\n";
	}
}

VulkanBuffer::~VulkanBuffer() {
	//CleanBuffer();
}

void VulkanBuffer::SetupBuffer(VkDeviceSize bufferSize,
	VkBufferUsageFlags bufferUsage, VmaMemoryUsage allocUsage,
	VmaAllocationCreateFlags allocFlags, void* memToCopy)
{

	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = bufferSize;
	bufferInfo.usage = bufferUsage;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = allocUsage;
	allocInfo.flags = allocFlags;

	buffer.allocator = device->GetGeneralAllocator();
	//Log::Debug << buffer.allocator << "\n";
	if (buffer.allocator == nullptr)
		throw std::runtime_error("Allocator was null!");
	VK_CHECK_RESULT(vmaCreateBuffer(buffer.allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocationInfo));

	if (memToCopy != nullptr) {
		memcpy(buffer.allocationInfo.pMappedData, memToCopy, bufferSize);
	}
}

void VulkanBuffer::Map(void** pData) {
	vmaMapMemory(buffer.allocator, buffer.allocation, pData);
	//device->VmaMapMemory(buffer, &mapped);
}
void VulkanBuffer::Unmap() {
	vmaUnmapMemory(buffer.allocator, buffer.allocation);
	//device->VmaUnmapMemory(buffer);
}

void VulkanBuffer::Flush() {
	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(buffer.allocator, buffer.allocationInfo.memoryType, &memFlags);
	if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
		VkMappedMemoryRange memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		memRange.memory = buffer.allocationInfo.deviceMemory;
		memRange.offset = buffer.allocationInfo.offset;
		memRange.size = buffer.allocationInfo.size;
		vkFlushMappedMemoryRanges(device->device, 1, &memRange);
	}

	//device->FlushBuffer(buffer);
}

void VulkanBuffer::SetupResource() {
	resource.FillResource(buffer.buffer, 0, m_size);
}

VkDeviceSize VulkanBuffer::Size() const {
	return m_size;
}

bool VulkanBuffer::IsCreated() const {
	return created;
}

void AlignedMemcpy(uint8_t bytes, VkDeviceSize destMemAlignment, void* src, void* dst) {
	int src_offset = 0;
	int dest_offset = 0;
	for (int i = 0; i < bytes; i++) {

		memcpy((char*)dst + dest_offset,
			(char*)src + src_offset,
			sizeof(bytes));
		src_offset += 1;
		dest_offset += (int)destMemAlignment;

	}
}

void VulkanBuffer::CopyToBuffer(void* pData, VkDeviceSize size)
{
	if (persistantlyMapped) {
		memcpy(mapped, pData, (size_t)size);

	}
	else {
		this->Map(&mapped);
		memcpy(mapped, pData, (size_t)size);

		//VkDeviceSize bufAlignment = device->physical_device_properties.limits.minUniformBufferOffsetAlignment;

		//AlignedMemcpy((size_t)size, bufAlignment, pData, mapped);

		this->Unmap();
	}
}

VulkanBufferUniform::VulkanBufferUniform(VulkanDevice& device) : VulkanBuffer(device) {

}

void VulkanBufferUniform::CreateUniformBuffer(VkDeviceSize size) {
	m_size = size;
	SetupBuffer(size, (VkBufferUsageFlags)(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		(VmaMemoryUsage)(VMA_MEMORY_USAGE_CPU_TO_GPU));

	//device->CreateUniformBuffer(buffer, size);
	SetupResource();
	created = true;
}

void VulkanBufferUniform::CreateUniformBufferPersitantlyMapped(VkDeviceSize size) {
	persistantlyMapped = true;
	//CreateUniformBuffer(size);
	SetupBuffer(size, (VkBufferUsageFlags)(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		(VmaMemoryUsage)VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);

	Map(&mapped);
	created = true;
	SetupResource();
}

void VulkanBufferUniform::CreateStagingUniformBuffer(void* pData, VkDeviceSize size) {
	m_size = size;
	//device->CreateStagingUniformBuffer(buffer, pData, size);
	SetupBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, pData);

	SetupResource();
	created = true;
}

VulkanBufferUniformDynamic::VulkanBufferUniformDynamic(VulkanDevice& device) :
	VulkanBuffer(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {

}

void VulkanBufferUniformDynamic::CreateDynamicUniformBuffer(uint32_t count, VkDeviceSize size) {
	size_t minUboAlignment = device->physical_device_properties.limits.minUniformBufferOffsetAlignment;
	size_t dynamicAlignment = size;
	if (minUboAlignment > 0) {
		dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	m_size = count * dynamicAlignment;

	SetupBuffer(dynamicAlignment, (VkBufferUsageFlags)(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	//device->CreateDynamicUniformBuffer(buffer, count, size);
	SetupResource();
	created = true;
}

VulkanBufferStagingResource::VulkanBufferStagingResource(VulkanDevice& device) :
	VulkanBuffer(device)
{
}

void VulkanBufferStagingResource::CreateStagingResourceBuffer(void* pData, VkDeviceSize size)
{
	m_size = size;
	SetupBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, pData);

	//device->CreateStagingImageBuffer(buffer, pData, size);
	created = true;
	SetupResource();
}


VulkanBufferData::VulkanBufferData(VulkanDevice& device) :
	VulkanBuffer(device)
{}

void VulkanBufferData::CreateDataBuffer(VkDeviceSize size) {
	m_size = size;

	SetupBuffer(size, (VkBufferUsageFlags)(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);

	//device->CreateDataBuffer(buffer, size);
	created = true;
	SetupResource();
}


VulkanBufferVertex::VulkanBufferVertex(VulkanDevice& device) : VulkanBuffer(device) {

}

void VulkanBufferVertex::CreateVertexBuffer(uint32_t count, uint32_t vertexElementCount) {

	VkDeviceSize size = count * vertexElementCount * sizeof(float);
	m_size = size;

	SetupBuffer(size, (VmaMemoryUsage)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_GPU_ONLY);
	//device->CreateMeshBufferVertex(buffer, size);


	SetupResource();
	created = true;
}

void VulkanBufferVertex::CreateStagingVertexBuffer(void* pData, uint32_t count, uint32_t vertexElementCount) {
	VkDeviceSize size = count * vertexElementCount * sizeof(float);
	m_size = size;

	SetupBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT, pData);
	//device->CreateMeshStagingBuffer(buffer, pData, size);

	SetupResource();
	created = true;
}

void VulkanBufferVertex::BindVertexBuffer(VkCommandBuffer cmdBuf) {
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuf, VERTEX_BUFFER_BIND_ID, 1, &buffer.buffer, offsets);
}

VulkanBufferIndex::VulkanBufferIndex(VulkanDevice& device) : VulkanBuffer(device) {

}

void VulkanBufferIndex::CreateIndexBuffer(uint32_t count) {
	m_size = sizeof(int) * count;

	SetupBuffer(m_size, (VkBufferUsageFlags)(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_GPU_ONLY);
	//device->CreateMeshBufferIndex(buffer, sizeof(int) * count);


	SetupResource();
	created = true;
}

void VulkanBufferIndex::CreateStagingIndexBuffer(void* pData, uint32_t count) {
	m_size = sizeof(int) * count;

	SetupBuffer(m_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, pData);
	//device->CreateMeshStagingBuffer(buffer, pData, sizeof(int) * count);

	SetupResource();
	created = true;
}

void VulkanBufferIndex::BindIndexBuffer(VkCommandBuffer cmdBuf) {
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindIndexBuffer(cmdBuf, buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}


VulkanBufferInstance::VulkanBufferInstance(VulkanDevice& device) : VulkanBuffer(device) {

}

void VulkanBufferInstance::CreateInstanceBuffer(uint32_t count, uint32_t indexElementCount) {

	VkDeviceSize size = count * indexElementCount * sizeof(float);
	m_size = size;
	//device->CreateInstancingBuffer(buffer, size);

	SetupBuffer(size, (VkBufferUsageFlags)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_GPU_ONLY);

	SetupResource();
	created = true;
}

void VulkanBufferInstance::CreateStagingInstanceBuffer(void* pData, uint32_t count, uint32_t indexElementCount) {
	VkDeviceSize size = count * indexElementCount * sizeof(float);
	m_size = size;
	//device->CreateStagingInstancingBuffer(buffer, pData, size);

	SetupBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, pData);

	SetupResource();
	created = true;
}

void VulkanBufferInstance::BindInstanceBuffer(VkCommandBuffer cmdBuf) {
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuf, INSTANCE_BUFFER_BIND_ID, 1, &buffer.buffer, offsets);
}

void VulkanBufferInstance::CreatePersistantInstanceBuffer(
	uint32_t count, uint32_t indexElementCount)
{
	persistantlyMapped = true;
	VkDeviceSize size = count * indexElementCount * sizeof(float);
	m_size = size;
	//device->CreateMappedInstancingBuffer(buffer, size);

	SetupBuffer(size, (VkBufferUsageFlags)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);

	SetupResource();
	Map(&mapped);
	created = true;


}
