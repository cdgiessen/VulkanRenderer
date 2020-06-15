#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "resources/Mesh.h"

#include "Buffer.h"

class VulkanDevice;
class AsyncTaskQueue;

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

class Models
{
	public:
	Models (Resource::Mesh::Meshes& meshes, VulkanDevice& device, AsyncTaskQueue& async_task_man);

	Models (Models const& man) = delete;
	Models& operator= (Models const& man) = delete;
	Models (Models&& man) = delete;
	Models& operator= (Models&& man) = delete;

	ModelID create_model (Resource::Mesh::MeshID mesh_id);

	ModelID create_model (Resource::Mesh::MeshData const& meshData);
	void free_model (ModelID id);
	bool is_uploaded (ModelID id);

	VertexLayout get_layout (ModelID id);

	void bind (VkCommandBuffer cmdBuf, ModelID id);

	void draw_indexed (VkCommandBuffer cmdBuf, ModelID id);

	private:
	void finished_model_upload (ModelID id);

	Resource::Mesh::Meshes& meshes;
	VulkanDevice& device;
	AsyncTaskQueue& async_task_man;

	std::mutex map_lock;
	ModelID counter = 0;
	std::unordered_map<ModelID, VulkanMesh> staging_models;
	std::unordered_map<ModelID, VulkanMesh> models;
};