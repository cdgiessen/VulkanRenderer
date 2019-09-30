#include "AsyncTask.h"

CommandPoolGroup::CommandPoolGroup (VulkanDevice& device)
: graphics_pool (device, device.GraphicsQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  transfer_pool (device, device.TransferQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  compute_pool (device, device.ComputeQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
{
}


AsyncTaskManager::AsyncTaskManager (VulkanDevice& device) : device (device)
{
	unsigned int thread_count = HardwareThreadCount ();
	for (unsigned int i = 0; i < thread_count; i++)
	{
		pools.emplace_back (device);
	}
}

AsyncTaskManager::~AsyncTaskManager () {}

void AsyncTaskManager::SubmitTask (TaskType type, AsyncTask&& task)
{
	switch (type)
	{
		case (TaskType::transfer):
			SubmitWork (std::move (task), pools.at (0).transfer_pool);

			break;
		case (TaskType::compute):
			SubmitWork (std::move (task), pools.at (0).compute_pool);

			break;
		default:
			SubmitWork (std::move (task), pools.at (0).graphics_pool);
	}
}



void AsyncTaskManager::SubmitWork (AsyncTask&& task, CommandPool& pool)
{
	CommandBuffer cmdBuf = CommandBuffer (pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	std::shared_ptr<VulkanFence> fence = std::make_shared<VulkanFence> (device);

	cmdBuf.Allocate ();
	cmdBuf.Begin (VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	cmdBuf.WriteTo (task.work);
	cmdBuf.SetFence (fence);
	cmdBuf.End ();
	cmdBuf.Submit (task.wait_sems, task.signal_sems);


	std::lock_guard lk (lock_finish_queue);
	finish_queue.emplace_back (cmdBuf, task.finish_work);
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