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
    ``