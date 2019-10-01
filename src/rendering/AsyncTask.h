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
	std::shared_ptr<job::TaskSignal> signal;
	TaskType type;
	std::function<void(const VkCommandBuffer)> work;
	std::vector<std::shared_ptr<VulkanSemaphore>> wait_sems;
	std::vector<std::shared_ptr<VulkanSemaphore>> signal_sems;
	std::function<void()> finish_work;
};

struct GraphicsCleanUpWork
{
	CommandBuffer cmdBuf;
	std::function<void()> finish_work;

	GraphicsCleanUpWork (CommandBuffer cmdBuf, std::function<void()> finish_work)
	: cmdBuf (cmdBuf), finish_work (finish_work)
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
	AsyncTaskManager (VulkanDevice& device);
	~AsyncTaskManager ();

	void SubmitTask (TaskType type, AsyncTask&& task);

	void CleanFinishQueue ();

	private:
	VulkanDevice& device;

	std::unordered_map<std::thread::id, std::unique_ptr<CommandPoolGroup>> pools;

	std::mutex lock_finish_queue;
	std::vector<GraphicsCleanUpWork> finish_queue;
};