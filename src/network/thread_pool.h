#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "task_queue.h"
#include <thread>
#include <vector>
#include <functional>

using std::thread;
using std::vector;
using std::function;


//任务的类型，其实就是function对象
using Task = function<void()>;

class ThreadPool
{
public:
    ThreadPool(size_t threadNum, size_t queSize);
    ~ThreadPool();

    //线程池的启动与停止
    void start();
    void stop();

    //任务的添加与获取
    void addTask(Task &&task);
private:
    Task getTask();

    //线程池交给工作线程执行的任务（线程入口函数）
    void doTask();

private:
    size_t threadNum_;//子线程的数目
    vector<thread> threads_;//存放线程的容器
    size_t queSize_;//任务队列的大小
    TaskQueue taskQueue_;//任务队列
    bool isExit_;//标识线程池是否退出的标志位

};

#endif
