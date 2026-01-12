#ifndef _UART_COMM_HPP_
#define _UART_COMM_HPP_

#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>
#include <libserial/SerialPort.h>

#include "msg/low_cmd.h"
#include "msg/low_state.h"
#include "msg/updown_msg.h"
#include "common/mathTools.h"

namespace fs = std::filesystem;
constexpr const char* const SERIAL_PORT_1 = "/dev/ttyACM0"; // usb设备名
using namespace LibSerial;

class UartComm
{
public:
  UartComm(): upper2lower_msg(new Upper2LowerMsg()), lower2upper_msg(new Lower2UpperMsg()), 
              low_state(new LowState())
  {
    // initialize uart
    try
    {
      serial_port.Open(SERIAL_PORT_1);
    }
    catch(const OpenFailed&)
    {
      spdlog::error("Failed to open serial port {} for uart communication", SERIAL_PORT_1);
    }
    serial_port.SetBaudRate(BaudRate::BAUD_4000000);
    serial_port.SetCharacterSize(CharacterSize::CHAR_SIZE_8);
    serial_port.SetParity(Parity::PARITY_NONE);
    serial_port.SetStopBits(StopBits::STOP_BITS_1);
    // initialize motor command
    for(uint8_t i = 0; i < 6; i++)
    {
      upper2lower_msg->pos_des[i] = 0;
    }
    // initialize motor feedback
    for(uint8_t i = 0; i < 3; i++)
    {
      low_state->motor_fb.left_angle[i] = 0.;
      low_state->motor_fb.left_speed[i] = 0.;
      low_state->motor_fb.right_angle[i] = 0.;
      low_state->motor_fb.right_speed[i] = 0.;
    }
  }

  void setLogFile(fs::path &log_file_name)
  {
    // set log file
    log_file = std::ofstream(log_file_name);
    std::stringstream header;
    header << "rev_duration";
    header << std::endl;
    log_file << header.str();
  }

  void updateLowCmd(const LowCmd & low_cmd) const
  {
    for(uint8_t i = 0; i < 6; i++)
    {
      upper2lower_msg->pos_des[i] = float_to_uint(low_cmd.pos_des[i], POS_MIN, POS_MAX, 16);
    }
    upper2lower_msg->kp = float_to_uint(low_cmd.kp, KP_MIN, KP_MAX, 16);
    upper2lower_msg->kd = float_to_uint(low_cmd.kd, KD_MIN, KD_MAX, 16);
  }

  void sendCmd()
  {
    // update message
    update_upper2lower_msg(upper2lower_msg);
    DataBuffer tx_buffer;
    std::copy(reinterpret_cast<const char*>(upper2lower_msg), 
              reinterpret_cast<const char*>(upper2lower_msg) + sizeof(Upper2LowerMsg), 
              std::back_inserter(tx_buffer));
    // send message
    serial_port.Write(tx_buffer);
  }

  void updateLowState()
  {
    DataBuffer rx_buffer;
    serial_port.Read(rx_buffer, sizeof(Lower2UpperMsg), 5); // 5ms timeout
    if (rx_buffer[0] == 0xAA && rx_buffer[1] == 0xBB && rx_buffer[FEEDBACK_FRAME_LEN-2] == 0xEE && rx_buffer[FEEDBACK_FRAME_LEN-1] == 0xFF)
    {
      std::copy(rx_buffer.begin(), rx_buffer.end(), reinterpret_cast<char*>(lower2upper_msg));
      // check crc
      crc_verification = calc_crc32((uint8_t*)&lower2upper_msg->motor_fbs_logical, FEEDBACK_FRAME_LEN-8);
      if (crc_verification == lower2upper_msg->crc32)
      {
        // process motor feedbacks
        for(uint8_t i = 0; i < 3; i++)
        {
          // left
          low_state->motor_fb.left_angle[i] = uint_to_float(lower2upper_msg->motor_fbs_logical.left[i].angle,POS_MIN,POS_MAX,16);
          low_state->motor_fb.left_speed[i] = uint_to_float(lower2upper_msg->motor_fbs_logical.left[i].speed,SPD_MIN,SPD_MAX,12);
          // right
          low_state->motor_fb.right_angle[i] = uint_to_float(lower2upper_msg->motor_fbs_logical.right[i].angle,POS_MIN,POS_MAX,16);
          low_state->motor_fb.right_speed[i] = uint_to_float(lower2upper_msg->motor_fbs_logical.right[i].speed,SPD_MIN,SPD_MAX,12);
        }
        // // evaluate real-time performance
        // uart_rev_time_last = uart_rev_time_this;
        // uart_rev_time_this = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(uart_rev_time_this-uart_rev_time_last);
        // if(log_file.is_open()) // write to log file
        // {
        //   log_file << duration.count() / 1000. << ","; // us->ms
        //   log_file << std::endl;
        // }
      }
      else
      {
        spdlog::error("CRC verification failed!");
        serial_port.FlushInputBuffer();
      }
      memset(lower2upper_msg, 0, FEEDBACK_FRAME_LEN);
    }
    else // header and tail qualified
    {
      spdlog::error("Invalid message header or tail!");
      serial_port.FlushInputBuffer();
    }
  }

  LowState* low_state;

private:
  SerialPort serial_port;
  uint32_t crc_verification = 0;
  Upper2LowerMsg* upper2lower_msg;
  Lower2UpperMsg* lower2upper_msg;
  std::ofstream log_file;
  // spi loop evaluation
  std::vector<float> uart_rev_interval;
  std::chrono::time_point<std::chrono::high_resolution_clock> uart_rev_time_this;
  std::chrono::time_point<std::chrono::high_resolution_clock> uart_rev_time_last;
};

#endif