#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// 线程池类
class ThreadPool {
public:
    // 构造函数：创建指定数量的工作线程
    // 返回当前系统的硬件线程并发数，也就是
    // CPU的核心数量或线程数（包含超线程）
    explicit ThreadPool(
        size_t numThreads = std::thread::hardware_concurrency());

    // 析构函数：停止所有线程
    ~ThreadPool();

    // 提交任务到线程池
    void enqueueTask(const std::function<void()>& task);

    // 获取线程池大小
    size_t size() const {
        return workers_.size();
    }

    // 获取队列中等待的任务数量
    size_t queueSize() const;

    // 停止线程池（等待所有任务完成）
    void shutdown();

    // 强制停止线程池（不等待任务完成）
    void forceShutdown();

private:
    // 工作线程
    std::vector<std::thread> workers_;

    // 任务队列
    std::queue<std::function<void()>> tasks_;

    // 同步原语
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;

    // 停止标志
    bool stop_;

    // 工作线程的主循环
    void workerLoop();
};