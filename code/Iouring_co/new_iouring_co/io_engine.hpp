#pragma once
#include <coroutine>
#include <memory>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

// 尝试检测io_uring支持
#if __has_include(<liburing.h>)
#include <liburing.h>
#define HAVE_LIBURING 1
#endif

class IoEngine {
public:
    virtual ~IoEngine() = default;
    virtual bool submit_read(int fd, void* buf, size_t len, std::coroutine_handle<> h) = 0;
    virtual void process_completions() = 0;
    virtual void stop() = 0;
};

// -------------------- io_uring引擎 --------------------
#ifdef HAVE_LIBURING
class IoUringEngine : public IoEngine {
public:
    IoUringEngine() {
        if (io_uring_queue_init(1024, &ring_, 0) < 0) {
            throw std::runtime_error("io_uring init failed");
        }
    }

    ~IoUringEngine() {
        io_uring_queue_exit(&ring_);
    }

    bool submit_read(int fd, void* buf, size_t len, std::coroutine_handle<> h) override {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
        if (!sqe) return false;

        io_uring_prep_read(sqe, fd, buf, len, 0);
        io_uring_sqe_set_data(sqe, h.address());
        return io_uring_submit(&ring_) == 1;
    }

    void process_completions() override {
        io_uring_cqe* cqe;
        unsigned head, count = 0;

        io_uring_for_each_cqe(&ring_, head, cqe) {
            ++count;
            auto h = std::coroutine_handle<>::from_address(io_uring_cqe_get_data(cqe));
            h.resume();
        }

        if (count > 0) io_uring_cq_advance(&ring_, count);
    }

    void stop() override {}

private:
    io_uring ring_;
};
#endif

// -------------------- epoll引擎 --------------------
class EpollEngine : public IoEngine {
public:
    EpollEngine() {
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ < 0) throw std::runtime_error("epoll create failed");

        event_fd_ = eventfd(0, EFD_NONBLOCK);
        if (event_fd_ < 0) {
            close(epoll_fd_);
            throw std::runtime_error("eventfd create failed");
        }

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = event_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_fd_, &ev) < 0) {
            close(epoll_fd_);
            close(event_fd_);
            throw std::runtime_error("epoll_ctl failed");
        }
    }

    ~EpollEngine() {
        close(epoll_fd_);
        close(event_fd_);
    }

    bool submit_read(int fd, void* buf, size_t len, std::coroutine_handle<> h) override {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = h.address();

        std::lock_guard lock(mutex_);
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            return false;
        }

        pending_io_[fd] = h;
        return true;
    }

    void process_completions() override {
        constexpr int max_events = 64;
        epoll_event events[max_events];

        int n = epoll_wait(epoll_fd_, events, max_events, 0);
        for (int i = 0; i < n; ++i) {
            auto h = std::coroutine_handle<>::from_address(events[i].data.ptr);
            h.resume();

            std::lock_guard lock(mutex_);
            pending_io_.erase(events[i].data.fd);
        }
    }

    void stop() override {
        uint64_t val = 1;
        write(event_fd_, &val, sizeof(val));
    }

private:
    int epoll_fd_;
    int event_fd_;
    std::mutex mutex_;
    std::unordered_map<int, std::coroutine_handle<>> pending_io_;
};
