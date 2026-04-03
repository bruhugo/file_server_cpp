#include "ThreadPool.hpp"
#include <mutex>
#include <iostream>

using namespace std;

ThreadPool::ThreadPool(size_t threads = 10){
    workers = Workers(threads);
    cout << "Starting thread pool..." << endl;
    for (int i = 0; i < threads; i++)
        workers.push_back(thread([this, i]{workerLoop(i);}));
}


ThreadPool::~ThreadPool(){
    cout << "Closing thread pool" << endl;

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
    cout << "Worker " << threadId << " started" << endl;
    
    for (;;){
        {
            unique_lock<mutex> lk(mu);
            cv.wait(lk, [this]{return !tasks.empty() || stop;});
        }
        if (stop) return;
        Task task;
        {
            lock_guard<mutex> lk(mu);
            task = tasks.front(); tasks.pop();
        }
        task();
    }
}
