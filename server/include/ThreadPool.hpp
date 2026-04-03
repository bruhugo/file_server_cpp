#include <functional>
#include <thread>
#include <stdexcept>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <string>



using namespace std;

using Task = function<void()>;
using Tasks = queue<Task>;
using Workers = vector<thread>;

class ThreadError : public exception{
public:
    ThreadError(char *msg);
    ThreadError(char *msg, int threadNum);
    char *what();

    int threadNum;
private:
    char *message;
};

class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool other) = delete;

    void submit(Task);

private:
    void workerLoop(int threadNum); 

    mutex mu;
    condition_variable cv;
    
    Tasks tasks;
    Workers workers;
    bool stop = false;
};
