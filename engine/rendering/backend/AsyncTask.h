#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "core/JobSystem.h"

#include "Wrappers.h"

class VulkanDevice;

enum class TaskType
{
	graphics,
	transfer,
	compute
};

struct AsyncTask
{
	TaskType type = TaskType::graphics;
	std::function<void (const VkCommandBuffer)> work;
	std::function<void ()> finish_work;
};

struct GraphicsCleanUpWork
{
	CommandBuffer cmdBuf;
	std::function<void ()> finish_work;

	GraphicsCleanUpWork (CommandBuffer&& cmdBuf, std::function<void ()> finish_work)
	: cmdBuf (std::move (cmdBuf)), finish_work (finish_work)
	{
	}
};

class CommandPoolGroup
{
	public:
	CommandPoolGroup (VulkanDevice const& device);

	CommandPool graphics_pool;
	CommandPool transfer_pool;
	CommandPool compute_pool;
};

class AsyncTaskQueue
{
	public:
	AsyncTaskQueue (job::ThreadPool& thread_pool, VulkanDevice& device);
	~AsyncTaskQueue ();

	AsyncTaskQueue (AsyncTaskQueue const& man) = delete;
	AsyncTaskQueue& operator= (AsyncTaskQueue const& man) = delete;
	AsyncTaskQueue (AsyncTaskQueue&& man) = delete;
	AsyncTaskQueue& operator= (AsyncTaskQueue&& man) = delete;


	void SubmitTask (AsyncTask&& task);

	void CleanFinishQueue ();

	private:
	job::ThreadPool& thread_pool;

	std::unordered_map<std::thread::id, std::unique_ptr<CommandPoolGroup>> pools;

	std::mutex in_progress;
	std::vector<std::shared_ptr<job::TaskSignal>> tasks_in_progress;

	std::mutex lock_finish_queue;
	std::vector<GraphicsCleanUpWork> finish_queue;
};