#ifndef __IMU_MSG_H__
#define __IMU_MSG_H__

#pragma pack(push, 1) // cancel byte alignment

typedef struct
{
  float quat_w;
  float quat_x;
  float quat_y;
  float quat_z;
  float ang_vel_x;
  float ang_vel_y;
  float ang_vel_z;
  float lin_acc_x;
  float lin_acc_y;
  float lin_acc_z;
  float roll;
  float pitch;
  float yaw;
}ImuMsg;

#pragma pack(pop) // restore byte alignment

#endif