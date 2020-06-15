#include "AsyncTask.h"

#include <algorithm>

#include "core/Logger.h"

#include "Device.h"

CommandPoolGroup::CommandPoolGroup (VulkanDevice const& device)
: graphics_pool (device.device, device.graphics_queue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  transfer_pool (device.device, device.transfer_queue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  compute_pool (device.device, device.compute_queue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
{
}


AsyncTaskQueue::AsyncTaskQueue (job::ThreadPool& thread_pool, VulkanDevice& device)
: thread_pool (thread_pool)
{
	auto thread_ids = thread_pool.get_thread_ids ();
	thread_ids.push_back (std::this_thread::get_id ());
	for (auto& id : thread_ids)
	{
		pools[id] = std::make_unique<CommandPoolGroup> (device);
		//		pools.emplace (std::piecewise_construct, std::forward_as_tuple (id), std::forward_as_tuple (device));
	}
}

AsyncTaskQueue::~AsyncTaskQueue () {}

void AsyncTaskQueue::SubmitTask (AsyncTask&& task)
{
	auto ts = std::make_shared<job::TaskSignal> ();
	{
		std::lock_guard lg (in_progress);
		tasks_in_progress.push_back (ts);
	}

	thread_pool.submit (
	    [&, task = std::move (task)] {
		    std::thread::id id = std::this_thread::get_id ();
		    CommandPool* pool;
		    switch (task.type)
		    {
			    case (TaskType::transfer): pool = &(pools.at (id)->transfer_pool); break;
			    case (TaskType::compute): pool = &(pools.at (id)->compute_pool); break;
			    default: pool = &(pools.at (id)->graphics_pool);
		    }
		    CommandBuffer cmdBuf = CommandBuffer (*pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		    cmdBuf.allocate ().begin (VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		    cmdBuf.write (task.work);
		    cmdBuf.end ().submit ();
		    {
			    std::lock_guard lk (lock_finish_queue);
			    finish_queue.emplace_back (std::move (cmdBuf), task.finish_work);
		    }
		    {
			    std::lock_guard lk (in_progress);
			    auto f = std::find (std::begin (tasks_in_progress), std::end (tasks_in_progress), ts);
			    if (f != std::end (tasks_in_progress))
			    {
				    tasks_in_progress.erase (f);
			    }
		    }
	    },
	    ts);
}

void AsyncTaskQueue::CleanFinishQueue ()
{
	std::lock_guard lk (lock_finish_queue);

	for (auto it = finish_queue.begin (); it != finish_queue.end ();)
	{
		if (it->cmdBuf.get_fence ().check ())
		{
			it->cmdBuf.free ();
			if (it->finish_work)
			{
				it->finish_work ();
			}
			it = finish_queue.erase (it);
		}
		else
		{
			++it;
		}
	}
}