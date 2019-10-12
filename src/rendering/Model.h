#pragma once

#include <mutex>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include "resources/Mesh.h"

#include "AsyncTask.h"
#include "Buffer.h"


class VertexLayout
{
	public:
	VertexLayout (VertexDescription desc)
	: bindingDesc (getBindingDescription (desc)), attribDesc (getAttributeDescriptions (desc))
	{
	}

	std::vector<VkVertexInputBindingDescription> bindingDesc;
	std::vector<VkVertexInputAttributeDescription> attribDesc;

	private:
	std::vector<VkVertexInputBindingDescription> getBindingDescription (VertexDescription vertDesc);
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions (VertexDescription vertDesc);
};

class VulkanModel
{
	public:
	VulkanModel (VulkanDevice& device,
	    AsyncTaskManager& async_task_man,
	    BufferManager& buf_man,
	    std::unique_ptr<MeshData> meshData);

	uint32_t vertexCount = 0;
	uint32_t vertexElementCount = 0;
	uint32_t indexCount = 0;

	std::unique_ptr<VulkanBuffer> vmaVertices;
	std::unique_ptr<VulkanBuffer> vmaIndicies;

	std::unique_ptr<VulkanBuffer> vertexStagingBuffer;
	std::unique_ptr<VulkanBuffer> indexStagingBuffer;

	void BindModel (VkCommandBuffer cmdBuf);

	VertexLayout GetVertexLayout ();

	private:
	VertexLayout vertLayout;
};

using ModelID = int;

class ModelManager
{
	public:
	ModelManager (Resource::Mesh::Manager& mesh_manager,
	    VulkanDevice& device,
	    AsyncTaskManager& async_task_man,
	    BufferManager& buf_man);

	ModelManager (ModelManager const& man) = delete;
	ModelManager& operator= (ModelManager const& man) = delete;


	ModelID CreateModel (std::unique_ptr<MeshData> meshData);
	void FreeModel (ModelID id);

	VulkanModel& GetModel (ModelID id);

	private:
	VulkanDevice& device;
	Resource::Mesh::Manager& mesh_manager;
	AsyncTaskManager& async_task_man;
	BufferManager& buf_man;

	std::mutex map_lock;
	std::unordered_map<ModelID, VulkanModel> models;
};