#include "Model.h"

#include <iterator>
#include <numeric>

#include "resources/Mesh.h"

#include "AsyncTask.h"
#include "Device.h"
#include "rendering/Initializers.h"

VertexLayout::VertexLayout (Resource::Mesh::VertexDescription const& vertDesc)
{
	// VkVertexInputBindingDescription
	int size = std::accumulate (std::begin (vertDesc.layout), std::end (vertDesc.layout), 0) * 4;

	bindingDesc.push_back (initializers::vertexInputBindingDescription (0, size, VK_VERTEX_INPUT_RATE_VERTEX));

	// VkVertexInputAttributeDescription's
	int offset = 0;
	for (size_t i = 0; i < vertDesc.layout.size (); i++)
	{
		VkFormat vertSize = VK_FORMAT_R32_SFLOAT;
		if (vertDesc.layout[i] == 2) vertSize = VK_FORMAT_R32G32_SFLOAT;
		if (vertDesc.layout[i] == 3) vertSize = VK_FORMAT_R32G32B32_SFLOAT;
		if (vertDesc.layout[i] == 4) vertSize = VK_FORMAT_R32G32B32A32_SFLOAT;

		attribDesc.push_back (initializers::vertexInputAttributeDescription (0, 0, vertSize, offset));
		offset += vertDesc.layout[i] * sizeof (float);
	}
}

VulkanMesh::VulkanMesh (
    VertexLayout const& vertLayout, VulkanBuffer&& vertices, VulkanBuffer&& indices, uint32_t index_count)
: vertLayout (vertLayout), vertices (std::move (vertices)), indices (std::move (indices)), index_count (index_count)
{
}

ModelManager::ModelManager (Resource::Mesh::Manager& mesh_manager, VulkanDevice& device, AsyncTaskManager& async_task_man)
: mesh_manager (mesh_manager), device (device), async_task_man (async_task_man)
{
}

ModelID ModelManager::CreateModel (Resource::Mesh::MeshData const& meshData)
{
	std::lock_guard lg (map_lock);

	uint32_t vertexCount = static_cast<uint32_t> (meshData.vertexData.size ());
	uint32_t vertexElementCount = static_cast<uint32_t> (meshData.desc.ElementCount ());
	uint32_t indexCount = static_cast<uint32_t> (meshData.indexData.size ());

	uint32_t vBufferSize = (vertexCount) * sizeof (float);
	uint32_t iBufferSize = (indexCount) * sizeof (uint32_t);

	ModelID new_id = counter++;

	auto model = VulkanMesh (VertexLayout (meshData.desc),
	    VulkanBuffer (device, vertex_details (vertexCount, vertexElementCount)),
	    VulkanBuffer (device, vertex_details (vertexCount, vertexElementCount)),
	    indexCount);
	auto loading_model = VulkanMesh (VertexLayout (meshData.desc),
	    VulkanBuffer (device, staging_details (BufferType::vertex, vBufferSize)),
	    VulkanBuffer (device, staging_details (BufferType::index, iBufferSize)),
	    indexCount);

	loading_model.vertices.CopyToBuffer (meshData.vertexData);
	loading_model.indices.CopyToBuffer (meshData.indexData);

	VkBuffer vStage = loading_model.vertices.buffer;
	VkBuffer iStage = loading_model.indices.buffer;

	VkBuffer vBuff = model.vertices.buffer;
	VkBuffer iBuff = model.indices.buffer;

	staging_models.emplace (new_id, std::move (loading_model));
	models.emplace (new_id, std::move (model));


	std::function<void(const VkCommandBuffer)> work =
	    [vStage, iStage, vBuff, iBuff, vBufferSize, iBufferSize](const VkCommandBuffer copyCmd) {
		    VkBufferCopy copyRegion{};
		    copyRegion.size = vBufferSize;
		    vkCmdCopyBuffer (copyCmd, vStage, vBuff, 1, &copyRegion);

		    copyRegion.size = iBufferSize;
		    vkCmdCopyBuffer (copyCmd, iStage, iBuff, 1, &copyRegion);
	    };

	AsyncTask transfer;
	transfer.type = TaskType::transfer;
	transfer.work = work;
	transfer.finish_work = [&] { this->FinishModelUpload (new_id); };

	async_task_man.SubmitTask (std::move (transfer));
	return new_id;
}
void ModelManager::FreeModel (ModelID id)
{
	std::lock_guard lg (map_lock);
	models.erase (id);
}

void ModelManager::FinishModelUpload (ModelID id)
{
	std::lock_guard lg (map_lock);

	staging_models.erase (id);
}

bool ModelManager::IsUploaded (ModelID id)
{
	std::lock_guard lg (map_lock);
	return staging_models.count (id) == 0;
}

VertexLayout ModelManager::GetLayout (ModelID id) { return models.at (id).vertLayout; }

void ModelManager::Bind (VkCommandBuffer cmdBuf, ModelID id)
{
	std::lock_guard lg (map_lock);

	// finished uploading
	if (staging_models.count (id) == 0)
	{
		models.at (id).vertices.BindVertexBuffer (cmdBuf);
		models.at (id).indices.BindIndexBuffer (cmdBuf);
	}
}
void ModelManager::DrawIndexed (VkCommandBuffer cmdBuf, ModelID id)
{
	Bind (cmdBuf, id);
	vkCmdDrawIndexed (cmdBuf, static_cast<uint32_t> (models.at (id).index_count), 1, 0, 0, 0);
}
