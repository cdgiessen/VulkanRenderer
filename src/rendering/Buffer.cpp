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

// VulkanBuffer::VulkanBuffer(const VulkanBuffer& buf)
// :buffer(buf.buffer), resource(buf.resource),
// device(buf.device), m_size(buf.m_size), 
// persistantlyMapped(buf.persistantlyMapped),
// mapped(buf.mapped)
// {

// }
// VulkanBuffer& VulkanBuffer::operator=(const VulkanBuffer& buf)
// {
// 	buffer = buf.buffer;
// 	resource = buf.resource;
// 	device = buf.device; 
// 	m_size = buf.m_size;
// 	persistantlyMapped = buf.persistantlyMapped;
// 	mapped = buf.mapped;

// }

void VulkanBuffer::CleanBuffer() {
	if (created) {
		if (persistantlyMapped) {
			Unmap();
		}

		device->DestroyVmaAllocatedBuffer(buffer);

		created = false;
	}
	else {
		//Log::Debug << "Cleaned up buffer!\n";
	}
}

VulkanBuffer::~VulkanBuffer() {
	//CleanBuffer();
}

void VulkanBuffer::Map(void** pData) {
	device->VmaMapMemory(buffer, pData);
}
void VulkanBuffer::Unmap() {
	device->VmaUnmapMemory(buffer);
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
	device->CreateUniformBuffer(buffer, size);
	SetupResource();
	created = true;
}

void VulkanBufferUniform::CreateUniformBufferPersitantlyMapped(VkDeviceSize size) {
	persistantlyMapped = true;
	CreateUniformBuffer(size);
	Map(&mapped);
	created = true;
}

void VulkanBufferUniform::CreateStagingUniformBuffer(void* pData, VkDeviceSize size) {
	m_size = size;
	device->CreateStagingUniformBuffer(buffer, pData, size);
	SetupResource();
	created = true;
}

VulkanBufferUniformDynamic::VulkanBufferUniformDynamic(VulkanDevice& device) :
	VulkanBuffer(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {

}

void VulkanBufferUniformDynamic::CreateDynamicUniformBuffer(uint32_t count, VkDeviceSize size) {
	m_size = count * size;
	device->CreateDynamicUniformBuffer(buffer, count, size);
	created = true;
}

VulkanBufferStagingResource::VulkanBufferStagingResource(VulkanDevice& device, void* pData, VkDeviceSize size) :
	VulkanBuffer(device)
{
	m_size = size;
	this->device->CreateStagingImageBuffer(buffer, pData, size);
	created = true;
}

VulkanBufferVertex::VulkanBufferVertex(VulkanDevice& device) : VulkanBuffer(device) {

}

void VulkanBufferVertex::CreateVertexBuffer(uint32_t count, uint32_t vertexElementCount) {

	VkDeviceSize size = count * vertexElementCount * sizeof(float);
	m_size = size;
	device->CreateMeshBufferVertex(buffer, size);
	SetupResource();
	created = true;
}

void VulkanBufferVertex::CreateStagingVertexBuffer(void* pData, uint32_t count, uint32_t vertexElementCount) {
	VkDeviceSize size = count * vertexElementCount * sizeof(float);
	m_size = size;
	device->CreateMeshStagingBuffer(buffer, pData, size);
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
	device->CreateMeshBufferIndex(buffer, sizeof(int) * count);
	SetupResource();
	created = true;
}

void VulkanBufferIndex::CreateStagingIndexBuffer(void* pData, uint32_t count) {
	m_size = sizeof(int) * count;
	device->CreateMeshStagingBuffer(buffer, pData, sizeof(int) * count);
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
	device->CreateInstancingBuffer(buffer, size);
	SetupResource();
	created = true;
}

void VulkanBufferInstance::CreateStagingInstanceBuffer(void* pData, uint32_t count, uint32_t indexElementCount) {
	VkDeviceSize size = count * indexElementCount * sizeof(float);
	m_size = size;
	device->CreateStagingInstancingBuffer(buffer, pData, size);
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
	CreateInstanceBuffer(count, indexElementCount);
	Map(&mapped);
	created = true;


}
