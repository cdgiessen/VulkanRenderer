#include "JobSystem.h"

#include "Logger.h"

namespace job {

    TaskSignal::TaskSignal(std::shared_ptr<TaskBarrier> barrier):
        barrier(barrier)
    {

    }

    TaskSignal::~TaskSignal(){
            
    }
        
    void TaskSignal::Signal(){
        finished = true;
        condVar.notify_all();
    }

    void TaskSignal::Wait(){
        std::unique_lock<std::mutex> mlock(condVar_lock);
        condVar.wait(mlock);
    }

    Task::Task(TaskType type,
        std::weak_ptr<TaskSignal> signalBlock):
        type(type), signalBlock(signalBlock){

    }

    void Task::AddJob(Job&& newJob){
        jobs.push_back(std::move(newJob));
    }

    void Task::operator()(){
        if( auto sbp = signalBlock.lock()){      
            for(auto& job : jobs)
                job();
            sbp->Signal();
        }
    }

    void Task::WaitOn() {
        if( auto sbp = signalBlock.lock())     
            sbp->Wait();
    }

    TaskPool::TaskPool(){

    }

    void TaskPool::AddTask(Task&& task){
        std::lock_guard<std::mutex>lg(queueLock);
        tasks.push(std::move(task));
    }

    std::optional<Task> TaskPool::GetTask(){
        std::lock_guard<std::mutex>lg(queueLock);
        if(!tasks.empty()){
            auto val = tasks.front();
            tasks.pop();
            return val;
        }
        return {};
    }

    TaskManager::TaskManager(){

    }

    void TaskManager::AddTask(Task&& task){
        currentFrameTasks.AddTask(std::move(task));
    }
    
    std::optional<Task> TaskManager::GetTask(){
        return currentFrameTasks.GetTask();
    }

    Worker::Worker( TaskManager& taskMan): 
        taskMan(taskMan),
        workerThread{&Worker::Work, this}
    {
    }

    Worker::~Worker(){
        Stop();
        workerThread.join();
    }
    void Worker::Stop(){
        isWorking = false;
    }

    void Worker::Work() {
    	while (isWorking)
    	{
            // taskMan.WaitOn();

            auto task = taskMan.GetTask();
    		while (task.has_value()) {
    			(*task)();
                task = taskMan.GetTask();
    		}
    	}
    }

    WorkerPool::WorkerPool(TaskManager& taskMan, int workerCount):
        taskMan(taskMan), workerCount(workerCount)
    {

    }

    void WorkerPool::StartWorkers(){
        if(workerCount > 0){
            for(int i = 0 ; i < workerCount; i++){
                workers.push_back(std::make_unique<Worker>(taskMan));
            }
        }
    }
    void WorkerPool::StopWorkers(){
        for(auto& worker : workers){
            worker->Stop();
        }
    }

    class JobTesterClass {
    public:
        void AddOne();
        void MulTwo();

        void Print() { 
            for(auto& n : nums){
                Log::Debug << n << " ";
            }
            Log::Debug << "\n";
        }
    private:
        std::vector<int> nums = {1,2,3,4,5};
    };

    void JobTesterClass::AddOne(){
        for(auto& n : nums)
            n += 1;
    }

    void JobTesterClass::MulTwo(){
        for(auto& n : nums)
            n *= 2;
    }

    bool JobTester(){
        Log::Debug << "Job system test\n";
        TaskManager tMan;

        WorkerPool workerPool(tMan, 2);
        workerPool.StartWorkers();

        JobTesterClass jtc;
        jtc.Print();
        Job j1{[&](){jtc.AddOne(); }};
        Job j2{[&](){jtc.MulTwo(); }};

        auto signal = std::make_shared<TaskSignal>();
        Task t = Task(TaskType::currentFrame, signal);
        t.AddJob(std::move(j1));
        t.AddJob(std::move(j2));
        tMan.AddTask(std::move(t));
        workerPool.StopWorkers();
        signal->Wait();
        jtc.Print();
        
    }
}