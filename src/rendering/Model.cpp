#include "Model.h"

#include <iterator>
#include <numeric>

#include "AsyncTask.h"
#include "Device.h"
#include "Initializers.h"

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

VulkanModel::VulkanModel (
    VulkanDevice& device, AsyncTaskManager& async_task_man, BufferManager& buf_man, std::unique_ptr<MeshData> mesh)
: vertLayout (mesh->desc)
{
	vertexCount = (uint32_t)mesh->vertexData.size ();
	vertexElementCount = (uint32_t)mesh->desc.ElementCount ();
	indexCount = (uint32_t)mesh->indexData.size ();

	uint32_t vBufferSize = static_cast<uint32_t> (vertexCount) * sizeof (float);
	uint32_t iBufferSize = static_cast<uint32_t> (indexCount) * sizeof (uint32_t);

	vmaVertices = std::make_unique<VulkanBuffer> (device, vertex_details (vertexCount, vertexElementCount));
	vmaIndicies = std::make_unique<VulkanBuffer> (device, index_details (indexCount));

	auto vert_stage_details = staging_details (BufferType::vertex, vBufferSize);
	auto index_stage_details = staging_details (BufferType::index, iBufferSize);

	auto vertexStagingBuffer = buf_man.CreateBuffer (vert_stage_details);
	auto indexStagingBuffer = buf_man.CreateBuffer (index_stage_details);

	buf_man.GetBuffer (vertexStagingBuffer).CopyToBuffer (mesh->vertexData);
	buf_man.GetBuffer (indexStagingBuffer).CopyToBuffer (mesh->indexData);

	VkBuffer vert = vmaVertices->buffer;
	VkBuffer index = vmaIndicies->buffer;
	VkBuffer v_stage = buf_man.GetBuffer (vertexStagingBuffer).buffer;
	VkBuffer i_stage = buf_man.GetBuffer (indexStagingBuffer).buffer;

	std::function<void(const VkCommandBuffer)> work = [=](const VkCommandBuffer copyCmd) {
		CopyMeshBuffers (copyCmd, v_stage, vert, vBufferSize, i_stage, index, iBufferSize);
	};

	std::function<void()> finish_work = [&]() {
		buf_man.FreeBuffer (vertexStagingBuffer);
		buf_man.FreeBuffer (indexStagingBuffer);
	};

	AsyncTask transfer;
	transfer.work = work;
	transfer.finish_work = finish_work;

	async_task_man.SubmitTask (TaskType::transfer, std::move (transfer));
}

void VulkanModel::BindModel (VkCommandBuffer cmdBuf)
{
	vmaVertices->BindVertexBuffer (cmdBuf);
	vmaIndicies->BindIndexBuffer (cmdBuf);
}


VertexLayout VulkanModel::GetVertexLayout () { return vertLayout; }

ModelManager::ModelManager (
    Resource::Mesh::Manager& mesh_manager, VulkanDevice& device, AsyncTaskManager& async_task_man, BufferManager& buf_man)
: mesh_manager (mesh_manager), device (device), async_task_man (async_task_man), buf_man (buf_man)
{
}