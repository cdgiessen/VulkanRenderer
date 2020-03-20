#pragma once


#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

unsigned int HardwareThreadCount ();

namespace job
{

using WorkFuncSig = std::function<void ()>;


class TaskSignal
{
	public:
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
	Task (WorkFuncSig&& job, std::weak_ptr<TaskSignal> signalBlock);

	void Run ();

	void WaitOn ();

	bool IsReadyToRun ();

	private:
	WorkFuncSig m_job;
	std::weak_ptr<TaskSignal> signalBlock;
};

class ThreadPool
{
	public:
	ThreadPool ();
	~ThreadPool ();

	void Stop ();

	void Submit (WorkFuncSig&& job, std::weak_ptr<TaskSignal> signalBlock);
	void Submit (std::vector<Task> tasks);

	std::optional<Task> GetTask ();

	std::vector<std::thread::id> GetThreadIDs ();

	private:
	std::mutex queue_lock;
	std::queue<Task> task_queue;

	std::atomic_bool continue_working = true;
	std::vector<std::thread> worker_threads;

	std::mutex workSubmittedLock;
	std::condition_variable workSubmittedCondVar;
};
} // namespace job

namespace
{
bool JobTester ();
}