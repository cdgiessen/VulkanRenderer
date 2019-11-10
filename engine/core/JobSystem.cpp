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

void TaskSignal::Notify () { active_waiters++; }

void TaskSignal::Signal ()
{
	active_waiters--;
	if (active_waiters == 0) condVar.notify_all ();
}

void TaskSignal::Wait ()
{
	if (active_waiters == 0) return;
	std::unique_lock mlock (condVar_lock);
	condVar.wait (mlock);
}

void TaskSignal::WaitOn (std::shared_ptr<TaskSignal> taskSig)
{
	std::lock_guard lg (pred_lock);
	predicates.push_back (taskSig);
}

void TaskSignal::Cancel () { cancelled = true; }

bool TaskSignal::IsCancelled () { return cancelled; }

bool TaskSignal::IsReadyToRun ()
{
	std::lock_guard<std::mutex> lg (pred_lock);
	for (auto& task : predicates)
	{
		if (!task->IsReadyToRun ()) return false;
	}
	return true;
}

int TaskSignal::InQueue () { return active_waiters; }

// Task

Task::Task (WorkFuncSig&& m_job, std::weak_ptr<TaskSignal> signalBlock)
: m_job (m_job), signalBlock (signalBlock)
{
	if (auto sbp = signalBlock.lock ()) sbp->Notify ();
}

void Task::Run ()
{
	if (auto sbp = signalBlock.lock ())
	{
		if (!sbp->IsCancelled ())
		{
			m_job ();
		}
		sbp->Signal ();
	}
}

void Task::WaitOn ()
{
	if (auto sbp = signalBlock.lock ()) sbp->Wait ();
}

bool Task::IsReadyToRun ()
{
	if (auto sbp = signalBlock.lock ())
	{
		return sbp->IsReadyToRun ();
	}
	return true; // Is this right? no signal block to wait on
}

// TaskManager

TaskManager::TaskManager ()
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
				auto task = GetTask ();
				while (task.has_value ())
				{
					task.value ().Run ();
					task = GetTask ();
				}
			}
		}));
	}
}


TaskManager::~TaskManager ()
{
	Stop ();
	for (auto& thread : worker_threads)
	{
		thread.join ();
	}
}


void TaskManager::Stop ()
{
	continue_working = false;
	{
		std::lock_guard lg (workSubmittedLock);
		workSubmittedCondVar.notify_all ();
	}
}

void TaskManager::Submit (WorkFuncSig&& job, std::weak_ptr<TaskSignal> signal_block)
{
	{
		std::lock_guard lg (queue_lock);
		task_queue.push (Task{ std::move (job), signal_block });
	}

	workSubmittedCondVar.notify_one ();
}

void TaskManager::Submit (std::vector<Task> in_tasks)
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

std::optional<Task> TaskManager::GetTask ()
{
	std::lock_guard lg (queue_lock);
	// Log.Debug (fmt::format ("queue size {}\n", task_queue.size ()));
	while (!task_queue.empty ())
	{
		auto val = task_queue.front ();
		task_queue.pop ();
		if (val.IsReadyToRun ()) return val;

		task_queue.push (val); // possible infinite wait...
	}
	return {};
}

std::vector<std::thread::id> TaskManager::GetThreadIDs ()
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
			Log.Debug (fmt::format ("{} ", n));
			//				Log.Debug << n << " ";
		}
		Log.Debug (fmt::format ("\n"));
		//			Log.Debug << "\n";
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
	Log.Debug (fmt::format ("Job system test: start\n"));

	//	Log.Debug << "Job system test: Start\n";
	TaskManager tMan;

	JobTesterClass jtc;
	jtc.Print ();

	int jobCount = 1000;

	auto signal1 = std::make_shared<TaskSignal> ();
	auto t1 = [&] {
		for (int i = 0; i < jobCount; i++)
			jtc.AddNum (1);
	};

	auto signal2 = std::make_shared<TaskSignal> ();

	signal2->WaitOn (signal1);
	auto t2 = [&] {
		for (int i = 0; i < jobCount; i++)
			jtc.MulNumAddNum (2, 0);
	};

	tMan.Submit (std::move (t1), signal1);
	tMan.Submit (std::move (t2), signal2);

	signal2->Wait ();
	jtc.Print ();
	Log.Debug (fmt::format ("Job system test: done\n"));

	//	Log.Debug << "Job system test: done\n";
	return true;
}
} // namespace job