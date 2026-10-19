# agilepf_hardware

This project is the hardware of AgilePF (Agile Point-Foot biped robot), which serves as the bridge between the low-level communication (USB, TTL, e.t.c.) and dds middleware.

This project is mainly for subscribing and publishing dds messages and transform them into low-level data to communicate with peripherals on the robot. Compared to biped_rl_controller, this project doesn't include the functionality of neural network policy inference.

**Author**: Yasen Jia  
**Email**: jason_1120202397@163.com

## Dependencies

- legged_sdk
- yaml-cpp
- libserial
- gainput
- PREEMPT_RT

## Installation

1. Install [legged_sdk](https://gitee.com/jia-yasen/legged_sdk) under `/opt/legged_sdk`

    ```bash
    git clone https://gitee.com/jia-yasen/legged_sdk
    cd legged_sdk/
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/opt/legged_sdk
    make
    sudo make install
    ```

2. Install Gainput under `/usr/local`
    ```bash
    git clone https://github.com/lupinjia/gainput.git
    cd gainput
    mkdir build && cd build
    cmake ..
    make
    make install
    ```

3. Install libserial (`sudo apt install libserial-dev`), yaml-cpp (`sudo apt install libyaml-cpp-dev`) and PREEMPT_RT (google it)

4. Clone this repo and compile
    ```bash
    git clone https://gitee.com/jia-yasen/agilepf_hardware.git
    mkdir build && cd build
    cmake ..
    make
    ```

5. Run agilepf_hardware

    By default, the network interface is "lo". To enable data transmission through udp, you need to specify network interface (can be seen through `ifconfig`) like:
    ```bash
    ./agilepf_hardware network_interface
    ```

    To make the program achieve real-time performance, you need to run it as sudo:
    ```bash
    sudo -i
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/legged_sdk/lib
    ./agilepf_hardware network_interface
    ```

## Read-only monitor

`agilepf-monitor` subscribes to `rt/lowstate` only. It never publishes
`rt/lowcmd`, opens the hardware serial devices, or requires sudo/PREEMPT_RT.
Network interface access can still depend on local DDS/network policy.

The monitor defaults to DDS domain **1**, interface `lo`, and topic
`rt/lowstate`. This differs from running `agilepf_hardware lo`, which selects
domain **0** because the hardware executable switches domains when an interface
argument is supplied. For the driver's default settings, run the monitor with
no interface override:

```bash
./agilepf-monitor
./agilepf-monitor --domain 0 --interface eth0 --rate 10
./agilepf-monitor --format json
./agilepf-monitor --once --wait-timeout-ms 5000
```

`--once` waits for a first message (up to 5 seconds by default), prints one
snapshot, then exits; timeout is reported on stderr with a nonzero status.
The wait deadline includes SDK subscriber initialization, although a blocking
SDK `init()` cannot be interrupted and may delay when timeout is reported.
Non-terminal output is newline-delimited JSON when `--format json` is selected;
text output is also plain line-oriented when stdout is not an interactive
terminal. On a terminal, width and height are queried as the display runs; if
the rendered frame will not fit vertically, output falls back to plain text
snapshots instead of clearing the screen. Ctrl+C or SIGTERM stops the display.
`--help` prints CLI help without initializing the SDK.

The monitor build still requires `legged_sdk` installed under
`/opt/legged_sdk`; SDK lifecycle behavior has not been validated here. To
configure only the monitor without hardware-bridge dependencies:

```bash
cmake -S . -B build-monitor -DAGILEPF_BUILD_HARDWARE=OFF
cmake --build build-monitor --target agilepf-monitor
```

Application diagnostics are written to stderr. The external SDK's own logging
destination is SDK-controlled and has not been verified, so SDK startup logs
may be outside the monitor's JSON stdout guarantees.

Only fields populated by `HardwareInterface::updateLowState` are displayed:
six motor positions and velocities, IMU quaternion/gyro/acceleration/RPY, and
joystick axes/buttons. Torque, temperature, voltage, and fault values are not
available from this bridge's populated state. Reported rate and age describe
DDS message arrivals, not sensor feedback validity; stale DDS data does not
establish device health. Core tests use synthetic snapshots and do not claim
SDK/DDS integration coverage. No real SDK compile validation is claimed when
`/opt/legged_sdk` is unavailable.

Offline tests do not require the SDK:

```bash
g++ -std=c++17 -pthread -Iinclude tests/monitor_core_test.cpp -o /tmp/monitor_core_test
/tmp/monitor_core_test
g++ -std=c++17 -Iinclude tests/monitor_view_test.cpp -o /tmp/monitor_view_test
/tmp/monitor_view_test
```
