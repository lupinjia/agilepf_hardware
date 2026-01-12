#ifndef __HARDWARE_INTERFACE_HPP__
#define __HARDWARE_INTERFACE_HPP__

#include <array>
#include <chrono>
#include <cmath>
#include <idl/agilepf/LowState/LowState.hpp>
#include "module/imu_reader.hpp"
#include "module/uart_comm.hpp"
#include "module/gamepad_interface.h"
#include <cstring>

class HardwareInterface
{
public:

  HardwareInterface()
  {
    for(uint8_t i = 0; i < 6; i++)
    {
      low_cmd_uart.pos_des[i] = 0.;
    }
    low_cmd_uart.kp = 0.0;
    low_cmd_uart.kd = 0.0;
    initGamepadInterface();
  }

  void updateLowState(legged::idl::agilepf::LowState &low_state)
  {
    // imu
    imu_reader.imu_read();
    low_state.imu_state().quaternion()[0] = imu_reader.imu_data.quat_w;
    low_state.imu_state().quaternion()[1] = imu_reader.imu_data.quat_x;
    low_state.imu_state().quaternion()[2] = imu_reader.imu_data.quat_y;
    low_state.imu_state().quaternion()[3] = imu_reader.imu_data.quat_z;
    low_state.imu_state().gyroscope()[0] = imu_reader.imu_data.ang_vel_x;
    low_state.imu_state().gyroscope()[1] = imu_reader.imu_data.ang_vel_y;
    low_state.imu_state().gyroscope()[2] = imu_reader.imu_data.ang_vel_z;
    low_state.imu_state().accelerometer()[0] = imu_reader.imu_data.lin_acc_x;
    low_state.imu_state().accelerometer()[1] = imu_reader.imu_data.lin_acc_y;
    low_state.imu_state().accelerometer()[2] = imu_reader.imu_data.lin_acc_z;
    low_state.imu_state().rpy()[0] = imu_reader.imu_data.roll;
    low_state.imu_state().rpy()[1] = imu_reader.imu_data.pitch;
    low_state.imu_state().rpy()[2] = imu_reader.imu_data.yaw;

    // joystick
    gamepad_interface->run();
    // align lx value with that in simulation
    low_state.joystick_data().left_stick_x() = gamepad_data.lx;
    low_state.joystick_data().left_stick_y() = -1*gamepad_data.ly;
    low_state.joystick_data().right_stick_x() = gamepad_data.rx;
    low_state.joystick_data().right_stick_y() = gamepad_data.ry;
    low_state.joystick_data().start_button() = gamepad_data.start;
    low_state.joystick_data().select_button() = gamepad_data.select;
    low_state.joystick_data().left_button() = gamepad_data.left;
    low_state.joystick_data().right_button() = gamepad_data.right;
    low_state.joystick_data().up_button() = gamepad_data.up;
    low_state.joystick_data().down_button() = gamepad_data.down;
    low_state.joystick_data().a_button() = gamepad_data.a;
    low_state.joystick_data().b_button() = gamepad_data.b;
    low_state.joystick_data().x_button() = gamepad_data.x;
    low_state.joystick_data().y_button() = gamepad_data.y;
    low_state.joystick_data().l1_button() = gamepad_data.l1;
    low_state.joystick_data().r1_button() = gamepad_data.r1;
    
    // motor
    uart_comm.updateLowState();
    // motor angle and agular vel
    for(uint8_t i = 0; i < 3; i++)
    {
      low_state.motor_state()[i].q() = uart_comm.low_state->motor_fb.left_angle[i];
      low_state.motor_state()[i].dq() = uart_comm.low_state->motor_fb.left_speed[i];
      low_state.motor_state()[i+3].q() = uart_comm.low_state->motor_fb.right_angle[i];
      low_state.motor_state()[i+3].dq() = uart_comm.low_state->motor_fb.right_speed[i];
    }
  }

  void updateLowCmd(const legged::idl::agilepf::LowCmd &low_cmd_dds)
  {
    // only use q for now
    for(uint8_t i = 0; i < 6; i++)
    {
      low_cmd_uart.pos_des[i] = low_cmd_dds.motor_cmd()[i].q();
    }
    // assume that all motors share the same kp and kd
    low_cmd_uart.kp = low_cmd_dds.motor_cmd()[0].kp();
    low_cmd_uart.kd = low_cmd_dds.motor_cmd()[0].kd();
    uart_comm.updateLowCmd(low_cmd_uart);
    uart_comm.sendCmd();
  }

private:
  void initGamepadInterface()
  {
    gamepad_interface = new GamepadInterface(gamepad_data);
    gamepad_interface->init();
  }

  ImuReader imu_reader;
  UartComm uart_comm;
  GamepadInterface* gamepad_interface = nullptr;
  GamepadData gamepad_data;
  LowCmd low_cmd_uart;
};

#endif