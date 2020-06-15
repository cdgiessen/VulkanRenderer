#include "JobSystem.h"

#include "Logger.h"

unsigned int HardwareThreadCount ()
{
	unsigned int concurentThreadsSupported = std::thread::hardware_concurrency ();
	return concurentThreadsSupported > 0 ? concurentThreadsSupported : 1;
}

namespace job
{

// TaskSignal

void TaskSignal::notify () { active_waiters++; }

void TaskSignal::signal ()
{
	active_waiters--;
	if (active_waiters == 0) condVar.notify_all ();
}

void TaskSignal::wait ()
{
	if (active_waiters == 0) return;
	std::unique_lock mlock (condVar_lock);
	condVar.wait (mlock);
}

void TaskSignal::wait_on (std::shared_ptr<TaskSignal> taskSig)
{
	std::lock_guard lg (pred_lock);
	predicates.push_back (taskSig);
}

void TaskSignal::cancel () { cancelled = true; }

bool TaskSignal::is_cancelled () { return cancelled; }

bool TaskSignal::is_ready_to_run ()
{
	std::lock_guard<std::mutex> lg (pred_lock);
	for (auto& task : predicates)
	{
		if (!task->is_ready_to_run ()) return false;
	}
	return true;
}

int TaskSignal::in_queue () { return active_waiters; }

// Task

Task::Task (WorkFuncSig&& m_job, std::weak_ptr<TaskSignal> signalBlock)
: m_job (m_job), signalBlock (signalBlock)
{
	if (auto sbp = signalBlock.lock ()) sbp->notify ();
}

void Task::run ()
{
	if (auto sbp = signalBlock.lock ())
	{
		if (!sbp->is_cancelled ())
		{
			m_job ();
		}
		sbp->signal ();
	}
}

void Task::wait_on ()
{
	if (auto sbp = signalBlock.lock ()) sbp->wait ();
}

bool Task::is_ready_to_run ()
{
	if (auto sbp = signalBlock.lock ())
	{
		return sbp->is_ready_to_run ();
	}
	return true; // Is this right? no signal block to wait on
}

// ThreadPool

ThreadPool::ThreadPool ()
{
	int thread_count = static_cast<int> (HardwareThreadCount ());
	for (int i = 0; i < thread_count; i++)
	{
		worker_threads.emplace_back (std::thread ([this] {
			while (continue_working)
			{
				{
					std::unique_lock lock (workSubmittedLock);
					workSubmittedCondVar.wait (lock);
				}
				auto task = get_task ();
				while (task.has_value ())
				{
					task.value ().run ();
					task = get_task ();
				}
			}
		}));
	}
}


ThreadPool::~ThreadPool ()
{
	stop ();
	for (auto& thread : worker_threads)
	{
		thread.join ();
	}
}


void ThreadPool::stop ()
{
	continue_working = false;
	{
		std::lock_guard lg (workSubmittedLock);
		workSubmittedCondVar.notify_all ();
	}
}

void ThreadPool::submit (WorkFuncSig&& job, std::weak_ptr<TaskSignal> signal_block)
{
	{
		std::lock_guard lg (queue_lock);
		task_queue.push (Task{ std::move (job), signal_block });
	}

	workSubmittedCondVar.notify_one ();
}

void ThreadPool::submit (std::vector<Task> in_tasks)
{
	{
		std::lock_guard lg (queue_lock);
		for (auto& t : in_tasks)
		{
			task_queue.push (t);
		}
	}
	workSubmittedCondVar.notify_all ();
}

std::optional<Task> ThreadPool::get_task ()
{
	std::lock_guard lg (queue_lock);
	// Log.debug (fmt::format ("queue size {}", task_queue.size ()));
	while (!task_queue.empty ())
	{
		auto val = task_queue.front ();
		task_queue.pop ();
		if (val.is_ready_to_run ()) return val;

		task_queue.push (val); // possible infinite wait...
	}
	return {};
}

std::vector<std::thread::id> ThreadPool::get_thread_ids ()
{
	std::vector<std::thread::id> ids;
	for (auto& thread : worker_threads)
	{
		ids.push_back (thread.get_id ());
	}
	return ids;
}

class JobTesterClass
{
	public:
	void AddOne ();
	void AddNum (int num);
	void MulTwo ();
	void MulNumAddNum (int num1, int num2);

	void Print ()
	{
		for (auto& n : nums)
		{
			Log.debug (fmt::format ("{} ", n));
		}
	}

	private:
	std::vector<int> nums = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
};

void JobTesterClass::AddNum (int num)
{
	for (auto& n : nums)
		n += num;
}

void JobTesterClass::AddOne ()
{
	for (auto& n : nums)
		n += 1;
}

void JobTesterClass::MulTwo ()
{
	for (auto& n : nums)
		n *= 2;
}

void JobTesterClass::MulNumAddNum (int num1, int num2)
{
	for (auto& n : nums)
		n += n * num1 + num2;
}

bool JobTester ()
{
	Log.debug (fmt::format ("Job system test: start"));

	ThreadPool tMan;

	JobTesterClass jtc;
	jtc.Print ();

	int jobCount = 1000;

	auto signal1 = std::make_shared<TaskSignal> ();
	auto t1 = [&] {
		for (int i = 0; i < jobCount; i++)
			jtc.AddNum (1);
	};

	auto signal2 = std::make_shared<TaskSignal> ();

	signal2->wait_on (signal1);
	auto t2 = [&] {
		for (int i = 0; i < jobCount; i++)
			jtc.MulNumAddNum (2, 0);
	};

	tMan.submit (std::move (t1), signal1);
	tMan.submit (std::move (t2), signal2);

	signal2->wait ();
	jtc.Print ();
	Log.debug (fmt::format ("Job system test: done"));
	return true;
}
} // namespace job