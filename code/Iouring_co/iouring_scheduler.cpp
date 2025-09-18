#include "iouring_scheduler.hpp"
#include <stdexcept>
#include <unistd.h>

IoUringScheduler::IoUringScheduler(size_t entries) {
    if (io_uring_queue_init(entries, &m_ring, 0) != 0) {
        throw std::runtime_error("io_uring init failed");
    }
}

IoUringScheduler::~IoUringScheduler() {
    io_uring_queue_exit(&m_ring);
}

void IoUringScheduler::ReadAwaiter::submit_io() {
    auto* scheduler = static_cast<IoUringScheduler*>(this->m_scheduler);
    auto* sqe = io_uring_get_sqe(&scheduler->m_ring);
    io_uring_prep_read(sqe, m_fd, m_buf, m_len, m_offset);
    io_uring_sqe_set_data(sqe, this);
    io_uring_submit(&scheduler->m_ring);
}

void IoUringScheduler::run() {
    m_running = true;
    while (m_running) {
        // 处理完成事件
        io_uring_cqe* cqe;
        int ret = io_uring_peek_cqe(&m_ring, &cqe);
        if (ret == 0 && cqe) {
            auto* awaiter = static_cast<IoUringAwaiter*>(io_uring_cqe_get_data(cqe));
            awaiter->m_result = cqe->res;
            m_ready_coros.push(awaiter->m_coro);
            io_uring_cqe_seen(&m_ring, cqe);
        }

        // 恢复就绪协程
        while (!m_ready_coros.empty()) {
            auto h = m_ready_coros.front();
            m_ready_coros.pop();
            h.resume();
        }

        // 无任务时休眠（避免忙等）
        if (m_ready_coros.empty() && ret != 0) {
            __io_uring_sqring_wait(&m_ring);
        }
    }
}

void IoUringScheduler::stop() {
    m_running = false;
}