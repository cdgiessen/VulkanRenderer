#include "Model.h"

#include <iterator>
#include <numeric>

#include "Initializers.h"
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
	for (size_t i = 0; i < vertDesc.layout.size (); i++)
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

VulkanModel::VulkanModel (VulkanRenderer& renderer, std::unique_ptr<MeshData> mesh)
: renderer (renderer), vertLayout (mesh->desc)
{
	readyToUse = std::make_shared<bool> (false);

	vertexCount = (uint32_t)mesh->vertexData.size ();
	vertexElementCount = (uint32_t)mesh->desc.ElementCount ();
	indexCount = (uint32_t)mesh->indexData.size ();

	uint32_t vBufferSize = static_cast<uint32_t> (vertexCount) * sizeof (float);
	uint32_t iBufferSize = static_cast<uint32_t> (indexCount) * sizeof (uint32_t);

	vmaVertices = std::make_unique<VulkanBuffer> (
	    renderer.device, vertex_details (vertexCount, vertexElementCount));
	vmaIndicies = std::make_unique<VulkanBuffer> (renderer.device, index_details (indexCount));

	auto vert_stage_details = staging_details (BufferType::vertex, vBufferSize);
	auto index_stage_details = staging_details (BufferType::index, iBufferSize);


	auto vertexStagingBuffer =
	    std::make_shared<VulkanBuffer> (renderer.device, vert_stage_details, mesh->vertexData.data ());
	auto indexStagingBuffer =
	    std::make_shared<VulkanBuffer> (renderer.device, index_stage_details, mesh->indexData.data ());

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