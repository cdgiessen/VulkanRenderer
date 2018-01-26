#include "VulkanBuffer.hpp"


VulkanBuffer::VulkanBuffer()
	: resource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
}

VulkanBuffer::VulkanBuffer(VkDescriptorType type)
	: resource(type) {
}

void VulkanBuffer::CleanBuffer(VulkanDevice& device) {
	device.DestroyVmaAllocatedBuffer(buffer);
}

void VulkanBuffer::Map(VulkanDevice& device, void** pData) {
	device.VmaMapMemory(buffer, pData);
}
void VulkanBuffer::Unmap(VulkanDevice& device) {
	device.VmaUnmapMemory(buffer);
}

void VulkanBuffer::SetupResource() {
	resource.FillResource(buffer.buffer, 0, m_size);
}

void VulkanBuffer::CopyToBuffer(VulkanDevice& device, void* pData, VkDeviceSize size)
{
	void* mapped;
	this->Map(device, &mapped);
	memcpy(mapped, pData, (size_t)size);
	this->Unmap(device);
}

VulkanBufferUniform::VulkanBufferUniform() {

}

void VulkanBufferUniform::CreateUniformBuffer(VulkanDevice& device, VkDeviceSize size) {
	m_size = size;
	device.CreateUniformBuffer(buffer, size);
	SetupResource();
}

void VulkanBufferUniform::CreateStagingUniformBuffer(VulkanDevice& device, void* pData, VkDeviceSize size) {
	m_size = size;
	device.CreateStagingUniformBuffer(buffer, pData, size);
	SetupResource();
}

VulkanBufferUniformDynamic::VulkanBufferUniformDynamic() :
	VulkanBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {

}

void VulkanBufferUniformDynamic::CreateDynamicUniformBuffer(VulkanDevice& device, uint32_t count, VkDeviceSize size) {
	m_size = count*size;
	device.CreateDynamicUniformBuffer(buffer, count, size);
}

VulkanBufferVertex::VulkanBufferVertex() {

}
	 
void VulkanBufferVertex::CreateVertexBuffer(VulkanDevice& device, uint32_t count) {

	VkDeviceSize size = count * 12 * sizeof(float);
	m_size = size;
	device.CreateMeshBufferVertex(buffer, size);
	SetupResource();
}

void VulkanBufferVertex::CreateStagingVertexBuffer(VulkanDevice& device, void* pData, uint32_t count) {
	VkDeviceSize size = count * 12 * sizeof(float);
	m_size = size;
	device.CreateMeshStagingBuffer(buffer, pData, size);
	SetupResource();
}

void VulkanBufferVertex::BindVertexBuffer(VkCommandBuffer cmdBuf) {
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuf, 0, 1, &buffer.buffer, offsets);
}

VulkanBufferIndex::VulkanBufferIndex() {

}

void VulkanBufferIndex::CreateIndexBuffer(VulkanDevice& device, uint32_t count) {
	m_size = sizeof(int) * count;
	device.CreateMeshBufferIndex(buffer, sizeof(int) * count);
	SetupResource();
} 

void VulkanBufferIndex::CreateStagingIndexBuffer(VulkanDevice& device, void* pData, uint32_t count) {
	m_size = sizeof(int) * count;
	device.CreateMeshStagingBuffer(buffer, pData, sizeof(int) * count);
	SetupResource();
}

void VulkanBufferIndex::BindIndexBuffer(VkCommandBuffer cmdBuf) {
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindIndexBuffer(cmdBuf, buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}