
#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <numeric>
#include <algorithm>

#include <iostream>
#include <sstream>

#include "time_util.h"
#include "elapsed-repos.h"

namespace libpf {

ElapsedRepos& ElapsedRepos::instance() {
    static ElapsedRepos handler {};
    return handler;
}

bool ElapsedRepos::init(time_t duration, uint32_t sample) {

    if(initialized_) {
        std::cout << "[INFO] ElapsedRepos already initialized successfully." << std::endl;
        return true;
    }

    duration_ms_ = duration * 1000;
    sample_ = sample;
    if(duration_ms_ == 0)
        duration_ms_ = 60 * 1000;
    if(sample_ == 0)
        sample_ = 1;

    int fds[2] {};
    if(::pipe2(fds, O_NONBLOCK)) {
        std::cout << "[ERROR] create notify_fds failed." << std::endl;
        return false;
    }

    notify_send_fd_ = fds[1]; // 写端
    notify_recv_fd_ = fds[0]; // 读端

    // 启动辅助线程
    helper_ = std::thread(std::bind(&ElapsedRepos::run, this));

    initialized_ = true;
    return true;
}

bool ElapsedRepos::try_switch(std::lock_guard<std::mutex> &lock) {

    if(TimeUtil::now_diff_ms(start_clock_ms_) < duration_ms_)
        return false;

    {
        std::lock_guard<std::mutex> lock_queue(mutex_queue_);

        metric_runtimes* prev = run_ptr_;
        process_queue_.push_back(run_ptr_);
        run_ptr_ = nullptr;

        do {
            run_ptr_ = new metric_runtimes();
            start_clock_ms_ = TimeUtil::now_ms();
            if(!run_ptr_) {
                std::cout << "[ERROR] create metric_runtimes failed." << std::endl;
                break;
            }

            if(!prev)
                break;

            // reserve space for performance
            for(auto iter=prev->begin(); iter!=prev->end(); ++iter) {
                if(!iter->second.detail_.empty()) {
                    metric_runtime run(start_clock_ms_ / 1000, duration_ms_);
                    run.detail_.reserve(iter->second.detail_.size() * 1.2);
                    run_ptr_->insert(std::make_pair(iter->first, std::move(run)));
                }
            }
        } while(0);

    }

    // notify queue event
    ::write(notify_send_fd_, "d", 1);
    return true;
}

void ElapsedRepos::terminate() {

    std::cout << "[INFO] begin to terminate libpf." << std::endl;

    terminate_ = true;
    if(helper_.joinable()) {
        helper_.join();
        std::cout << "[INFO] join helper thread done." << std::endl;
    }

    std::cout << "[INFO] terminate libpf done." << std::endl;
}

void ElapsedRepos::submit(const std::string &metric, int32_t val) {

    if(metric.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);

    if(!run_ptr_) {
        start_clock_ms_ = TimeUtil::now_ms();
        run_ptr_ = new metric_runtimes();
        if(!run_ptr_) {
            std::cout << "[ERROR] create metric_runtimes failed." << std::endl;
            return;
        }
    }

    auto iter = run_ptr_->find(metric);
    if(iter == run_ptr_->end()) {
        metric_runtime run(start_clock_ms_ / 1000, duration_ms_);
        run.detail_.reserve(256);
        auto result = run_ptr_->insert(std::make_pair(metric, std::move(run)));
        iter = result.first;
    }

    if(iter != run_ptr_->end()) {
        iter->second.detail_.push_back(val);
    }

    try_switch(lock);
}

bool ElapsedRepos::message(std::string &msg) {

    std::lock_guard<std::mutex> lock_persist(mutex_persist_);

    if(persistence_.empty()) {
        msg = "\nEMPTY.\n";
        return true;
    }

    std::stringstream ss{};
    ss << "===== BEGIN RUNTIME STATISTIC:     =====" << std::endl;

    ss << std::endl;
    ss << "\tduration_ms:" << duration_ms_ << ", samples:" << sample_ << ", droped:" << dropped_sample_ << std::endl;
    ss << std::endl;

    for(size_t i=0; i<persistence_.size(); ++i) {
        const auto infos = persistence_[i];
        for(auto iter=infos->begin(); iter!=infos->end(); ++iter) {
            const auto& info = iter->second;
            ss << "\tmetric:" << iter->first << ", start from:" << TimeUtil::unixtime_str(info.start_tm_) << std::endl;
            ss << "\tmin:" << info.min_ << ", max:" << info.max_ << ", avg:" << info.avg_ << std::endl;
            ss << "\tcnt:" << info.cnt_ << ", sum:" << info.sum_ << ", tps:" << info.tps_ << std::endl;
            ss << "\tp10:" << info.p10_ << ", p50:" << info.p50_ << ", p99:" << info.p99_ << std::endl;
            ss << "\tp999:" << info.p999_ << std::endl;

            ss << std::endl;
        }

        ss << std::endl;
        ss << "----------------------------------------" << std::endl;
        ss << std::endl;
    }

    ss << "===== END RUNTIME STATISTIC:       =====" << std::endl;
    msg = ss.str();
    return true;
}

void ElapsedRepos::run() {

    struct timeval tv {};
    fd_set rfds {};

    while(!terminate_) {

        try_switch();

        // 每次都要重新设置超时时间
        tv.tv_sec = 0;
        tv.tv_usec = 10 * 1000; // 10ms

        FD_ZERO(&rfds);
        FD_SET(notify_recv_fd_, &rfds);
        int max_fd = notify_recv_fd_ + 1;

        // select监听
        int code = ::select(max_fd, &rfds, NULL, NULL, &tv);
        if(code == -1) {
            std::cout << "[ERROR] select call failed." << std::endl;
            break; // fatal ???
        }

        std::vector<metric_runtimes*> tasks {};
        if(FD_ISSET(notify_recv_fd_, &rfds)) {

            // 读完所有的通知
            // TODO: 读取的内容其实可以是命令编码的
            char buff[256] {};
            while(::read(notify_recv_fd_, buff, 256) > 0)
                /* next */;

            std::lock_guard<std::mutex> lock_queue(mutex_queue_);
            tasks.swap(process_queue_);
        }

        // process task
        for(size_t i=0; i<tasks.size(); ++i) {

            metric_runtimes* task_ptr = tasks[i];
            if(!task_ptr) continue;

            if(task_ptr->empty()) {
                delete task_ptr;
                continue;
            }

            // 保存计算汇总后的结果
            metric_infos* infos = new metric_infos();
            if(!infos) {
                std::cout << "[ERROR] create metric_infos failed." << std::endl;
                delete task_ptr;
                continue;
            }

            for(auto iter=task_ptr->begin(); iter!=task_ptr->end(); ++iter) {

                const auto& metric = iter->first;
                auto& run = iter->second;
                metric_info info(run.start_tm_, run.duration_);

                // calc
                info.cnt_ = run.detail_.size();
                info.sum_ = std::accumulate(run.detail_.begin(), run.detail_.end(), 0);
                info.avg_ = info.sum_ / info.cnt_;
                info.tps_ = (info.cnt_ * 1000) / duration_ms_;

                const auto result = std::minmax_element(run.detail_.begin(), run.detail_.end());
                info.min_ = *result.first;
                info.max_ = *result.second;

                size_t p10_idx = info.cnt_ * 0.1;
                std::nth_element(run.detail_.begin(), run.detail_.end() + p10_idx, run.detail_.end());
                info.p10_ = run.detail_[p10_idx];

                size_t p50_idx = info.cnt_ * 0.5;
                std::nth_element(run.detail_.begin(), run.detail_.end() + p50_idx, run.detail_.end());
                info.p50_ = run.detail_[p50_idx];

                size_t p99_idx = info.cnt_ * 0.99;
                std::nth_element(run.detail_.begin(), run.detail_.end() + p99_idx, run.detail_.end());
                info.p99_ = run.detail_[p99_idx];

                size_t p999_idx = info.cnt_ * 0.999;
                std::nth_element(run.detail_.begin(), run.detail_.end() + p999_idx, run.detail_.end());
                info.p999_ = run.detail_[p999_idx];

                infos->insert(std::make_pair(metric, std::move(info)));
            }

            {
                std::lock_guard<std::mutex> lock_persist(mutex_persist_);
                persistence_.push_back(infos);

                // 回环，删除旧的记录
                while(persistence_.size() > sample_) {
                    metric_infos* front_item = persistence_.front();
                    delete front_item;
                    persistence_.pop_front();
                    dropped_sample_ ++;
                }
            }

            delete task_ptr;
        }
    }
}

} // end namespace libpf
