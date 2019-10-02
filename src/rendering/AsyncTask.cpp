#include "AsyncTask.h"
#include "core/Logger.h"

CommandPoolGroup::CommandPoolGroup (VulkanDevice& device)
: graphics_pool (device, device.GraphicsQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  transfer_pool (device, device.TransferQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  compute_pool (device, device.ComputeQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
{
}


AsyncTaskManager::AsyncTaskManager (job::TaskManager& task_manager, VulkanDevice& device)
: task_manager (task_manager), device (device)
{
	auto thread_ids = task_manager.GetThreadIDs ();
	thread_ids.push_back (std::this_thread::get_id ());
	for (auto& id : thread_ids)
	{
		auto group = std::make_unique<CommandPoolGroup> (device);
		pools.insert ({ id, std::move (group) });
	}
}

AsyncTaskManager::~AsyncTaskManager () {}

void AsyncTaskManager::SubmitTask (TaskType type, AsyncTask task)
{
	task_manager.Submit (job::Task ([type, &task, this] {
		Log.Debug ("started render task\n");
		std::thread::id id = std::this_thread::get_id ();
		CommandPool* pool;
		switch (type)
		{
			case (TaskType::transfer):
				pool = &(pools.at (id)->transfer_pool);
				break;
			case (TaskType::compute):
				pool = &(pools.at (id)->compute_pool);
				break;
			default:
				pool = &(pools.at (id)->graphics_pool);
		}
		CommandBuffer cmdBuf = CommandBuffer (*pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		std::shared_ptr<VulkanFence> fence = std::make_shared<VulkanFence> (device);

		cmdBuf.Allocate ();
		cmdBuf.Begin (VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		cmdBuf.WriteTo (task.work);
		cmdBuf.SetFence (fence);
		cmdBuf.End ();
		cmdBuf.Submit (task.wait_sems, task.signal_sems);

		std::lock_guard lk (lock_finish_queue);
		finish_queue.emplace_back (cmdBuf, task.finish_work);
		Log.Debug ("Finished render task\n");
	}));
}

void AsyncTaskManager::CleanFinishQueue ()
{
	std::lock_guard lk (lock_finish_queue);

	for (auto it = finish_queue.begin (); it != finish_queue.end ();)
	{
		if (it->cmdBuf.GetFence ().Check ())
		{
			it->finish_work ();
			it->cmdBuf.Free ();
			it = finish_queue.erase (it);
		}
		else
		{
			++it;
		}
	}
}