#pragma once

#include "monitor_types.hpp"

#include <chrono>
#include <cmath>
#include <deque>
#include <mutex>

namespace agilepf::monitor {

class SnapshotStore {
public:
    using Clock = std::chrono::steady_clock;

    explicit SnapshotStore(double stale_after_ms = 200) : stale_after_ms_(stale_after_ms) {}

    void update(const MonitorSnapshot& incoming, Clock::time_point now = Clock::now()) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (received_) {
            last_interval_ms_ = std::chrono::duration<double, std::milli>(now - last_).count();
            if (last_interval_ms_ > stale_after_ms_) arrivals_.clear();
        }
        received_ = true;
        last_ = now;
        arrivals_.push_back(now);
        while (arrivals_.size() > 2 && std::chrono::duration<double, std::milli>(now - arrivals_.front()).count() > kRateWindowMs) arrivals_.pop_front();
        while (arrivals_.size() > kMaxArrivals) arrivals_.pop_front();
        snapshot_ = incoming;
        snapshot_.received = true;
        ++count_;
    }

    MonitorSnapshot snapshot(double stale_after_ms, Clock::time_point now = Clock::now()) const {
        std::lock_guard<std::mutex> lock(mutex_);
        MonitorSnapshot result = snapshot_;
        result.received = received_;
        result.message_count = count_;
        if (!received_) return result;
        result.age_ms = std::chrono::duration<double, std::milli>(now - last_).count();
        result.stale = result.age_ms > stale_after_ms;
        result.last_interval_ms = last_interval_ms_;
        const double span_ms = arrivals_.size() >= 2 ?
            std::chrono::duration<double, std::milli>(arrivals_.back() - arrivals_.front()).count() : 0;
        result.rx_hz = span_ms > 0 && !result.stale ?
            (arrivals_.size() - 1) * 1000.0 / span_ms : 0;
        return result;
    }

private:
    static constexpr double kRateWindowMs = 5000;
    static constexpr std::size_t kMaxArrivals = 8192;
    mutable std::mutex mutex_;
    const double stale_after_ms_;
    MonitorSnapshot snapshot_{};
    bool received_ = false;
    std::uint64_t count_ = 0;
    Clock::time_point last_{};
    double last_interval_ms_ = 0;
    std::deque<Clock::time_point> arrivals_;
};

} // namespace agilepf::monitor
