#include "Mesh.h"



Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices) : vertices(vertices), indices(indices)
{
}


Mesh::~Mesh()
{
}

void Mesh::createMeshBuffers(VkDevice device, VkPhysicalDevice physicalDevice)
{
	this->device = device;

	createVertexBuffer();
	createIndexBuffer();
}

void Mesh::createVertexBuffer() {
	//VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	//VulkanBuffer stagingBuffer(device,physicalDevice);
	//stagingBuffer.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	//void* data;
	//vkMapMemory(device, stagingBuffer.getBufferMemory(), 0, bufferSize, 0, &data);
	//memcpy(data, vertices.data(), (size_t)bufferSize);
	//vkUnmapMemory(device, stagingBuffer.getBufferMemory());

	//createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	//stagingBuffer.cleanBuffer();
}

//16
void Mesh::createIndexBuffer() {
/*	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	//indexBuffer.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	//indexBuffer.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);*/
}

void Mesh::cleanup(VkDevice device) {
	//vertexBuffer.cleanBuffer();
	//indexBuffer.cleanBuffer();

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}


