#include "JobSystem.h"

#include "Logger.h"

namespace job
{

Task::Task (TaskType type, std::weak_ptr<TaskSignal> signalBlock)
: type (type), signalBlock (signalBlock)
{
}

void Task::Add (Job&& newJob) { jobs.push_back (std::move (newJob)); }

void Task::operator() ()
{
	if (auto sbp = signalBlock.lock ())
	{
		for (auto& job : jobs)
			job ();
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

TaskSignal::TaskSignal () {}

TaskSignal::~TaskSignal () {}

void TaskSignal::Signal ()
{
	finished = true;
	condVar.notify_all ();
}

void TaskSignal::Wait ()
{
	if (finished) return;
	std::unique_lock<std::mutex> mlock (condVar_lock);
	condVar.wait (mlock);
}

void TaskSignal::AddTaskToWaitOn (std::shared_ptr<TaskSignal> taskSig)
{
	std::lock_guard<std::mutex> lg (pred_lock);
	predicates.push_back (taskSig);
}

bool TaskSignal::IsReadyToRun ()
{
	std::lock_guard<std::mutex> lg (pred_lock);
	for (auto& task : predicates)
	{
		if (!task->IsReadyToRun ()) return false;
	}
	return true;
}

TaskPool::TaskPool () {}

void TaskPool::AddTask (Task&& task)
{
	std::lock_guard<std::mutex> lg (queueLock);
	tasks.push (std::move (task));
}

std::optional<Task> TaskPool::GetTask ()
{
	std::lock_guard<std::mutex> lg (queueLock);
	while (!tasks.empty ())
	{
		auto val = tasks.front ();
		tasks.pop ();
		if (val.IsReadyToRun ()) return val;
		tasks.push (val); // possible infinite wait...
	}
	return {};
}

TaskManager::TaskManager () {}

void TaskManager::AddTask (Task&& task) { currentFrameTasks.AddTask (std::move (task)); }

std::optional<Task> TaskManager::GetTask () { return currentFrameTasks.GetTask (); }

Worker::Worker (TaskManager& taskMan) : taskMan (taskMan), workerThread{ &Worker::Work, this } {}

Worker::~Worker ()
{
	Stop ();
	workerThread.join ();
}
void Worker::Stop () { isWorking = false; }

void Worker::Work ()
{
	while (isWorking)
	{
		// taskMan.WaitOn();
		auto task = taskMan.GetTask ();
		while (task.has_value ())
		{
			(*task) ();
			task = taskMan.GetTask ();
		}
	}
}

WorkerPool::WorkerPool (TaskManager& taskMan, int workerCount)
: taskMan (taskMan), workerCount (workerCount)
{
}

void WorkerPool::StartWorkers ()
{
	if (workerCount > 0)
	{
		for (int i = 0; i < workerCount; i++)
		{
			workers.push_back (std::make_unique<Worker> (taskMan));
		}
	}
}
void WorkerPool::StopWorkers ()
{
	for (auto& worker : workers)
	{
		worker->Stop ();
	}
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
			//				Log::Debug << n << " ";
		}
		Log.Debug (fmt::format ("\n"));
		//			Log::Debug << "\n";
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

	//	Log::Debug << "Job system test: Start\n";
	TaskManager tMan;

	WorkerPool workerPool (tMan, 2);
	workerPool.StartWorkers ();

	JobTesterClass jtc;
	jtc.Print ();

	std::vector<Job> js1, js2;
	int jobCount = 1000;
	for (int i = 0; i < jobCount; i++)
		js1.push_back ({ [&]() { jtc.AddNum (1); } });
	for (int i = 0; i < jobCount; i++)
		js2.push_back ({ [&]() { jtc.MulNumAddNum (2, 0); } });

	auto signal1 = std::make_shared<TaskSignal> ();
	Task t1 = Task (TaskType::currentFrame, signal1);

	auto signal2 = std::make_shared<TaskSignal> ();
	signal2->AddTaskToWaitOn (signal1);
	Task t2 = Task (TaskType::currentFrame, signal2);


	for (int i = 0; i < jobCount; i++)
		t1.Add (std::move (js1.at (i)));

	for (int i = 0; i < jobCount; i++)
		t2.Add (std::move (js2.at (i)));

	tMan.AddTask (std::move (t1));
	tMan.AddTask (std::move (t2));


	workerPool.StopWorkers ();
	signal2->Wait ();
	jtc.Print ();
	Log.Debug (fmt::format ("Job system test: done\n"));

	//	Log::Debug << "Job system test: done\n";
	return true;
}
} // namespace job