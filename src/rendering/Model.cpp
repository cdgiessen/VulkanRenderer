#include "Model.h"

#include <iterator>

#include "Renderer.h"

std::vector<VkVertexInputBindingDescription> VertexLayout::getBindingDescription (VertexDescription vertDesc)
{
	std::vector<VkVertexInputBindingDescription> bindingDescription;

	int size = std::accumulate (std::begin (vertDesc.layout), std::end (vertDesc.layout), 0) * 4;

	bindingDescription.push_back (
	    initializers::vertexInputBindingDescription (0, size, VK_VERTEX_INPUT_RATE_VERTEX));
	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> VertexLayout::getAttributeDescriptions (VertexDescription vertDesc)
{
	std::vector<VkVertexInputAttributeDescription> attrib = {};

	int offset = 0;
	for (int i = 0; i < vertDesc.layout.size (); i++)
	{
		VkFormat vertSize = VK_FORMAT_R32_SFLOAT;
		if (vertDesc.layout[i] == 2) vertSize = VK_FORMAT_R32G32_SFLOAT;
		if (vertDesc.layout[i] == 3) vertSize = VK_FORMAT_R32G32B32_SFLOAT;
		if (vertDesc.layout[i] == 4) vertSize = VK_FORMAT_R32G32B32A32_SFLOAT;

		attrib.push_back (initializers::vertexInputAttributeDescription (0, 0, vertSize, offset));
		offset += vertDesc.layout[i] * 4;
	}
	return attrib;
};

void CopyMeshBuffers (const VkCommandBuffer copyCmd,
    const VkBuffer vertexStagingBuffer,
    const VkBuffer vertexBuffer,
    const uint32_t vBufferSize,
    const VkBuffer indexStagingBuffer,
    const VkBuffer indexBuffer,
    const uint32_t iBufferSize)
{
	VkBufferCopy copyRegion{};
	copyRegion.size = vBufferSize;
	vkCmdCopyBuffer (copyCmd, vertexStagingBuffer, vertexBuffer, 1, &copyRegion);

	copyRegion.size = iBufferSize;
	vkCmdCopyBuffer (copyCmd, indexStagingBuffer, indexBuffer, 1, &copyRegion);
}

// VulkanModel::VulkanModel (VulkanRenderer& renderer, std::shared_ptr<Mesh> mesh)
//: renderer (renderer), vertLayout (VertexDescription ({ 3, 3, 2 }))
//{
//	readyToUse = std::make_shared<bool> (false);
//
//	std::vector<float> vertexBuffer;
//	std::vector<uint32_t> indexBuffer;
//
//	indexCount = static_cast<uint32_t> (mesh->indices.size ());
//	vertexElementCount = mesh->vertexElementCount;
//	vertexBuffer.resize (vertexCount * vertexElementCount);
//
//	if (vertexElementCount == 6)
//	{
//		vertexCount = static_cast<uint32_t> (std::get<Vertices_PosNorm> (mesh->vertices).size ());
//		vertexBuffer.resize (vertexCount * mesh->vertexElementCount);
//
//		for (int i = 0; i < (int)vertexCount; i++)
//		{
//			vertexBuffer[i * 6 + 0] = std::get<Vertices_PosNorm> (mesh->vertices)[i].pos[0];
//			vertexBuffer[i * 6 + 1] = std::get<Vertices_PosNorm> (mesh->vertices)[i].pos[1];
//			vertexBuffer[i * 6 + 2] = std::get<Vertices_PosNorm> (mesh->vertices)[i].pos[2];
//			vertexBuffer[i * 6 + 3] = std::get<Vertices_PosNorm> (mesh->vertices)[i].normal[0];
//			vertexBuffer[i * 6 + 4] = std::get<Vertices_PosNorm> (mesh->vertices)[i].normal[1];
//			vertexBuffer[i * 6 + 5] = std::get<Vertices_PosNorm> (mesh->vertices)[i].normal[2];
//		}
//		vertLayout = VertexLayout (VertexDescription ({ 3, 3 }));
//	}
//	else if (vertexElementCount == 8)
//	{
//		vertexCount = static_cast<uint32_t> (std::get<Vertices_PosNormTex> (mesh->vertices).size ());
//		vertexBuffer.resize (vertexCount * mesh->vertexElementCount);
//
//		for (int i = 0; i < (int)vertexCount; i++)
//		{
//			vertexBuffer[i * 8 + 0] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].pos[0];
//			vertexBuffer[i * 8 + 1] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].pos[1];
//			vertexBuffer[i * 8 + 2] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].pos[2];
//			vertexBuffer[i * 8 + 3] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].normal[0];
//			vertexBuffer[i * 8 + 4] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].normal[1];
//			vertexBuffer[i * 8 + 5] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].normal[2];
//			vertexBuffer[i * 8 + 6] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].texCoord[0];
//			vertexBuffer[i * 8 + 7] = std::get<Vertices_PosNormTex> (mesh->vertices)[i].texCoord[1];
//		}
//		vertLayout = VertexLayout (VertexDescription ({ 3, 3, 2 }));
//	}
//	else
//	{
//		vertexCount = static_cast<uint32_t> (std::get<Vertices_PosNormTexColor> (mesh->vertices).size ());
//		vertexBuffer.resize (vertexCount * vertexElementCount);
//
//		for (int i = 0; i < (int)vertexCount; i++)
//		{
//			vertexBuffer[i * 12 + 0] = (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).pos[0];
//			vertexBuffer[i * 12 + 1] = (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).pos[1];
//			vertexBuffer[i * 12 + 2] = (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).pos[2];
//			vertexBuffer[i * 12 + 3] =
//			    (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).normal[0];
//			vertexBuffer[i * 12 + 4] =
//			    (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).normal[1];
//			vertexBuffer[i * 12 + 5] =
//			    (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).normal[2];
//			vertexBuffer[i * 12 + 6] =
//			    (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).texCoord[0];
//			vertexBuffer[i * 12 + 7] =
//			    (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).texCoord[1];
//			vertexBuffer[i * 12 + 8] = (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).color[0];
//			vertexBuffer[i * 12 + 9] = (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).color[1];
//			vertexBuffer[i * 12 + 10] =
//			    (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).color[2];
//			vertexBuffer[i * 12 + 11] =
//			    (std::get<Vertices_PosNormTexColor> (mesh->vertices)).at (i).color[3];
//		}
//		vertLayout = VertexLayout (VertexDescription ({ 3, 3, 2, 4 }));
//	}
//
//	indexBuffer.resize (indexCount);
//	for (int i = 0; i < (int)indexCount; i++)
//	{
//		indexBuffer[i] = mesh->indices[i];
//	}
//
//	uint32_t vBufferSize = static_cast<uint32_t> (vertexBuffer.size ()) * sizeof (float);
//	uint32_t iBufferSize = static_cast<uint32_t> (indexBuffer.size ()) * sizeof (uint32_t);
//
//
//	vmaVertices = std::make_unique<VulkanBufferVertex> (
//	    renderer.device, (uint32_t)vertexBuffer.size (), vertexElementCount);
//	vmaIndicies = std::make_unique<VulkanBufferIndex> (renderer.device, (uint32_t)indexBuffer.size ());
//
//	auto vertexStagingBuffer = std::make_shared<VulkanBufferStagingVertex> (
//	    renderer.device, (uint32_t)vertexCount, vertexElementCount, vertexBuffer.data ());
//	auto indexStagingBuffer = std::make_shared<VulkanBufferStagingIndex> (
//	    renderer.device, (uint32_t)indexCount, indexBuffer.data ());
//
//	VkBuffer vert = vmaVertices->buffer.buffer;
//	VkBuffer index = vmaIndicies->buffer.buffer;
//	VkBuffer v_stage = vertexStagingBuffer->buffer.buffer;
//	VkBuffer i_stage = indexStagingBuffer->buffer.buffer;
//
//	std::function<void(const VkCommandBuffer)> work = [=](const VkCommandBuffer copyCmd) {
//		CopyMeshBuffers (copyCmd, v_stage, vert, vBufferSize, i_stage, index, iBufferSize);
//	};
//
//	renderer.SubmitWork (
//	    WorkType::transfer, work, {}, {}, { vertexStagingBuffer, indexStagingBuffer }, { readyToUse });
//}

VulkanModel::VulkanModel (VulkanRenderer& renderer, std::shared_ptr<MeshData> mesh)
: renderer (renderer), vertLayout (mesh->desc)
{
	readyToUse = std::make_shared<bool> (false);

	vertexCount = mesh->vertexData.size();
	vertexElementCount = mesh->desc.ElementCount();
	indexCount = (uint16_t)mesh->indexData.size();

	uint32_t vBufferSize = static_cast<uint32_t> (vertexCount) * sizeof (float);
	uint16_t iBufferSize = static_cast<uint16_t> (indexCount) * sizeof (uint16_t);


	vmaVertices = std::make_unique<VulkanBufferVertex> (renderer.device, vertexCount);
	vmaIndicies = std::make_unique<VulkanBufferIndex> (renderer.device, (uint16_t)indexCount);

	auto vertexStagingBuffer = std::make_shared<VulkanBufferStagingVertex> (
	    renderer.device, mesh->vertexData.size (), mesh->vertexData.data ());
	auto indexStagingBuffer = std::make_shared<VulkanBufferStagingIndex> (
	    renderer.device, (uint16_t)mesh->indexData.size(), mesh->indexData.data ());

	VkBuffer vert = vmaVertices->buffer.buffer;
	VkBuffer index = vmaIndicies->buffer.buffer;
	VkBuffer v_stage = vertexStagingBuffer->buffer.buffer;
	VkBuffer i_stage = indexStagingBuffer->buffer.buffer;

	std::function<void(const VkCommandBuffer)> work = [=](const VkCommandBuffer copyCmd) {
		CopyMeshBuffers (copyCmd, v_stage, vert, vBufferSize, i_stage, index, iBufferSize);
	};

	renderer.SubmitWork (
	    WorkType::transfer, work, {}, {}, { vertexStagingBuffer, indexStagingBuffer }, { readyToUse });
}

void VulkanModel::BindModel (VkCommandBuffer cmdBuf)
{
	vmaVertices->BindVertexBuffer (cmdBuf);
	vmaIndicies->BindIndexBuffer (cmdBuf);
}


VertexLayout VulkanModel::GetVertexLayout () { return vertLayout; }