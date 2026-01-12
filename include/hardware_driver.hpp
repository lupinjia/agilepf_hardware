/**
 * @file      hardware_driver.hpp
 * @author    lupinjia (jason_1120202397@163.com)
 * @brief     Class definition of HardwareDriver(interface with low-level hardware)
 * @version   0.1
 * @date      2025-01-10
 * @copyright Copyright (c) 2026 Beijing Institute of Technology, Yasen Jia.
 */
#ifndef __HARDWARE_DRIVER_HPP__
#define __HARDWARE_DRIVER_HPP__

#include <math.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <array>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>
#include <future>
#include <algorithm>
#include <sstream>
#include <sched.h>

#include <common/periodic_task.hpp>
#include <common/crc.h>
#include <idl/agilepf/LowState/LowState.hpp>
#include <idl/agilepf/LowState/LowStatePubSubTypes.hpp>
#include <dds/dds_publisher.hpp>
#include <dds/dds_subscriber.hpp>
#include <idl/agilepf/LowCmd/LowCmd.hpp>
#include <idl/agilepf/LowCmd/LowCmdPubSubTypes.hpp>

#include "hardware_interface.hpp"
#include "cfg.hpp"
#include "common/mathTools.h"

namespace fs = std::filesystem;
using namespace legged::dds;
using namespace legged::common;

#define TOPIC_LOWCMD "rt/lowcmd"
#define TOPIC_LOWSTATE "rt/lowstate"

class HardwareDriver
{
public:
    HardwareDriver() {}

    HardwareDriver(int domain_id, std::string interface_name) :
        domain_id_(domain_id), interface_name_(interface_name)
    {
        // init low state
        initLowState();
        // init low cmd
        initLowCmd();
    }

    void loadParam(const std::string& cfg_file)
    {
        RLCfg cfg(cfg_file);
        spdlog::info("Load Hardware Driver Parameters from {}", cfg_file);
    }

    void initDDSPubSub()
    {
        low_state_publisher = std::make_shared<DDSPublisher<legged::idl::agilepf::LowState,
                        legged::idl::agilepf::LowStatePubSubType>>(domain_id_, interface_name_, TOPIC_LOWSTATE, 2000.0f);
        low_cmd_subscriber = std::make_shared<DDSSubscriber<legged::idl::agilepf::LowCmd,
                         legged::idl::agilepf::LowCmdPubSubType>>(domain_id_, interface_name_, TOPIC_LOWCMD, 
                            std::bind(&HardwareDriver::lowCmdMessageHandler, this, std::placeholders::_1), 2000.0f);
        
        low_state_publisher->init();
        low_cmd_subscriber->init();
    }

    void start()
    {
        std::chrono::milliseconds duration(100); // 100ms

        // prepare for start
        spdlog::info("Start Hardware Driver");

        // Start the lowlevel state thread
        std::this_thread::sleep_for(duration);
        startSendLowState();
        spdlog::info("Start Send LowState");

        // keep the main thread alive
        while (true)
        {
            std::this_thread::sleep_for(duration);
        }
    }

protected:
    DDSPublisherPtr<legged::idl::agilepf::LowState,
                        legged::idl::agilepf::LowStatePubSubType> low_state_publisher;
    DDSSubscriberPtr<legged::idl::agilepf::LowCmd,
                         legged::idl::agilepf::LowCmdPubSubType> low_cmd_subscriber;
    PeriodicTaskPtr low_state_publish_task;

    legged::idl::agilepf::LowCmd low_cmd;
    legged::idl::agilepf::LowState low_state;

    HardwareInterface hardware_interface;

    uint64_t low_level_dt_micro_sec = 2000; // timestep for low-level communication loop

    int domain_id_;
    std::string interface_name_;

private:
        
    // 500Hz
    void lowCmdMessageHandler(const void* msg)
    {
        low_cmd = *(legged::idl::agilepf::LowCmd*)msg;
        if(low_cmd.header()[0] == 0xAA && low_cmd.header()[1] == 0xBB &&
           low_cmd.tail()[0] == 0xEE && low_cmd.tail()[1] == 0xFF)
        {
            hardware_interface.updateLowCmd(low_cmd);
        }
        else
        {
            spdlog::warn("LowCmd Message Header or Tail Error!");
        }
        
        // uncomment following lines for debug of test_low_cmd_pub.cpp
        // spdlog::info("Motor Cmd Received");
        // spdlog::info("Motor Pos Des: [{:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}]",
        //     low_cmd.motor_cmd()[0].q(),
        //     low_cmd.motor_cmd()[1].q(),
        //     low_cmd.motor_cmd()[2].q(),
        //     low_cmd.motor_cmd()[3].q(),
        //     low_cmd.motor_cmd()[4].q(),
        //     low_cmd.motor_cmd()[5].q());
        // spdlog::info("Motor Kp: {:.3f}, Kd: {:.3f}",
        //     low_cmd.motor_cmd()[0].kp(),
        //     low_cmd.motor_cmd()[0].kd());
    }

    // 500Hz
    void lowStatewriteHandler()
    {
        hardware_interface.updateLowState(low_state);
        low_state_publisher->publish(&low_state);
    }

    void initLowState()
    {
        // initialize header and tail
        low_state.header() = {0xAA, 0xBB};
        low_state.tail() = {0xEE, 0xFF};
    }

    void initLowCmd()
    {
        // initialize header and tail
        low_cmd.header() = {0xAA, 0xBB};
        low_cmd.tail() = {0xEE, 0xFF};
        // initialize motor command to zero
        for(uint8_t i = 0; i < 6; i++)
        {
            low_cmd.motor_cmd()[i].q() = 0.0f;
            low_cmd.motor_cmd()[i].dq() = 0.0f;
            low_cmd.motor_cmd()[i].kp() = 0.0f;
            low_cmd.motor_cmd()[i].kd() = 0.0f;
            low_cmd.motor_cmd()[i].tau() = 0.0f;
        }
    }

    void startSendLowState()
    {
        // write_low_state thread: 2000us(2ms), 500Hz; running on CPU4
        low_state_publish_task = createPeriodicTask("write_low_state", 2000, 
            std::bind(&HardwareDriver::lowStatewriteHandler, this),
            0, SCHED_FIFO, 99);
    }
};

#endif // __HARDWARE_DRIVER_HPP__