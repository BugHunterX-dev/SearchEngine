#include "event_loop.h"
#include "acceptor.h"
#include "tcp_connection.h"
#include <iostream>
#include <sys/eventfd.h>
#include <unistd.h>

using std::cerr;
using std::cout;
using std::endl;

EventLoop::EventLoop(Acceptor& acceptor)
    : epfd_(createEpollFd())
    , evtList_(1024) // 采用个count个value进行初始化
    , isLooping_(false)
    , acceptor_(acceptor)
    , evtfd_(createEventFd()) // 创建用于通知的文件描述符
    , pengdings_()
    , mutex_() {
    // 需要获取listenfd，然后放在红黑树上进行监听
    int listenfd = acceptor_.fd();
    addEpollReadFd(listenfd);

    // 监听用于通信的文件描述符
    addEpollReadFd(evtfd_);
}

EventLoop::~EventLoop() {
    close(epfd_);
    close(evtfd_);
}

// 循环是否在进行
void EventLoop::loop() {
    isLooping_ = true;
    while (isLooping_) {
        waitEpollFd();
    }
}

void EventLoop::unloop() {
    isLooping_ = false;
}

// 封装了epoll_wait函数
void EventLoop::waitEpollFd() {
    int nready = 0;
    do {
        // vector中如何获取第一个元素的首地址
        nready = epoll_wait(epfd_, &*evtList_.begin(), evtList_.size(), 3000);
    } while (-1 == nready && errno == EINTR);

    if (-1 == nready) {
        cerr << "-1 == nready" << endl;
        return;
    } else if (0 == nready) {
        cout << ">>epoll_wait timeout!!!" << endl;
    } else {
        // 需要考虑vector，也就是_evtList的扩容问题(1024)
        if ((int)evtList_.size() == nready) {
            evtList_.reserve(2 * nready);
        }

        for (int idx = 0; idx < nready; ++idx) {
            int listenfd = acceptor_.fd();
            int fd = evtList_[idx].data.fd;

            if (fd == listenfd) // 有新的连接请求
            {
                handleNewConnection();
            } else if (fd == evtfd_) // 通信的文件描述符就绪
            {
                handleRead();
                // 可以执行所有的"任务",就是遍历vector
                doPengdingFunctors();
                /* for(auto &cb : _pengdings) */
                /* { */
                /*     cb(); */
                /* } */
            } else {
                handleMessage(fd); // 处理老的连接
            }
        }
    }
}

// 处理新的连接请求
void EventLoop::handleNewConnection() {
    // 只要accept函数有正确返回结果，就表明三次握手建立成功
    int connfd = acceptor_.accept();
    if (connfd < 0) {
        perror("handleNewConnection");
        return;
    }

    // 将connfd放在红黑树上进行监听
    addEpollReadFd(connfd);

    // 可以使用文件描述符connfd创建TcpConnection连接的对象
    TcpConnectionPtr con(new TcpConnection(connfd, this));

    // 将三个事件转接个TcpConnection的对象
    con->setNewConnectionCallback(onNewConnection_); // 注册连接建立的事件
    con->setMessageCallback(onMessage_); // 注册消息到达的事件
    con->setCloseCallback(onClose_);     // 注册连接断开的事件

    /* _conns.insert({connfd, con}); */
    conns_[connfd] = con; // 存放键值对

    // 连接建立的事件已经满足
    con->handleNewConnectionCallback(); // 执行连接建立的事件
}

// 老的连接上数据发过来
void EventLoop::handleMessage(int fd) {
    auto it = conns_.find(fd);
    if (it != conns_.end()) {
        // isClose函数的作用就是看有没有断开
        bool flag = it->second->isClosed();
        if (flag) {
            // 连接已经断开了
            it->second->handleCloseCallback(); // 连接断开的事件
            delEpollReadFd(fd); // 取消监听（从红黑树上删除）
            conns_.erase(it);   // 删除文件描述符与连接的键值对
        } else {
            it->second->handleMessageCallback(); // 消息达到的事件
        }
    } else {
        cout << "连接是不存在的 " << endl;
        return;
    }
}

// 创建文件epoll的文件描述符
int EventLoop::createEpollFd() {
    int fd = epoll_create(10);
    if (fd < 0) {
        perror("createEpollFd");
        return -1;
    }

    return fd;
}

// 将文件描述符放在红黑树上进行监听
void EventLoop::addEpollReadFd(int fd) {
    struct epoll_event evt;
    evt.data.fd = fd; // 要监听的文件描述符
    evt.events = EPOLLIN;

    int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &evt);
    if (ret < 0) {
        perror("addEpollReadFd");
        return;
    }
}

// 将文件描述符从红黑树上取消监听
void EventLoop::delEpollReadFd(int fd) {
    struct epoll_event evt;
    evt.data.fd = fd; // 要监听的文件描述符
    evt.events = EPOLLIN;

    int ret = epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &evt);
    if (ret < 0) {
        perror("delEpollReadFd");
        return;
    }
}
// 注册三个事件（起到桥梁作用，目的是为了传递给TcpConnection）
void EventLoop::setNewConnectionCallback(TcpConnectionCallback&& cb) {
    onNewConnection_ = std::move(cb);
}

void EventLoop::setMessageCallback(TcpConnectionCallback&& cb) {
    onMessage_ = std::move(cb);
}

void EventLoop::setCloseCallback(TcpConnectionCallback&& cb) {
    onClose_ = std::move(cb);
}
// 创建用于通知的文件描述符
int EventLoop::createEventFd() {
    int fd = eventfd(0, 0);
    if (fd < 0) {
        perror("createEventFd");
        return -1;
    }

    return fd;
}

// 封装read
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t ret = read(evtfd_, &one, sizeof(uint64_t));
    if (ret != sizeof(uint64_t)) {
        perror("handleRead");
    }
}
// 封装write
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t ret = write(evtfd_, &one, sizeof(uint64_t));
    if (ret != sizeof(uint64_t)) {
        perror("wakeup");
    }
}

void EventLoop::doPengdingFunctors() {
    vector<Functor> tmp;
    {
        lock_guard<mutex> lg(mutex_);
        tmp.swap(pengdings_);
    }

    // 执行所有的任务:此处的“任务”就是线程池处理好之后的数据msg
    // 以及发送数据的函数send，以及执行send函数的TcpConnection的
    // 对象this,
    for (auto& cb : tmp) {
        cb();
    }
}

void EventLoop::runInLoop(Functor&& cb) {
    {
        lock_guard<mutex> lg(mutex_);
        pengdings_.push_back(std::move(cb));
    }

    wakeup();
}
