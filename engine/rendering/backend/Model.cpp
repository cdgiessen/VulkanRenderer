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
	std::vector<int> descriptions;
	for (auto& d : vertDesc.layout)
		descriptions.push_back (static_cast<int> (d));
	int size = std::accumulate (std::begin (descriptions), std::end (descriptions), 0) * 4;

	bindingDesc.push_back (initializers::vertexInputBindingDescription (0, size, VK_VERTEX_INPUT_RATE_VERTEX));

	// VkVertexInputAttributeDescription's
	int offset = 0;
	for (size_t i = 0; i < vertDesc.layout.size (); i++)
	{
		VkFormat vertSize = VK_FORMAT_R32_SFLOAT;
		if (vertDesc.layout[i] == Resource::Mesh::VertexType::Vert2)
			vertSize = VK_FORMAT_R32G32_SFLOAT;
		if (vertDesc.layout[i] == Resource::Mesh::VertexType::Vert3)
			vertSize = VK_FORMAT_R32G32B32_SFLOAT;
		if (vertDesc.layout[i] == Resource::Mesh::VertexType::Vert4)
			vertSize = VK_FORMAT_R32G32B32A32_SFLOAT;

		attribDesc.push_back (initializers::vertexInputAttributeDescription (0, 0, vertSize, offset));
		offset += static_cast<int> (vertDesc.layout[i]) * sizeof (float);
	}
}

VulkanMesh::VulkanMesh (
    VertexLayout const& vertLayout, VulkanBuffer&& vertices, VulkanBuffer&& indices, uint32_t index_count)
: vertLayout (vertLayout), vertices (std::move (vertices)), indices (std::move (indices)), index_count (index_count)
{
}

Models::Models (Resource::Mesh::Meshes& meshes, VulkanDevice& device, AsyncTaskQueue& async_task_man)
: meshes (meshes), device (device), async_task_man (async_task_man)
{
}

ModelID Models::CreateModel (Resource::Mesh::MeshData const& meshData)
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

	VkBuffer vStage = loading_model.vertices.Get ();
	VkBuffer iStage = loading_model.indices.Get ();

	VkBuffer vBuff = model.vertices.Get ();
	VkBuffer iBuff = model.indices.Get ();

	staging_models.emplace (new_id, std::move (loading_model));
	models.emplace (new_id, std::move (model));


	std::function<void (const VkCommandBuffer)> work =
	    [vStage, iStage, vBuff, iBuff, vBufferSize, iBufferSize] (const VkCommandBuffer copyCmd) {
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
void Models::FreeModel (ModelID id)
{
	std::lock_guard lg (map_lock);
	models.erase (id);
}

void Models::FinishModelUpload (ModelID id)
{
	std::lock_guard lg (map_lock);

	staging_models.erase (id);
}

bool Models::IsUploaded (ModelID id)
{
	std::lock_guard lg (map_lock);
	return staging_models.count (id) == 0;
}

VertexLayout Models::GetLayout (ModelID id) { return models.at (id).vertLayout; }

void Models::Bind (VkCommandBuffer cmdBuf, ModelID id)
{
	std::lock_guard lg (map_lock);

	// finished uploading
	if (staging_models.count (id) == 0)
	{
		models.at (id).vertices.BindVertexBuffer (cmdBuf);
		models.at (id).indices.BindIndexBuffer (cmdBuf);
	}
}
void Models::DrawIndexed (VkCommandBuffer cmdBuf, ModelID id)
{
	Bind (cmdBuf, id);
	vkCmdDrawIndexed (cmdBuf, static_cast<uint32_t> (models.at (id).index_count), 1, 0, 0, 0);
}
