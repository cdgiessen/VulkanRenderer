#include "Buffer.hpp"


VulkanBuffer::VulkanBuffer(VulkanDevice& device)
	: device(device), resource(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER){
}

VulkanBuffer::VulkanBuffer(VulkanDevice& device, VkDescriptorType type)
	: device(device), resource(type) {
}

void VulkanBuffer::CleanBuffer() {
	if (persistantlyMapped) {
		Unmap();
	}

	device.DestroyVmaAllocatedBuffer(buffer);
}

void VulkanBuffer::Map(void** pData) {
	device.VmaMapMemory(buffer, pData);
}
void VulkanBuffer::Unmap() {
	device.VmaUnmapMemory(buffer);
}

void VulkanBuffer::SetupResource() {
	resource.FillResource(buffer.buffer, 0, m_size);
}

void AlignedMemcpy(uint8_t bytes, VkDeviceSize destMemAlignment, void* src, void* dst ) {
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
	
		//VkDeviceSize bufAlignment = device.physical_device_properties.limits.minUniformBufferOffsetAlignment;
	
		//AlignedMemcpy((size_t)size, bufAlignment, pData, mapped);
	
		this->Unmap();
	}
}

VulkanBufferUniform::VulkanBufferUniform(VulkanDevice& device): VulkanBuffer(device) {

}

void VulkanBufferUniform::CreateUniformBuffer(VkDeviceSize size) {
	m_size = size;
	device.CreateUniformBuffer(buffer, size);
	SetupResource();
}

void VulkanBufferUniform::CreateUniformBufferPersitantlyMapped(VkDeviceSize size) {
	persistantlyMapped = true;
	CreateUniformBuffer(size);
	Map(&mapped);
}

void VulkanBufferUniform::CreateStagingUniformBuffer(void* pData, VkDeviceSize size) {
	m_size = size;
	device.CreateStagingUniformBuffer(buffer, pData, size);
	SetupResource();
}

VulkanBufferUniformDynamic::VulkanBufferUniformDynamic(VulkanDevice& device):
	VulkanBuffer(device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {

}

void VulkanBufferUniformDynamic::CreateDynamicUniformBuffer(uint32_t count, VkDeviceSize size) {
	m_size = count*size;
	device.CreateDynamicUniformBuffer(buffer, count, size);
}

VulkanBufferVertex::VulkanBufferVertex(VulkanDevice& device) : VulkanBuffer(device) {

}
	 
void VulkanBufferVertex::CreateVertexBuffer(uint32_t count) {

	VkDeviceSize size = count * 12 * sizeof(float);
	m_size = size;
	device.CreateMeshBufferVertex(buffer, size);
	SetupResource();
}

void VulkanBufferVertex::CreateStagingVertexBuffer(void* pData, uint32_t count) {
	VkDeviceSize size = count * 12 * sizeof(float);
	m_size = size;
	device.CreateMeshStagingBuffer(buffer, pData, size);
	SetupResource();
}

void VulkanBufferVertex::BindVertexBuffer(VkCommandBuffer cmdBuf) {
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuf, 0, 1, &buffer.buffer, offsets);
}

VulkanBufferIndex::VulkanBufferIndex(VulkanDevice& device) : VulkanBuffer(device) {

}

void VulkanBufferIndex::CreateIndexBuffer(uint32_t count) {
	m_size = sizeof(int) * count;
	device.CreateMeshBufferIndex(buffer, sizeof(int) * count);
	SetupResource();
} 

void VulkanBufferIndex::CreateStagingIndexBuffer(void* pData, uint32_t count) {
	m_size = sizeof(int) * count;
	device.CreateMeshStagingBuffer(buffer, pData, sizeof(int) * count);
	SetupResource();
}

void VulkanBufferIndex::BindIndexBuffer(VkCommandBuffer cmdBuf) {
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindIndexBuffer(cmdBuf, buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}