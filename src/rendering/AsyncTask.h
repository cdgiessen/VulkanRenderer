#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Device.h"
#include "Wrappers.h"

#include "core/JobSystem.h"

enum class TaskType
{
	graphics,
	transfer,
	compute
};

struct AsyncTask
{
	TaskType type = TaskType::graphics;
	std::shared_ptr<job::TaskSignal> signal;
	std::function<void(const VkCommandBuffer)> work;
	std::function<void()> finish_work;
	std::vector<std::shared_ptr<VulkanSemaphore>> wait_sems;
	std::vector<std::shared_ptr<VulkanSemaphore>> signal_sems;
	std::vector<std::shared_ptr<VulkanBuffer>> buffers;
};

struct GraphicsCleanUpWork
{
	CommandBuffer cmdBuf;
	std::function<void()> finish_work;
	std::vector<std::shared_ptr<VulkanBuffer>> buffers;

	GraphicsCleanUpWork (CommandBuffer cmdBuf,
	    std::function<void()> finish_work,
	    std::vector<std::shared_ptr<VulkanBuffer>> buffers)
	: cmdBuf (cmdBuf), finish_work (finish_work), buffers (buffers)
	{
	}
};

class CommandPoolGroup
{
	public:
	CommandPoolGroup (VulkanDevice& device);

	CommandPool graphics_pool;
	CommandPool transfer_pool;
	CommandPool compute_pool;
};

class AsyncTaskManager
{
	public:
	AsyncTaskManager (job::TaskManager& task_manager, VulkanDevice& device);
	~AsyncTaskManager ();

	AsyncTaskManager (AsyncTaskManager const& man) = delete;
	AsyncTaskManager& operator= (AsyncTaskManager const& man) = delete;

	void SubmitTask (AsyncTask const& task);

	void CleanFinishQueue ();

	private:
	job::TaskManager& task_manager;
	VulkanDevice& device;

	std::unordered_map<std::thread::id, std::unique_ptr<CommandPoolGroup>> pools;

	std::mutex in_progress;
	std::vector<std::shared_ptr<job::TaskSignal>> tasks_in_progress;

	std::mutex lock_finish_queue;
	std::vector<GraphicsCleanUpWork> finish_queue;
};