#include "Model.h"

#include <iterator>

#include "Renderer.h"


void CopyMeshBuffers(
	const VkCommandBuffer copyCmd,
	const VkBuffer vertexStagingBuffer,
	const VkBuffer vertexBuffer,
	const uint32_t vBufferSize,
	const VkBuffer indexStagingBuffer,
	const VkBuffer indexBuffer,
	const uint32_t iBufferSize)
{
	VkBufferCopy copyRegion{};
	copyRegion.size = vBufferSize;
	vkCmdCopyBuffer(copyCmd, vertexStagingBuffer, vertexBuffer, 1, &copyRegion);

	copyRegion.size = iBufferSize;
	vkCmdCopyBuffer(copyCmd, indexStagingBuffer, indexBuffer, 1, &copyRegion);
}

VulkanModel::VulkanModel(VulkanRenderer& renderer, std::shared_ptr<Mesh> mesh)
	: renderer(renderer)
{
	readyToUse = std::make_shared<bool>(false);

	std::vector<float> vertexBuffer;
	std::vector<uint32_t> indexBuffer;

	indexCount = static_cast<uint32_t>(mesh->indices.size());
	vertexElementCount = mesh->vertexElementCount;
	vertexBuffer.resize(vertexCount * vertexElementCount);

	if (vertexElementCount == 6) {
		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNorm>(mesh->vertices).size());
		vertexBuffer.resize(vertexCount * mesh->vertexElementCount);

		for (int i = 0; i < (int)vertexCount; i++)
		{
			vertexBuffer[i * 6 + 0] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[0];
			vertexBuffer[i * 6 + 1] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[1];
			vertexBuffer[i * 6 + 2] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[2];
			vertexBuffer[i * 6 + 3] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[0];
			vertexBuffer[i * 6 + 4] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[1];
			vertexBuffer[i * 6 + 5] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[2];
		}
	}
	else if (vertexElementCount == 8) {
		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNormTex>(mesh->vertices).size());
		vertexBuffer.resize(vertexCount * mesh->vertexElementCount);

		for (int i = 0; i < (int)vertexCount; i++)
		{
			vertexBuffer[i * 8 + 0] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[0];
			vertexBuffer[i * 8 + 1] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[1];
			vertexBuffer[i * 8 + 2] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[2];
			vertexBuffer[i * 8 + 3] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[0];
			vertexBuffer[i * 8 + 4] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[1];
			vertexBuffer[i * 8 + 5] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[2];
			vertexBuffer[i * 8 + 6] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].texCoord[0];
			vertexBuffer[i * 8 + 7] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].texCoord[1];
		}

	}
	else {
		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNormTexColor>(mesh->vertices).size());
		vertexBuffer.resize(vertexCount * vertexElementCount);

		for (int i = 0; i < (int)vertexCount; i++)
		{
			vertexBuffer[i * 12 + 0] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[0];
			vertexBuffer[i * 12 + 1] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[1];
			vertexBuffer[i * 12 + 2] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[2];
			vertexBuffer[i * 12 + 3] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[0];
			vertexBuffer[i * 12 + 4] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[1];
			vertexBuffer[i * 12 + 5] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[2];
			vertexBuffer[i * 12 + 6] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).texCoord[0];
			vertexBuffer[i * 12 + 7] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).texCoord[1];
			vertexBuffer[i * 12 + 8] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[0];
			vertexBuffer[i * 12 + 9] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[1];
			vertexBuffer[i * 12 + 10] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[2];
			vertexBuffer[i * 12 + 11] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[3];


		}
	}

	indexBuffer.resize(indexCount);
	for (int i = 0; i < (int)indexCount; i++)
	{
		indexBuffer[i] = mesh->indices[i];
	}

	uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
	uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);


	vmaVertices = std::make_unique<VulkanBufferVertex>(renderer.device, (uint32_t)vertexBuffer.size(), vertexElementCount);
	vmaIndicies = std::make_unique<VulkanBufferIndex>(renderer.device, (uint32_t)indexBuffer.size());

	auto vertexStagingBuffer = std::make_shared<VulkanBufferStagingVertex>(renderer.device, (uint32_t)vertexCount, vertexElementCount, vertexBuffer.data());
	auto indexStagingBuffer = std::make_shared<VulkanBufferStagingIndex>(renderer.device, (uint32_t)indexCount, indexBuffer.data());

	//vertexStagingBuffer.CreateStagingVertexBuffer(vertexBuffer.data(), (uint32_t)vertexCount, vertexElementCount);
	//indexStagingBuffer.CreateStagingIndexBuffer(indexBuffer.data(), (uint32_t)indexCount);

	VkBuffer vert = vmaVertices->buffer.buffer;
	VkBuffer index = vmaIndicies->buffer.buffer;
	VkBuffer v_stage = vertexStagingBuffer->buffer.buffer;
	VkBuffer i_stage = indexStagingBuffer->buffer.buffer;

	std::function<void(const VkCommandBuffer)> work =
		[=](const VkCommandBuffer copyCmd) {

		CopyMeshBuffers(copyCmd,
			v_stage, vert, vBufferSize,
			i_stage, index, iBufferSize);
	};

	//std::vector<VulkanBuffer> buffers;
	//buffers.push_back(std::move(vertexStagingBuffer));
	//buffers.push_back(std::move(indexStagingBuffer));

	renderer.SubmitTransferWork(work,
		{}, {}, { vertexStagingBuffer, indexStagingBuffer },
		{ readyToUse });

}
//
//bool VulkanModel::loadFromMesh(std::shared_ptr<Mesh> mesh,
//	VulkanRenderer& renderer) {
//
//	std::vector<float> vertexBuffer;
//	std::vector<uint32_t> indexBuffer;
//
//	indexCount = static_cast<uint32_t>(mesh->indices.size());
//	vertexElementCount = mesh->vertexElementCount;
//	vertexBuffer.resize(vertexCount * vertexElementCount);
//
//	if (vertexElementCount == 6) {
//		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNorm>(mesh->vertices).size());
//		vertexBuffer.resize(vertexCount * mesh->vertexElementCount);
//
//		for (int i = 0; i < (int)vertexCount; i++)
//		{
//			vertexBuffer[i * 6 + 0] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[0];
//			vertexBuffer[i * 6 + 1] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[1];
//			vertexBuffer[i * 6 + 2] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[2];
//			vertexBuffer[i * 6 + 3] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[0];
//			vertexBuffer[i * 6 + 4] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[1];
//			vertexBuffer[i * 6 + 5] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[2];
//		}
//	}
//	else if (vertexElementCount == 8) {
//		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNormTex>(mesh->vertices).size());
//		vertexBuffer.resize(vertexCount * mesh->vertexElementCount);
//
//		for (int i = 0; i < (int)vertexCount; i++)
//		{
//			vertexBuffer[i * 8 + 0] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[0];
//			vertexBuffer[i * 8 + 1] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[1];
//			vertexBuffer[i * 8 + 2] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[2];
//			vertexBuffer[i * 8 + 3] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[0];
//			vertexBuffer[i * 8 + 4] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[1];
//			vertexBuffer[i * 8 + 5] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[2];
//			vertexBuffer[i * 8 + 6] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].texCoord[0];
//			vertexBuffer[i * 8 + 7] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].texCoord[1];
//		}
//
//	}
//	else {
//		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNormTexColor>(mesh->vertices).size());
//		vertexBuffer.resize(vertexCount * vertexElementCount);
//
//		for (int i = 0; i < (int)vertexCount; i++)
//		{
//			vertexBuffer[i * 12 + 0] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[0];
//			vertexBuffer[i * 12 + 1] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[1];
//			vertexBuffer[i * 12 + 2] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[2];
//			vertexBuffer[i * 12 + 3] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[0];
//			vertexBuffer[i * 12 + 4] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[1];
//			vertexBuffer[i * 12 + 5] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[2];
//			vertexBuffer[i * 12 + 6] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).texCoord[0];
//			vertexBuffer[i * 12 + 7] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).texCoord[1];
//			vertexBuffer[i * 12 + 8] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[0];
//			vertexBuffer[i * 12 + 9] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[1];
//			vertexBuffer[i * 12 + 10] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[2];
//			vertexBuffer[i * 12 + 11] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[3];
//
//
//		}
//	}
//
//	indexBuffer.resize(indexCount);
//	for (int i = 0; i < (int)indexCount; i++)
//	{
//		indexBuffer[i] = mesh->indices[i];
//	}
//
//	uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
//	uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);
//
//
//	vmaVertices = std::make_unique<VulkanBufferVertex>(device, (uint32_t)vertexBuffer.size(), vertexElementCount);
//	vmaIndicies = std::make_unique<VulkanBufferIndex>(device, (uint32_t)indexBuffer.size());
//
//	auto vertexStagingBuffer = std::make_shared<VulkanBufferStagingVertex>(device, (uint32_t)vertexCount, vertexElementCount, vertexBuffer.data());
//	auto indexStagingBuffer = std::make_shared<VulkanBufferStagingIndex>(device, (uint32_t)indexCount, indexBuffer.data());
//
//	//vertexStagingBuffer.CreateStagingVertexBuffer(vertexBuffer.data(), (uint32_t)vertexCount, vertexElementCount);
//	//indexStagingBuffer.CreateStagingIndexBuffer(indexBuffer.data(), (uint32_t)indexCount);
//
//	VkBuffer vert = vmaVertices->buffer.buffer;
//	VkBuffer index = vmaIndicies->buffer.buffer;
//	VkBuffer v_stage = vertexStagingBuffer->buffer.buffer;
//	VkBuffer i_stage = indexStagingBuffer->buffer.buffer;
//
//	std::function<void(const VkCommandBuffer)> work =
//		[=](const VkCommandBuffer copyCmd) {
//
//		CopyMeshBuffers(copyCmd,
//			v_stage, vert, vBufferSize,
//			i_stage, index, iBufferSize);
//	};
//
//	//std::vector<VulkanBuffer> buffers;
//	//buffers.push_back(std::move(vertexStagingBuffer));
//	//buffers.push_back(std::move(indexStagingBuffer));
//
//	renderer.SubmitTransferWork(work,
//		{}, {}, { vertexStagingBuffer, indexStagingBuffer },
//		{ readyToUse });
//
//	return true;
//
//
//
//	//// Use staging buffer to move vertex and index buffer to device local memory
//	//// Create staging buffers
//	//VulkanBuffer vertexStaging, indexStaging;
//
//	//// Vertex buffer
//	//VK_CHECK_RESULT(device.createBuffer(
//	//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//	//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
//	//	&vertexStaging,
//	//	vBufferSize,
//	//	vertexBuffer.data()));
//	////auto vbdata = vertexBuffer.data();
//	//// Index buffer
//	//VK_CHECK_RESULT(device.createBuffer(
//	//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//	//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
//	//	&indexStaging,
//	//	iBufferSize,
//	//	indexBuffer.data()));
//
//	//// Create device local target buffers
//	//// Vertex buffer
//	//VK_CHECK_RESULT(device.createBuffer(
//	//	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//	//	&vertices,
//	//	vBufferSize));
//
//	//// Index buffer
//	//VK_CHECK_RESULT(device.createBuffer(
//	//	VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//	//	&indices,
//	//	iBufferSize));
//
//	//// Copy from staging buffers
//	////VkCommandBuffer copyCmd = 
//	//VkCommandBuffer copyCmd = device.createCommandBuffer(device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
//
//	//VkBufferCopy copyRegion{};
//
//	//copyRegion.size = vertices.size;
//	//vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);
//
//	//copyRegion.size = indices.size;
//	//vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
//
//	//device.flushCommandBuffer(copyCmd, copyQueue);
//
//	//// Destroy staging resources
//	//vkDestroyBuffer(device.device, vertexStaging.buffer, nullptr);
//	//vkFreeMemory(device.device, vertexStaging.bufferMemory, nullptr);
//	//vkDestroyBuffer(device.device, indexStaging.buffer, nullptr);
//	//vkFreeMemory(device.device, indexStaging.bufferMemory, nullptr);
//
//	//return true;
//}

void VulkanModel::BindModel(VkCommandBuffer cmdBuf) {
	vmaVertices->BindVertexBuffer(cmdBuf);
	vmaIndicies->BindIndexBuffer(cmdBuf);
}