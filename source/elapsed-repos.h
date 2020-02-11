#ifndef __ELAPSED_REPOS_H__
#define __ELAPSED_REPOS_H__

#include <set>
#include <map>
#include <vector>
#include <deque>
#include <memory>

#include <thread>
#include <mutex>

#include "libpf-internal.h"

namespace libpf {

// 根据metric_runtime处理计算后的汇总结果
struct metric_info {

    time_t start_tm_; // unixstamp， 开始时刻
    time_t duration_; // 采样时长

    int32_t avg_;
    int32_t min_;
    int32_t max_;
    int32_t p10_;
    int32_t p50_;
    int32_t p99_;
    int32_t p999_;

    int32_t cnt_;
    int32_t tps_;
    int64_t sum_;

    metric_info(time_t start_tm, time_t duration):
        start_tm_(start_tm),
        duration_(duration) {}
};

struct metric_runtime {

    time_t start_tm_;
    time_t duration_;

    std::vector<int32_t> detail_;

    metric_runtime(time_t start_tm, time_t duration):
        start_tm_(start_tm),
        duration_(duration) {}
};

using metric_infos = std::map<std::string, metric_info>;
using metric_runtimes = std::map<std::string, metric_runtime>;


class ElapsedRepos {

    __noncopyable__(ElapsedRepos)

public:
    static ElapsedRepos& instance();

    bool init(time_t duration, uint32_t sample);
    void terminate();

    void submit(const std::string& metric, int32_t val);
    bool message(std::string& msg);

private:
    bool try_switch() {
        std::lock_guard<std::mutex> lock(mutex_);
        return try_switch(lock);
    }

    bool try_switch(std::lock_guard<std::mutex>& lock);

private:
    ElapsedRepos() = default; // 默认构造

    bool initialized_ = false;
    bool terminate_ = false;

    time_t start_clock_ms_; // 本轮采样开始的时间，使用ms精度记录
    time_t duration_ms_; // 需要保留采样的时长

    std::mutex mutex_;
    metric_runtimes* run_ptr_; // 实时数据存储

    // 辅助线程，主要驱动时间，已经对已经完成采样的数据进行计算处理
    std::thread helper_;
    void run();

    // 处理队列
    std::mutex mutex_queue_;
    std::vector<metric_runtimes *> process_queue_;

    // 队列的异步通知fd，由select监听事件
    int notify_send_fd_;
    int notify_recv_fd_;

    uint32_t sample_ = 1; // 保留的样本数
    uint32_t dropped_sample_ = 0; // 环形队列，已经丢弃的历史样本数
    std::mutex mutex_persist_;
    std::deque<metric_infos *> persistence_; // 统计结果的持久化
};



} // end namespace libpf

#endif // __ELAPSED_REPOS_H__