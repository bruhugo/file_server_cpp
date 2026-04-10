#include "ThreadPool.hpp"
#include "Logger.hpp"
#include <mutex>
#include <iostream>

using namespace std;

ThreadPool::ThreadPool(size_t threads = 10){
    workers = Workers(threads);
    LOG_DEBUG << "Starting thread pool";
    for (int i = 0; i < threads; i++)
        workers.push_back(thread([this, i]{workerLoop(i);}));
}


ThreadPool::~ThreadPool(){
    LOG_DEBUG << "Closing thread pool";

    {
        lock_guard<mutex> lk(mu);
        stop = true;
        cv.notify_all();
    }

    for (auto& worker : workers){
        if (worker.joinable())
            worker.join();
    }

}

void ThreadPool::submit(Task task){
    {
        lock_guard<mutex> lk(mu);
        tasks.push(task);
        cv.notify_one();
    }
}

void ThreadPool::workerLoop(int threadId){
    LOG_DEBUG << "Worker " << threadId << " starting.";
    for (;;){
        {
            unique_lock<mutex> lk(mu);
            cv.wait(lk, [this]{return !tasks.empty() || stop;});
        }
        if (stop) return;
        LOG_DEBUG << "Worker " << threadId << " received a task.";
        Task task;
        {
            lock_guard<mutex> lk(mu);
            task = tasks.front(); tasks.pop();
        }
        task();
    }
}
