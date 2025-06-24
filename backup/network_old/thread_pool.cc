#include "thread_pool.h"
#include <iostream>

using std::cout;
using std::endl;
using std::unique_lock;
using std::mutex;
using std::thread;
using std::function;

ThreadPool::ThreadPool(size_t numThreads)
    : stop_(false) {
    cout << "创建线程池，工作线程数量: " << numThreads << endl;

    // 创建工作线程
    workers_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::enqueueTask(const function<void()>& task) {
    {
        unique_lock<mutex> lock(queueMutex_);
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks_.push(task);
    }
    condition_.notify_one();
}

size_t ThreadPool::queueSize() const {
    unique_lock<mutex> lock(queueMutex_);
    return tasks_.size();
}

void ThreadPool::shutdown() {
    if (stop_) {
        return; // 已经停止
    }

    cout << "正在关闭线程池..." << endl;

    {
        unique_lock<mutex> lock(queueMutex_);
        stop_ = true;
    }

    // 唤醒所有等待的线程
    condition_.notify_all();

    // 等待所有线程完成
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    cout << "线程池已关闭" << endl;
}

void ThreadPool::forceShutdown() {
    cout << "强制关闭线程池..." << endl;

    {
        unique_lock<mutex> lock(queueMutex_);
        stop_ = true;
        // 清空任务队列
        while (!tasks_.empty()) {
            tasks_.pop();
        }
    }

    condition_.notify_all();

    for (thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    cout << "线程池已强制关闭" << endl;
}

void ThreadPool::workerLoop() {
    while (true) {
        function<void()> task;

        {
            unique_lock<mutex> lock(queueMutex_);

            // 等待任务或停止信号
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            // 如果停止且队列为空，退出
            if (stop_ && tasks_.empty()) {
                return;
            }

            // 取出一个任务
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        // 执行任务
        try {
            task();
        } catch (const std::exception& e) {
            cout << "线程池任务执行出错: " << e.what() << endl;
        } catch (...) {
            cout << "线程池任务执行出现未知错误" << endl;
        }
    }
}