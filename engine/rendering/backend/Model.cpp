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

	bindingDesc.push_back (initializers::vertex_input_binding_description (0, size, VK_VERTEX_INPUT_RATE_VERTEX));

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

		attribDesc.push_back (initializers::vertex_input_attribute_description (0, 0, vertSize, offset));
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

ModelID Models::create_model (Resource::Mesh::MeshData const& meshData)
{
	std::lock_guard lg (map_lock);

	uint32_t vertexCount = static_cast<uint32_t> (meshData.vertexData.size ());
	uint32_t vertexelement_count = static_cast<uint32_t> (meshData.desc.element_count ());
	uint32_t indexCount = static_cast<uint32_t> (meshData.indexData.size ());

	uint32_t vBufferSize = (vertexCount) * sizeof (float);
	uint32_t iBufferSize = (indexCount) * sizeof (uint32_t);

	ModelID new_id = counter++;

	auto model = VulkanMesh (VertexLayout (meshData.desc),
	    VulkanBuffer (device, vertex_details (vertexCount, vertexelement_count)),
	    VulkanBuffer (device, vertex_details (vertexCount, vertexelement_count)),
	    indexCount);
	auto loading_model = VulkanMesh (VertexLayout (meshData.desc),
	    VulkanBuffer (device, staging_details (BufferType::vertex, vBufferSize)),
	    VulkanBuffer (device, staging_details (BufferType::index, iBufferSize)),
	    indexCount);

	loading_model.vertices.copy_to_buffer (meshData.vertexData);
	loading_model.indices.copy_to_buffer (meshData.indexData);

	VkBuffer vStage = loading_model.vertices.get ();
	VkBuffer iStage = loading_model.indices.get ();

	VkBuffer vBuff = model.vertices.get ();
	VkBuffer iBuff = model.indices.get ();

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
	transfer.finish_work = [&] { this->finished_model_upload (new_id); };

	async_task_man.SubmitTask (std::move (transfer));
	return new_id;
}
void Models::free_model (ModelID id)
{
	std::lock_guard lg (map_lock);
	models.erase (id);
}

void Models::finished_model_upload (ModelID id)
{
	std::lock_guard lg (map_lock);

	staging_models.erase (id);
}

bool Models::is_uploaded (ModelID id)
{
	std::lock_guard lg (map_lock);
	return staging_models.count (id) == 0;
}

VertexLayout Models::get_layout (ModelID id) { return models.at (id).vertLayout; }

void Models::bind (VkCommandBuffer cmdBuf, ModelID id)
{
	std::lock_guard lg (map_lock);

	// finished uploading
	if (staging_models.count (id) == 0)
	{
		models.at (id).vertices.bind_vertex_buffer (cmdBuf);
		models.at (id).indices.bind_index_buffer (cmdBuf);
	}
}
void Models::draw_indexed (VkCommandBuffer cmdBuf, ModelID id)
{
	bind (cmdBuf, id);
	vkCmdDrawIndexed (cmdBuf, static_cast<uint32_t> (models.at (id).index_count), 1, 0, 0, 0);
}
