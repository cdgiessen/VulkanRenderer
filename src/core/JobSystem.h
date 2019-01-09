#pragma once


#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "util/ConcurrentQueue.h"

namespace job
{

class TaskSignal
{
	public:
	TaskSignal ();
	~TaskSignal ();

	void Notify (); // for tasks that should be waited upon

	void Signal (); // for task to call when done

	void Wait (); // for owner to call

	void Cancel (); // cancel jobs not yet started

	void WaitOn (std::shared_ptr<TaskSignal>); // set other signal to wait to start until this one is ready

	bool IsCancelled (); // make sure signal wasn't cancelled

	bool IsReadyToRun (); // if all predicates are satisfied

	int InQueue (); // number of threads who will signal this queue

	private:
	std::atomic_bool cancelled = false;
	std::atomic_int active_waiters = 0;

	std::mutex condVar_lock;
	std::condition_variable condVar;

	std::mutex pred_lock;
	std::vector<std::shared_ptr<TaskSignal>> predicates;
};

class Task
{
	public:
	Task (std::weak_ptr<TaskSignal> signalBlock, std::function<void()>&& job);

	void operator() ();

	void WaitOn ();

	bool IsReadyToRun ();

	private:
	std::function<void()> m_job;
	std::weak_ptr<TaskSignal> signalBlock;
};

class TaskPool
{
	public:
	TaskPool ();

	void AddTask (Task&& task);
	void AddTasks (std::vector<Task> tasks);

	std::optional<Task> GetTask ();

	bool HasTasks();

	private:
	std::mutex queueLock;
	std::queue<Task> tasks;
};

enum class TaskType
{
	currentFrame,
	async,
	nextFrame
};

class Worker;
class WorkerPool;

class TaskManager
{
	public:
	TaskManager ();

	void Submit (Task&& task, TaskType type = TaskType::currentFrame);
	void Submit (std::vector<Task> tasks, TaskType type = TaskType::currentFrame);

	std::optional<Task> GetTask ();

	private:
	//int jobCount = 0;
	TaskPool currentFrameTasks;
	TaskPool asyncTasks;

	friend Worker;
	friend WorkerPool;
	std::mutex workSubmittedLock;
	std::condition_variable workSubmittedCondVar;
};

class Worker
{
	public:
	Worker (TaskManager& taskMan, int threadID);
	~Worker ();

	void Stop ();

	private:
	void Work ();
	TaskManager& taskMan;
	int threadID;
	std::thread workerThread;
	std::atomic_bool isWorking = true;
};

class WorkerPool
{
	public:
	WorkerPool (TaskManager& taskMan, int workerCount = 1);
	~WorkerPool ();

	void StopWorkers ();

	private:
	TaskManager& taskMan;
	std::vector<std::unique_ptr<Worker>> workers;
	int workerCount = 1;
};

extern bool JobTester ();
} // namespace job

extern job::TaskManager taskManager;