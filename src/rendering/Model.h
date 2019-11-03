#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "Buffer.h"

namespace Resource::Mesh
{
class Manager;
}
class VulkanDevice;
class AsyncTaskManager;

struct VertexLayout
{
	VertexLayout (VertexDescription desc);

	const std::vector<VkVertexInputBindingDescription> bindingDesc;
	const std::vector<VkVertexInputAttributeDescription> attribDesc;
};
struct VulkanModel
{
	VulkanModel (VertexLayout vertLayout, VulkanBuffer&& vertices, VulkanBuffer&& indices);

	void BindModel (VkCommandBuffer cmdBuf);

	VertexLayout vertLayout;
	VulkanBuffer vertices;
	VulkanBuffer indices;
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

	ModelID CreateModel (Resource::Mesh::Manager mesh_id);

	ModelID CreateModel (MeshData const& meshData);
	void FreeModel (ModelID id);
	bool IsUploaded (ModelID id);

	void Bind (ModelID id, VkCommandBuffer cmdBuf);

	private:
	void FinishModelUpload (ModelID id);

	VulkanDevice& device;
	Resource::Mesh::Manager& mesh_manager;
	AsyncTaskManager& async_task_man;

	std::mutex map_lock;
	ModelID counter = 0;
	std::unordered_map<ModelID, VulkanModel> staging_models;
	std::unordered_map<ModelID, VulkanModel> models;
};