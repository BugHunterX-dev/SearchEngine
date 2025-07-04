#include "thread_pool.h"

#include <iostream>

using std::cout;
using std::endl;

ThreadPool::ThreadPool(size_t threadNum, size_t queSize)
    : threadNum_(threadNum)
    , threads_()
    , queSize_(queSize)
    , taskQueue_(queSize_)
    , isExit_(false) {}

ThreadPool::~ThreadPool() {}

// 线程池的启动与停止
void ThreadPool::start() {
    // 将子线程创建出来，并且存放在vector
    for (size_t idx = 0; idx < threadNum_; ++idx) {
        threads_.push_back(thread(&ThreadPool::doTask, this));
    }
}

void ThreadPool::stop() {
    // 只要任务队列中还有任务，就不应该让主线程急着向下执行去
    // 回收子线程，在此处可以添加对应代码控制主线程向下走
    while (!taskQueue_.empty()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 将标志位设置为true，表明线程池要退出
    isExit_ = true;

    // 可以在此处将所有等待在_notEmpty条件变量上的线程唤醒
    taskQueue_.wakeup();

    // 让主线程等待子线程的退出
    for (auto& th : threads_) {
        th.join();
    }
}

// 任务的添加与获取
void ThreadPool::addTask(Task&& task) {
    if (task) {
        taskQueue_.push(std::move(task));
    }
}

Task ThreadPool::getTask() {
    return taskQueue_.pop();
}

// 线程池交给工作线程执行的任务（线程入口函数）
void ThreadPool::doTask() {
    // 只要线程池不退出，就应该让工作线程一直执行任务
    // 如果任务队列为空，应该让工作线程睡眠
    while (!isExit_) {
        // 获取任务，然后再执行任务
        Task taskcb = getTask();
        if (taskcb) {
            /* ptask->process();//体现多态 */
            taskcb(); // 回调函数的执行
        } else {
            cout << "nullptr == taskcb" << endl;
        }
    }
}
