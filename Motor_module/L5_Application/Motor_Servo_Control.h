/*
 * Motor_Servo_Control.h
 *
 *  Created on: Nov 15, 2016
 *      Author: SaurabhDeshmukh
 */

#include "_can_dbc/generated_can.h"
#ifndef L5_APPLICATION_MOTOR_SERVO_CONTROL_H_
#define L5_APPLICATION_MOTOR_SERVO_CONTROL_H_

void initMotorModuleSetup();
void Motor_Servo_Set(MASTER_DRIVING_CAR_t,float);

#endif /* L5_APPLICATION_MOTOR_SERVO_CONTROL_H_ */