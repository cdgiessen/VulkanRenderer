#include "JobSystem.h"

#include "Logger.h"

job::TaskManager taskManager;

namespace job
{

Task::Task (std::weak_ptr<TaskSignal> signalBlock, std::function<void()>&& m_job)
: m_job (m_job), signalBlock (signalBlock)
{
	if (auto sbp = signalBlock.lock ()) sbp->Notify ();
}

void Task::operator() ()
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

TaskSignal::TaskSignal () {}

TaskSignal::~TaskSignal () {}

void TaskSignal::Notify () { active_waiters++; }

void TaskSignal::Signal ()
{
	active_waiters--;
	if (active_waiters == 0) condVar.notify_all ();
}

void TaskSignal::Wait ()
{
	if (active_waiters == 0) return;
	std::unique_lock<std::mutex> mlock (condVar_lock);
	condVar.wait (mlock);
}

void TaskSignal::WaitOn (std::shared_ptr<TaskSignal> taskSig)
{
	std::lock_guard<std::mutex> lg (pred_lock);
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

TaskPool::TaskPool () {}

void TaskPool::AddTask (Task&& task)
{
	std::lock_guard<std::mutex> lg (queueLock);
	tasks.push (std::move (task));
}

void TaskPool::AddTasks (std::vector<Task> in_tasks)
{
	std::lock_guard<std::mutex> lg (queueLock);
	for (auto& t : in_tasks)
	{
		tasks.push (std::move (t));
	}
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

bool TaskPool::HasTasks () { return !tasks.empty (); }


TaskManager::TaskManager () {}


void TaskManager::Submit (Task&& task, TaskType type)
{
	currentFrameTasks.AddTask (std::move (task));
	workSubmittedCondVar.notify_one ();
	return;

	switch (type)
	{
		default:
		case (TaskType::currentFrame):
			currentFrameTasks.AddTask (std::move (task));
			break;
		case (TaskType::async):
			asyncTasks.AddTask (std::move (task));
			break;
		case (TaskType::nextFrame):
			break;
	}
	workSubmittedCondVar.notify_one ();
}

void TaskManager::Submit (std::vector<Task> tasks, TaskType type)
{
	switch (type)
	{
		default:
		case (TaskType::currentFrame):
			currentFrameTasks.AddTasks (tasks);
			break;
		case (TaskType::async):
			asyncTasks.AddTasks (tasks);
			break;
		case (TaskType::nextFrame):
			break;
	}
	workSubmittedCondVar.notify_all ();
}

// void TaskManager::AddTask (Task&& task) { currentFrameTasks.AddTask (std::move (task)); }

std::optional<Task> TaskManager::GetTask ()
{
	if (currentFrameTasks.HasTasks ())
		return currentFrameTasks.GetTask ();
	else
		return asyncTasks.GetTask ();
}

Worker::Worker (TaskManager& taskMan, int threadID)
: taskMan (taskMan), workerThread{ &Worker::Work, this }, threadID (threadID)
{
}

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
		{
			std::unique_lock<std::mutex> lock (taskMan.workSubmittedLock);
			taskMan.workSubmittedCondVar.wait (lock);
		}

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
	for (int i = 0; i < workerCount; i++)
	{
		workers.push_back (std::make_unique<Worker> (taskMan, i));
	}
}

WorkerPool::~WorkerPool ()
{
	StopWorkers ();
	taskMan.workSubmittedCondVar.notify_all ();
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

	JobTesterClass jtc;
	jtc.Print ();

	int jobCount = 1000;

	auto signal1 = std::make_shared<TaskSignal> ();
	Task t1 = Task (signal1, [&]() {
		for (int i = 0; i < jobCount; i++)
			jtc.AddNum (1);
	});

	auto signal2 = std::make_shared<TaskSignal> ();

	signal2->WaitOn (signal1);
	Task t2 = Task (signal2, [&]() {
		for (int i = 0; i < jobCount; i++)
			jtc.MulNumAddNum (2, 0);
	});

	tMan.Submit (std::move (t1), TaskType::currentFrame);
	tMan.Submit (std::move (t2), TaskType::currentFrame);


	workerPool.StopWorkers ();
	signal2->Wait ();
	jtc.Print ();
	Log.Debug (fmt::format ("Job system test: done\n"));

	//	Log::Debug << "Job system test: done\n";
	return true;
}
} // namespace job