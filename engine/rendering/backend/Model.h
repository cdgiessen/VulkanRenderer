#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "SG14/flat_map.h"

#include "resources/Mesh.h"

#include "Buffer.h"

class VulkanDevice;
class AsyncTaskManager;

struct VertexLayout
{
	VertexLayout (Resource::Mesh::VertexDescription const& desc);

	std::vector<VkVertexInputBindingDescription> bindingDesc;
	std::vector<VkVertexInputAttributeDescription> attribDesc;
};
struct VulkanMesh
{
	VulkanMesh (VertexLayout const& vertLayout, VulkanBuffer&& vertices, VulkanBuffer&& indices, uint32_t index_count);

	VertexLayout vertLayout;
	VulkanBuffer vertices;
	VulkanBuffer indices;
	uint32_t index_count;
};


using ModelID = int;

class ModelManager
{
	public:
	ModelManager (Resource::Mesh::Manager& mesh_manager, VulkanDevice& device, AsyncTaskManager& async_task_man);

	ModelManager (ModelManager const& man) = delete;
	ModelManager& operator= (ModelManager const& man) = delete;
	ModelManager (ModelManager&& man) = delete;
	ModelManager& operator= (ModelManager&& man) = delete;

	ModelID CreateModel (Resource::Mesh::MeshID mesh_id);

	ModelID CreateModel (Resource::Mesh::MeshData const& meshData);
	void FreeModel (ModelID id);
	bool IsUploaded (ModelID id);

	VertexLayout GetLayout (ModelID id);

	void Bind (VkCommandBuffer cmdBuf, ModelID id);

	void DrawIndexed (VkCommandBuffer cmdBuf, ModelID id);

	private:
	void FinishModelUpload (ModelID id);

	Resource::Mesh::Manager& mesh_manager;
	VulkanDevice& device;
	AsyncTaskManager& async_task_man;

	std::mutex map_lock;
	ModelID counter = 0;
	stdext::flat_map<ModelID, VulkanMesh> staging_models;
	stdext::flat_map<ModelID, VulkanMesh> models;
};