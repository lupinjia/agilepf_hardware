#ifndef __LOW_CMD_H__
#define __LOW_CMD_H__

#pragma pack(push, 1) // cancel byte alignment

typedef struct
{
  float pos_des[6]; // 6 motors
  float kp;
  float kd;
}LowCmd;

#pragma pack(pop) // restore byte alignment

#endif