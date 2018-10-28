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


	class Job {
	public:
		Job(std::function<void()> work) : work(work) {}

		void operator()() {
			if (work)
				work();
		}
	private:
		std::function<void()> work;
	};

	enum class TaskType {
		currentFrame,
		async,
		nextFrame
	};

	class TaskSignal;

	class Task {
	public:
		Task(TaskType type, std::weak_ptr<TaskSignal> signalBlock);

		void Add(Job&& newJob);

		void operator()();

		void WaitOn();

		bool IsReadyToRun();

	private:
		TaskType type;
		std::vector<Job> jobs;
		std::weak_ptr<TaskSignal> signalBlock;
	};

	class TaskSignal {
	public:
		TaskSignal();
		~TaskSignal();

		void Signal(); //for task to call

		void Wait(); //for owner to call

		bool IsReadyToRun();
		void AddTaskToWaitOn(std::shared_ptr<TaskSignal>);

	private:
		std::atomic_bool finished = false;

		std::mutex condVar_lock;
		std::condition_variable condVar;

		std::mutex pred_lock;
		std::vector<std::shared_ptr<TaskSignal>> predicates;
	};

	class TaskPool {
	public:
		TaskPool();

		void AddTask(Task&& task);

		std::optional<Task> GetTask();

	private:
		std::mutex queueLock;
		std::queue<Task> tasks;
	};

	class TaskManager {
	public:
		TaskManager();

		void AddTask(Task&& task);

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

		void StartWorkers();
		void StopWorkers();

	private:
		TaskManager & taskMan;
		std::vector<std::unique_ptr<Worker>> workers;
		int workerCount = 1;
	};

	extern bool JobTester();
}