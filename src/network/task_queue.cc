#include "task_queue.h"

TaskQueue::TaskQueue(size_t capa)
    : capacity_(capa)
    , que_()
    , mutex_()
    , notEmpty_()
    , notFull_()
    , flag_(true) {}

TaskQueue::~TaskQueue() {}

// 添加任务与获取任务
void TaskQueue::push(ElemType&& task) {
    unique_lock<mutex> ul(mutex_);
    while (full()) {
        notFull_.wait(ul);
    }
    que_.push(std::move(task));
    notEmpty_.notify_one();
}

ElemType TaskQueue::pop() {
    unique_lock<mutex> ul(mutex_);
    while (empty() && flag_) {
        notEmpty_.wait(ul);
    }

    if (flag_) {
        ElemType tmp = que_.front();
        que_.pop();

        notFull_.notify_one();

        return tmp;
    } else {
        return nullptr;
    }
}

// 判空与判满
bool TaskQueue::empty() const {
    /* return _que.empty(); */
    return que_.size() == 0;
}

bool TaskQueue::full() const {
    return que_.size() == capacity_;
}

// 将所有等待在非空条件变量上的线程唤醒
void TaskQueue::wakeup() {
    flag_ = false;
    notEmpty_.notify_all();
}
