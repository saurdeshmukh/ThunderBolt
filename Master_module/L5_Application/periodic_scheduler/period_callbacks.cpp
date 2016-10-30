/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

/**
 * @file
 * This contains the period callback functions for the periodic scheduler
 *
 * @warning
 * These callbacks should be used for hard real-time system, and the priority of these
 * tasks are above everything else in the system (above the PRIORITY_CRITICAL).
 * The period functions SHOULD NEVER block and SHOULD NEVER run over their time slot.
 * For example, the 1000Hz take slot runs periodically every 1ms, and whatever you
 * do must be completed within 1ms.  Running over the time slot will reset the system.
 */

#include <stdint.h>
#include "io.hpp"
#include "periodic_callback.h"
#include "can.h"
#include "_can_dbc/generated_can.h"

#define LEFT_MIN 		20
#define RIGHT_MIN 		20
#define FRONT_MIN 		20
#define LEFT_MIDDLE 	50
#define RIGHT_MIDDLE 	50
#define FRONT_MIDDLE 	50
#define LEFT_MAX 		200
#define RIGHT_MAX 		200
#define FRONT_MAX 		200

can_msg_t rx_msg = {0};
can_msg_t tx_msg = {0};

const uint32_t                             COM_BRIDGE_HEARTBEAT__MIA_MS = 3000;
const COM_BRIDGE_HEARTBEAT_t               COM_BRIDGE_HEARTBEAT__MIA_MSG = {0};
const uint32_t                             GPS_HEARTBEAT__MIA_MS = 3000;
const GPS_HEARTBEAT_t                      GPS_HEARTBEAT__MIA_MSG = {0};
const uint32_t                             MOTOR_HEARTBEAT__MIA_MS = 3000;
const MOTOR_HEARTBEAT_t                    MOTOR_HEARTBEAT__MIA_MSG = {0};
const uint32_t                             SENSOR_HEARTBEAT__MIA_MS = 3000;
const SENSOR_HEARTBEAT_t                   SENSOR_HEARTBEAT__MIA_MSG = {0};
const uint32_t                             COM_BRIDGE_CLICKED_START__MIA_MS = 3000;
const COM_BRIDGE_CLICKED_START_t           COM_BRIDGE_CLICKED_START__MIA_MSG = {0};
const uint32_t                             COM_BRIDGE_STOPALL__MIA_MS = 3000;
const COM_BRIDGE_STOPALL_t                 COM_BRIDGE_STOPALL__MIA_MSG = {0};
const uint32_t                             SENSOR_SONARS__MIA_MS = 3000;
const SENSOR_SONARS_t                      SENSOR_SONARS__MIA_MSG = {8,8,8,8};
const uint32_t                             GPS_MASTER_DATA__MIA_MS = {3000};
const GPS_MASTER_DATA_t                    GPS_MASTER_DATA__MIA_MSG = {0};



COM_BRIDGE_HEARTBEAT_t com_bridge_heartbeat_status = {0};
GPS_HEARTBEAT_t gps_heartbeat_status = {0};
MOTOR_HEARTBEAT_t motor_heartbeat_status = {0};
SENSOR_HEARTBEAT_t sensor_heartbeat_status = {0};
COM_BRIDGE_CLICKED_START_t com_bridge_start = {0};
COM_BRIDGE_STOPALL_t com_bridge_stop = {0};
SENSOR_SONARS_t sensor_data = {0};
GPS_MASTER_DATA_t gps_data = {0};

MASTER_DRIVING_CAR_t motor_drive = {STOP, CENTER, LOW};


/// This is the stack size used for each of the period tasks (1Hz, 10Hz, 100Hz, and 1000Hz)
const uint32_t PERIOD_TASKS_STACK_SIZE_BYTES = (512 * 4);

/**
 * This is the stack size of the dispatcher task that triggers the period tasks to run.
 * Minimum 1500 bytes are needed in order to write a debug file if the period tasks overrun.
 * This stack size is also used while calling the period_init() and period_reg_tlm(), and if you use
 * printf inside these functions, you need about 1500 bytes minimum
 */
const uint32_t PERIOD_DISPATCHER_TASK_STACK_SIZE_BYTES = (512 * 3);

/// Called once before the RTOS is started, this is a good place to initialize things once
bool period_init(void)
{
	CAN_init(can1, 100, 50, 50, NULL, NULL);
	CAN_bypass_filter_accept_all_msgs();
	CAN_reset_bus(can1);
    return true; // Must return true upon success
}

/// Register any telemetry variables
bool period_reg_tlm(void)
{
    // Make sure "SYS_CFG_ENABLE_TLM" is enabled at sys_config.h to use Telemetry
    return true; // Must return true upon success
}


/**
 * Below are your periodic functions.
 * The argument 'count' is the number of times each periodic task is called.
 */

void period_1Hz(uint32_t count)
{
	// BUS RESET
	if(CAN_is_bus_off(can1))
		CAN_reset_bus(can1);

	// HEARTBEAT STATUS ON LEDS
	if(com_bridge_heartbeat_status.COM_BRIDGE_HEARTBEAT_UNSIGNED == COM_BRIDGE_HEARTBEAT_HDR.mid)
		LE.on(1);
	else
		LE.off(1);

	if(gps_heartbeat_status.GPS_HEARTBEAT_UNSIGNED == GPS_HEARTBEAT_HDR.mid)
		LE.on(2);
	else
		LE.off(2);

	if(motor_heartbeat_status.MOTOR_HEARTBEAT_UNSIGNED == MOTOR_HEARTBEAT_HDR.mid)
		LE.on(3);
	else
		LE.off(3);

	if(sensor_heartbeat_status.SENSOR_HEARTBEAT_UNSIGNED == SENSOR_HEARTBEAT_HDR.mid)
		LE.on(4);
	else
		LE.off(4);
}

void process_data()
{
	if(com_bridge_stop.COM_BRIDGE_STOPALL_UNSIGNED == COM_BRIDGE_STOPALL_HDR.mid)
	{
		com_bridge_start.COM_BRIDGE_CLICKED_START_UNSIGNED = 0;
		motor_drive = {STOP, CENTER, LOW};
	}
	else
	{
		if(com_bridge_start.COM_BRIDGE_CLICKED_START_UNSIGNED == COM_BRIDGE_CLICKED_START_HDR.mid)
		{
			com_bridge_stop.COM_BRIDGE_STOPALL_UNSIGNED = 0;

			if((sensor_data.SENSOR_SONARS_LEFT_UNSIGNED > LEFT_MIN) &&
			   (sensor_data.SENSOR_SONARS_RIGHT_UNSIGNED > RIGHT_MIN) &&
				(sensor_data.SENSOR_SONARS_FRONT_UNSIGNED > FRONT_MIDDLE))
			{
				//MOVE_FORWARD
				motor_drive.MASTER_DRIVE_ENUM = DRIVE;
				motor_drive.MASTER_SPEED_ENUM =  LOW;
				motor_drive.MASTER_STEER_ENUM = CENTER;
			}
			else if((sensor_data.SENSOR_SONARS_LEFT_UNSIGNED > LEFT_MIN) &&
			   (sensor_data.SENSOR_SONARS_RIGHT_UNSIGNED < RIGHT_MIN) &&
				(sensor_data.SENSOR_SONARS_FRONT_UNSIGNED > FRONT_MIDDLE))
			{
				//MOVE_LEFT
				motor_drive.MASTER_DRIVE_ENUM = DRIVE;
				motor_drive.MASTER_SPEED_ENUM =  LOW;
				motor_drive.MASTER_STEER_ENUM = LEFT;
			}
			else if((sensor_data.SENSOR_SONARS_LEFT_UNSIGNED < LEFT_MIN) &&
			   (sensor_data.SENSOR_SONARS_RIGHT_UNSIGNED > RIGHT_MIN) &&
				(sensor_data.SENSOR_SONARS_FRONT_UNSIGNED > FRONT_MIDDLE))
			{
				//MOVE_RIGHT
				motor_drive.MASTER_DRIVE_ENUM = DRIVE;
				motor_drive.MASTER_SPEED_ENUM =  LOW;
				motor_drive.MASTER_STEER_ENUM = RIGHT;
			}
			else if(sensor_data.SENSOR_SONARS_FRONT_UNSIGNED < FRONT_MIDDLE)
			{
				if(sensor_data.SENSOR_SONARS_RIGHT_UNSIGNED > RIGHT_MIN)
				{
					//MOVE_RIGHT
					motor_drive.MASTER_DRIVE_ENUM = DRIVE;
					motor_drive.MASTER_SPEED_ENUM =  LOW;
					motor_drive.MASTER_STEER_ENUM = RIGHT;
				}
				else if(sensor_data.SENSOR_SONARS_LEFT_UNSIGNED > LEFT_MIN)
				{
					//MOVE_LEFT
					motor_drive.MASTER_DRIVE_ENUM = DRIVE;
					motor_drive.MASTER_SPEED_ENUM =  LOW;
					motor_drive.MASTER_STEER_ENUM = RIGHT;
				}
				else
				{
					//STOP
					motor_drive.MASTER_DRIVE_ENUM = STOP;
					motor_drive.MASTER_SPEED_ENUM =  LOW;
					motor_drive.MASTER_STEER_ENUM = CENTER;
				}
			}
		}
		else
		{
			//STOP
			motor_drive.MASTER_DRIVE_ENUM = STOP;
			motor_drive.MASTER_SPEED_ENUM =  LOW;
			motor_drive.MASTER_STEER_ENUM = CENTER;
		}
	}
}

void period_10Hz(uint32_t count)
{
	dbc_msg_hdr_t msg_header;
	if(CAN_rx(can1, &rx_msg, 0))
	{
		msg_header.mid = rx_msg.msg_id;
		msg_header.dlc = rx_msg.frame_fields.data_len;

		if(msg_header.mid == COM_BRIDGE_HEARTBEAT_HDR.mid)
			dbc_decode_COM_BRIDGE_HEARTBEAT(&com_bridge_heartbeat_status, rx_msg.data.bytes, &msg_header);
		if(msg_header.mid == GPS_HEARTBEAT_HDR.mid)
			dbc_decode_GPS_HEARTBEAT(&gps_heartbeat_status, rx_msg.data.bytes, &msg_header);
		if(msg_header.mid == MOTOR_HEARTBEAT_HDR.mid)
			dbc_decode_MOTOR_HEARTBEAT(&motor_heartbeat_status, rx_msg.data.bytes, &msg_header);
		if(msg_header.mid == SENSOR_HEARTBEAT_HDR.mid)
			dbc_decode_SENSOR_HEARTBEAT(&sensor_heartbeat_status, rx_msg.data.bytes, &msg_header);
		if(msg_header.mid == COM_BRIDGE_CLICKED_START_HDR.mid)
			dbc_decode_COM_BRIDGE_CLICKED_START(&com_bridge_start, rx_msg.data.bytes, &msg_header);
		if(msg_header.mid == COM_BRIDGE_STOPALL_HDR.mid)
			dbc_decode_COM_BRIDGE_STOPALL(&com_bridge_stop, rx_msg.data.bytes, &msg_header);
		if(msg_header.mid == SENSOR_SONARS_HDR.mid)
			dbc_decode_SENSOR_SONARS(&sensor_data, rx_msg.data.bytes, &msg_header);
		if(msg_header.mid == GPS_MASTER_DATA_HDR.mid)
			dbc_decode_GPS_MASTER_DATA(&gps_data, rx_msg.data.bytes, &msg_header);

	}
	dbc_handle_mia_COM_BRIDGE_HEARTBEAT(&com_bridge_heartbeat_status, 100);
	dbc_handle_mia_GPS_HEARTBEAT(&gps_heartbeat_status, 100);
	dbc_handle_mia_MOTOR_HEARTBEAT(&motor_heartbeat_status, 100);
	dbc_handle_mia_SENSOR_HEARTBEAT(&sensor_heartbeat_status, 100);

	// Incrementing time by 0 so that mia will always be in disable state (For time being. Need to give more thought on this)
	dbc_handle_mia_COM_BRIDGE_CLICKED_START(&com_bridge_start, 0);
	dbc_handle_mia_COM_BRIDGE_STOPALL(&com_bridge_stop, 0);

	dbc_handle_mia_SENSOR_SONARS(&sensor_data, 100);
	dbc_handle_mia_GPS_MASTER_DATA(&gps_data, 100);

	process_data();

	msg_header = dbc_encode_MASTER_DRIVING_CAR(tx_msg.data.bytes, &motor_drive);
	tx_msg.msg_id = msg_header.mid;
	tx_msg.frame_fields.data_len = msg_header.dlc;
	CAN_tx(can1, &tx_msg, 0);
}

void period_100Hz(uint32_t count)
{
}

// 1Khz (1ms) is only run if Periodic Dispatcher was configured to run it at main():
// scheduler_add_task(new periodicSchedulerTask(run_1Khz = true));
void period_1000Hz(uint32_t count)
{
    LE.toggle(4);
}
