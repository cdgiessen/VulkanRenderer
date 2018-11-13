#pragma once


#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

#include "util/ConcurrentQueue.h"

namespace job {

	class TaskSignal {
	public:
		TaskSignal();
		~TaskSignal();

		void Notify(); //for tasks that should be waited upon

		void Signal(); //for task to call when done

		void Wait(); //for owner to call

		bool IsReadyToRun();
		void WaitOn(std::shared_ptr<TaskSignal>);

	private:
		std::atomic_int active_waiters = 0;

		std::mutex condVar_lock;
		std::condition_variable condVar;

		std::mutex pred_lock;
		std::vector<std::shared_ptr<TaskSignal>> predicates;
	};

	class Task {
	public:
		Task(std::weak_ptr<TaskSignal> signalBlock, std::function<void()>&& job);

		void operator()();

		void WaitOn();

		bool IsReadyToRun();

	private:
		std::function<void()> m_job;
		std::weak_ptr<TaskSignal> signalBlock;
	};

	class TaskPool {
	public:
		TaskPool();

		void AddTask(Task&& task);
		void AddTasks(std::vector<Task> tasks);

		std::optional<Task> GetTask();

	private:
		std::mutex queueLock;
		std::queue<Task> tasks;
	};

	enum class TaskType {
		currentFrame,
		async,
		nextFrame
	};

	class TaskManager {
	public:
		TaskManager();

		void Submit(Task&& task, TaskType type = TaskType::currentFrame);
		void Submit(std::vector<Task> tasks, TaskType type = TaskType::currentFrame);

		std::optional<Task> GetTask();

		void EndSubmission();

	private:
		TaskPool currentFrameTasks;
		TaskPool asyncTasks;
	};

	class Worker {
	public:
		Worker(TaskManager& taskMan);
		~Worker();

		void Stop();

	private:
		void Work();
		TaskManager& taskMan;

		std::thread workerThread;
		std::atomic_bool isWorking = true;
	};

	class WorkerPool {
	public:
		WorkerPool(TaskManager& taskMan, int workerCount = 1);
		~WorkerPool();

		void StopWorkers();

	private:
		TaskManager & taskMan;
		std::vector<std::unique_ptr<Worker>> workers;
		int workerCount = 1;
	};

	extern bool JobTester();
}

extern job::TaskManager taskManager;