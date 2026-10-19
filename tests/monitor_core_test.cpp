#include "monitor/monitor_options.hpp"
#include "monitor/monitor_state.hpp"

#include <cassert>
#include <cmath>
#include <string>

int main() {
    using namespace agilepf::monitor;
    Options options;
    std::string error;
    char a0[] = "monitor", a1[] = "--domain", a2[] = "0", a3[] = "--rate", a4[] = "20", a5[] = "--once";
    char* argv[] = {a0, a1, a2, a3, a4, a5};
    assert(parseOptions(6, argv, options, error));
    assert(options.view.domain == 0 && options.view.refresh_hz == 20 && options.once);
    char b0[] = "monitor", b1[] = "--domain", b2[] = "999999999999999999999";
    char* bad_domain[] = {b0, b1, b2};
    assert(!parseOptions(3, bad_domain, options, error));
    char c0[] = "monitor", c1[] = "operand";
    char* operand[] = {c0, c1};
    assert(!parseOptions(2, operand, options, error));

    SnapshotStore store;
    const auto t0 = SnapshotStore::Clock::time_point{};
    MonitorSnapshot sample;
    sample.q[0] = 1.25;
    assert(!store.snapshot(200, t0).received);
    store.update(sample, t0);
    auto first = store.snapshot(200, t0 + std::chrono::milliseconds(10));
    assert(first.received && !first.stale && first.message_count == 1 && first.q[0] == 1.25 && first.rx_hz == 0);
    store.update(sample, t0 + std::chrono::milliseconds(100));
    auto second = store.snapshot(200, t0 + std::chrono::milliseconds(110));
    assert(second.message_count == 2 && second.last_interval_ms == 100 && second.rx_hz == 10);
    auto stale = store.snapshot(200, t0 + std::chrono::milliseconds(400));
    assert(stale.stale && stale.rx_hz == 0);
    store.update(sample, t0 + std::chrono::milliseconds(500));
    assert(store.snapshot(200, t0 + std::chrono::milliseconds(500)).rx_hz == 0);
    store.update(sample, t0 + std::chrono::milliseconds(600));
    assert(store.snapshot(200, t0 + std::chrono::milliseconds(600)).rx_hz == 10);

    SnapshotStore duplicates;
    duplicates.update(sample, t0);
    duplicates.update(sample, t0);
    assert(duplicates.snapshot(200, t0).rx_hz == 0);
    duplicates.update(sample, t0 + std::chrono::milliseconds(100));
    assert(std::isfinite(duplicates.snapshot(200, t0 + std::chrono::milliseconds(100)).rx_hz));
}
