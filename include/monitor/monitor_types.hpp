#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace agilepf::monitor {

struct MonitorSnapshot {
    bool received = false;
    bool stale = false;
    std::uint64_t message_count = 0;
    double rx_hz = 0;
    double age_ms = 0;
    double last_interval_ms = 0;
    std::array<double, 6> q{};
    std::array<double, 6> dq{};
    std::array<double, 4> quaternion{};
    std::array<double, 3> gyroscope{};
    std::array<double, 3> accelerometer{};
    std::array<double, 3> rpy{};
    std::array<double, 4> axes{}; // left_x, left_y, right_x, right_y
    std::array<bool, 12> buttons{}; // start, select, left, right, up, down, a, b, x, y, l1, r1
};

struct ViewConfig {
    int domain = 1;
    std::string interface_name = "lo";
    std::string topic = "rt/lowstate";
    double refresh_hz = 10;
    double stale_after_ms = 200;
    std::size_t columns = 100;
};

} // namespace agilepf::monitor
