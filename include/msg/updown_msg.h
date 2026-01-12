#ifndef _UPDOWN_MSG_H_
#define _UPDOWN_MSG_H_
/********** Includes **********/
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1) // cancel byte alignment

/*********** Defines **********/

#define MOTOR_NUM_PER_LEG     3
#define MOTOR_NUM_TOTAL       6
#define CMD_DATA_LEN  16
#define KP_MIN 0.0f
#define KP_MAX 500.0f
#define KD_MIN 0.0f
#define KD_MAX 5.0f
#define POS_MIN -12.5f
#define POS_MAX 12.5f
#define SPD_MIN -18.0f
#define SPD_MAX 18.0f
#define T_MIN -90.0f
#define T_MAX 90.0f
#define I_MIN -30.0f
#define I_MAX 30.0f

#define RPM2RAD 0.10471975511965977
#define RAD2RPM 9.549296585513721
#define DEG2RAD 0.017453292519943295
#define RAD2DEG 57.29577951308232

// motor feedback data processed into logical values with int16_t type
// transimitted to upper board
// 4 bytes
typedef struct
{
    uint16_t angle;
    uint16_t speed;
}MotorFeedbackLogical;

// Integration of MotorFeedbackLogical
// 24 bytes
typedef struct
{
    MotorFeedbackLogical left[MOTOR_NUM_PER_LEG];
    MotorFeedbackLogical right[MOTOR_NUM_PER_LEG];
}MotorFeedbacksLogical;

// Message transmitted to upper board
// 32 bytes
typedef struct 
{
  uint8_t header1;
  uint8_t header2;
  MotorFeedbacksLogical motor_fbs_logical;
  uint32_t crc32;
  uint8_t tail1;
  uint8_t tail2;
}Lower2UpperMsg;

#define FEEDBACK_FRAME_LEN  sizeof(Lower2UpperMsg)// 32 bytes

/*********** Typedefs **********/
// Message sent from upper board
// 24 bytes
typedef struct
{
  uint8_t header1;
  uint8_t header2;
  uint16_t pos_des[MOTOR_NUM_TOTAL];
  uint16_t kp;
  uint16_t kd;
  uint32_t crc32;
  uint8_t tail1;
  uint8_t tail2;
}Upper2LowerMsg;

/********** Function Prototypes **********/

/**
 * @brief calculate crc32 of data
 * @param data pointer to data
 * @return crc32 value
 */
uint32_t calc_crc32(const uint8_t *data, uint8_t len);

/**
 * @brief update motor command message for all motors
 * @param msg 
 */
void update_upper2lower_msg(Upper2LowerMsg* msg);

#pragma pack(pop) // restore byte alignment

#endif /* _UPDOWN_MSG_H_ */