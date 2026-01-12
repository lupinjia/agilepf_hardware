#ifndef __LOW_STATE_H__
#define __LOW_STATE_H__

#pragma pack(push, 1) // cancel byte alignment

typedef struct
{
  float left_angle[3];
  float left_speed[3];
  float right_angle[3];
  float right_speed[3];
}MotorFeedback;

typedef struct
{
  MotorFeedback motor_fb;
}LowState;

#pragma pack(pop) // restore byte alignment

#endif // __LOW_STATE_H__