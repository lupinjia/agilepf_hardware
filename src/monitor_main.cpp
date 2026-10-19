#include "monitor/monitor_options.hpp"
#include "monitor/monitor_state.hpp"
#include "monitor/terminal_view.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <dds/dds_subscriber.hpp>
#include <idl/agilepf/LowState/LowState.hpp>
#include <idl/agilepf/LowState/LowStatePubSubTypes.hpp>

namespace {
volatile std::sig_atomic_t stop_requested = 0;
void requestStop(int) { stop_requested = 1; }

void updateTerminalSize(agilepf::monitor::ViewConfig& config, unsigned short* rows = nullptr) {
    struct winsize size {};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) return;
    if (size.ws_col != 0) config.columns = size.ws_col;
    if (rows && size.ws_row != 0) *rows = size.ws_row;
}

void sleepUntilOrStop(agilepf::monitor::SnapshotStore::Clock::time_point deadline) {
    using Clock = agilepf::monitor::SnapshotStore::Clock;
    constexpr auto quantum = std::chrono::duration_cast<Clock::duration>(std::chrono::milliseconds(50));
    while (!stop_requested) {
        const auto now = Clock::now();
        if (now >= deadline) return;
        std::this_thread::sleep_for(std::min(quantum, deadline - now));
    }
}

void usage(std::ostream& out) {
    out << "Usage: agilepf-monitor [options]\n"
           "  --domain N              DDS domain (0..232; default 1)\n"
           "  --interface NAME        network interface (default lo)\n"
           "  --topic NAME            DDS topic (default rt/lowstate)\n"
           "  --rate HZ               display refresh 0.1..100 (default 10)\n"
           "  --timeout-ms MS         stale threshold (default 200)\n"
           "  --format text|json      output format (default text)\n"
           "  --once                  print one received sample and exit\n"
           "  --wait-timeout-ms MS    --once wait limit (default 5000)\n"
           "  --help                  show this help\n";
}

agilepf::monitor::MonitorSnapshot decode(const void* raw) {
    const auto& msg = *static_cast<const legged::idl::agilepf::LowState*>(raw);
    agilepf::monitor::MonitorSnapshot s;
    for (std::size_t i = 0; i < 6; ++i) {
        s.q[i] = msg.motor_state()[i].q();
        s.dq[i] = msg.motor_state()[i].dq();
    }
    const auto& imu = msg.imu_state();
    for (std::size_t i = 0; i < 4; ++i) s.quaternion[i] = imu.quaternion()[i];
    for (std::size_t i = 0; i < 3; ++i) {
        s.gyroscope[i] = imu.gyroscope()[i];
        s.accelerometer[i] = imu.accelerometer()[i];
        s.rpy[i] = imu.rpy()[i];
    }
    const auto& joy = msg.joystick_data();
    s.axes = {joy.left_stick_x(), joy.left_stick_y(), joy.right_stick_x(), joy.right_stick_y()};
    s.buttons = {joy.start_button(), joy.select_button(), joy.left_button(), joy.right_button(),
                 joy.up_button(), joy.down_button(), joy.a_button(), joy.b_button(),
                 joy.x_button(), joy.y_button(), joy.l1_button(), joy.r1_button()};
    return s;
}
}

int main(int argc, char** argv) {
    agilepf::monitor::Options options;
    std::string error;
    if (!agilepf::monitor::parseOptions(argc, argv, options, error)) {
        std::cerr << "agilepf-monitor: " << error << '\n';
        usage(std::cerr);
        return 2;
    }
    if (options.help) { usage(std::cout); return 0; }

    std::signal(SIGINT, requestStop);
    std::signal(SIGTERM, requestStop);
    std::signal(SIGPIPE, SIG_IGN);
    const bool json = options.format == "json";
    const bool terminal = !json && ::isatty(STDOUT_FILENO) && std::getenv("TERM") && std::string(std::getenv("TERM")) != "dumb";
    auto store = std::make_shared<agilepf::monitor::SnapshotStore>(options.view.stale_after_ms);
    const auto wait_deadline = agilepf::monitor::SnapshotStore::Clock::now() +
        std::chrono::duration_cast<agilepf::monitor::SnapshotStore::Clock::duration>(std::chrono::duration<double, std::milli>(options.wait_timeout_ms));
    try {
        using Subscriber = legged::dds::DDSSubscriber<legged::idl::agilepf::LowState,
            legged::idl::agilepf::LowStatePubSubType>;
        // Keep the store alive with the callback; SDK listener teardown still needs live validation.
        Subscriber subscriber(options.view.domain, options.view.interface_name, options.view.topic,
            [store](const void* raw) {
                try { store->update(decode(raw)); } catch (...) { /* Never propagate through the SDK callback. */ }
            }, 2000.0f);
        subscriber.init();

        if (options.once) {
            while (!stop_requested && !store->snapshot(options.view.stale_after_ms).received &&
                   agilepf::monitor::SnapshotStore::Clock::now() < wait_deadline)
                sleepUntilOrStop(std::min(wait_deadline, agilepf::monitor::SnapshotStore::Clock::now() + std::chrono::milliseconds(50)));
            if (!store->snapshot(options.view.stale_after_ms).received) {
                if (!stop_requested) std::cerr << "agilepf-monitor: timed out waiting for first LowState message\n";
                return stop_requested ? 0 : 1;
            }
            updateTerminalSize(options.view);
            const auto snap = store->snapshot(options.view.stale_after_ms);
            std::cout << (json ? agilepf::monitor::renderJson(snap, options.view) : agilepf::monitor::renderText(snap, options.view, false)) << '\n' << std::flush;
            return 0; // A closed pipe is a normal output termination.
        }

        auto next = agilepf::monitor::SnapshotStore::Clock::now();
        std::string previous;
        while (!stop_requested) {
            unsigned short rows = 0;
            if (terminal) updateTerminalSize(options.view, &rows);
            const auto snapshot = store->snapshot(options.view.stale_after_ms);
            std::string frame;
            bool redraw = false;
            if (terminal) {
                const std::string plain = agilepf::monitor::renderText(snapshot, options.view, false);
                const auto frame_rows = static_cast<unsigned short>(std::count(plain.begin(), plain.end(), '\n'));
                redraw = rows != 0 && frame_rows < rows;
                frame = redraw ? agilepf::monitor::renderText(snapshot, options.view, true) : plain;
            } else {
                frame = json ? agilepf::monitor::renderJson(snapshot, options.view) : agilepf::monitor::renderText(snapshot, options.view, false);
            }
            if (terminal && redraw) std::cout << frame << std::flush;
            else if (frame != previous) { std::cout << frame << '\n' << std::flush; previous = frame; }
            if (!std::cout) break;
            next += std::chrono::duration_cast<agilepf::monitor::SnapshotStore::Clock::duration>(std::chrono::duration<double>(1.0 / options.view.refresh_hz));
            sleepUntilOrStop(next);
            if (agilepf::monitor::SnapshotStore::Clock::now() - next > std::chrono::seconds(1)) next = agilepf::monitor::SnapshotStore::Clock::now();
        }
    } catch (const std::exception& e) {
        std::cerr << "agilepf-monitor: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
