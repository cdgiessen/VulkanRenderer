#include "AsyncTask.h"

AsyncTaskManager::AsyncTaskManager (VulkanDevice& device)
: device (device),
  graphics_pool (device, device.GraphicsQueue ()),
  transfer_pool (device, device.TransferQueue ()),
  compute_pool (device, device.ComputeQueue ())
{
}

AsyncTaskManager::~AsyncTaskManager () {}

void AsyncTaskManager::SubmitGraphicsTask (AsyncTask&& task)
{
	SubmitWork (std::move (task), graphics_pool);
}
void AsyncTaskManager::SubmitTransferTask (AsyncTask&& task)
{
	SubmitWork (std::move (task), transfer_pool);
}
void AsyncTaskManager::SubmitComputeTask (AsyncTask&& task)
{
	SubmitWork (std::move (task), compute_pool);
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
	finish_queue.push_back (GraphicsCleanUpWork (cmdBuf, task.buffersToClean, task.signals));
}

void AsyncTaskManager::CleanFinishQueue ()
{
	std::lock_guard lk (lock_finish_queue);

	for (auto it = finish_queue.begin (); it != finish_queue.end ();)
	{
		if (it->cmdBuf.GetFence ().Check ())
		{
			for (auto& sig : it->signals)
			{
				if (sig != nullptr) *sig = true;
			}
			it->cmdBuf.Free ();
			it = finish_queue.erase (it);
		}
		else
		{
			++it;
		}
	}
}