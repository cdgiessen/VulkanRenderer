#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "Wrappers.h"

#include "Device.h"

using Signal = std::shared_ptr<bool>;

enum class WorkType
{
	graphics,
	transfer,
	compute,
};

struct GraphicsCleanUpWork
{
	CommandBuffer cmdBuf;
	std::vector<std::shared_ptr<VulkanBuffer>> buffers;
	std::vector<Signal> signals;

	explicit GraphicsCleanUpWork (
	    CommandBuffer cmdBuf, std::vector<std::shared_ptr<VulkanBuffer>>& buffers, std::vector<Signal>& signals)
	: cmdBuf (cmdBuf), buffers (buffers), signals (signals)
	{
	}
};

struct AsyncTask
{
	std::function<void(const VkCommandBuffer)> work;
	std::vector<std::shared_ptr<VulkanSemaphore>> wait_sems;
	std::vector<std::shared_ptr<VulkanSemaphore>> signal_sems;
	std::vector<std::shared_ptr<VulkanBuffer>> buffersToClean;
	std::vector<Signal> signals;
};

class AsyncTaskManager
{
	public:
	AsyncTaskManager (VulkanDevice& device);
	~AsyncTaskManager ();

	void SubmitGraphicsTask (AsyncTask&& task);
	void SubmitTransferTask (AsyncTask&& task);
	void SubmitComputeTask (AsyncTask&& task);

	void CleanFinishQueue ();

	private:
	void SubmitWork (AsyncTask&& task, CommandPool& pool);

	VulkanDevice& device;

	std::mutex lock_graphics;
	std::mutex lock_transfer;
	std::mutex lock_compute;

	CommandPool graphics_pool;
	CommandPool transfer_pool;
	CommandPool compute_pool;

	std::mutex lock_finish_queue;
	std::vector<GraphicsCleanUpWork> finish_queue;
};