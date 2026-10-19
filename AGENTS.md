# Project reference

- C++17 hardware bridge; see `README.md` for build and hardware setup. CMake
  expects `legged_sdk` under `/opt/legged_sdk`; its DDS wrappers and generated
  IDL headers are external to this repository.
- `src/main.cpp` selects DDS domain 1/interface `lo` with no argument, or
  domain 0/the supplied interface with an argument. Passing `lo` explicitly
  therefore differs from passing no argument.
- `include/hardware_driver.hpp` publishes `rt/lowstate` and subscribes to
  `rt/lowcmd`. The state task is configured for 2000 us (500 Hz),
  `SCHED_FIFO` priority 99; this is a target, not a measured rate.
- `HardwareInterface::updateLowState` populates IMU, joystick, and six motors'
  position/velocity. Motor indices 0-2 are left, 3-5 right. Do not infer valid
  torque, temperature, voltage, or fault data from other external IDL fields.
- Monitoring should subscribe to DDS rather than instantiate
  `HardwareInterface` or reopen its serial devices. A read-only monitor must
  not publish `rt/lowcmd` or require the driver's real-time scheduling policy.
- DDS message arrival freshness does not prove sensor feedback freshness:
  current state publication has no per-device valid-frame timestamp or CRC
  counters. Do not label a device healthy solely because DDS messages arrive.
- Topic names and state task period are currently hard-coded; loading
  `params/driver_params.yaml` does not apply those settings.
- Do not run the hardware executable or fault-injection tests without explicit
  hardware authorization. Keep formatting, terminal output, and file I/O out
  of the real-time state task.
- `agilepf-monitor` is a separate read-only DDS subscriber. Configure with
  `-DAGILEPF_BUILD_HARDWARE=OFF` to avoid the bridge's serial/gamepad build
  dependencies; the monitor still requires `legged_sdk`.
- Monitor parsing/state and rendering tests are SDK-free:
  `g++ -std=c++17 -pthread -Iinclude tests/monitor_core_test.cpp -o /tmp/opencode/monitor_core_test`
  and `g++ -std=c++17 -Iinclude tests/monitor_view_test.cpp -o /tmp/opencode/monitor_view_test`.
  Run the resulting binaries for offline checks; these do not validate the
  external SDK's ABI, callback teardown, or live DDS discovery.
