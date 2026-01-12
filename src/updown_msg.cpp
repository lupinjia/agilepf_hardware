/**
 * @file    updown_msg.cpp
 * @author  lupinjia(jason_1120202397@163.com)
 * @brief   contains definitions and functions for upboard-downboard communication
 * @version 1.0
 * @date    2024-12-08
 * 
 * @copyright HYGroup @ BIT 2024
 * 
 */
#include "msg/updown_msg.h"

/********** function implementations **********/
// refer to https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li
uint32_t calc_crc32(const uint8_t *data, uint8_t len) {
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (i < len) {
      byte = data[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

void update_upper2lower_msg(Upper2LowerMsg* msg)
{
   msg->header1 = 0xAA;
   msg->header2 = 0xBB;
   msg->crc32 = calc_crc32((uint8_t*)(msg->pos_des), CMD_DATA_LEN);
   msg->tail1 = 0xEE;
   msg->tail2 = 0xFF;
}