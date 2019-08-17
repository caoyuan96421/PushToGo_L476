/*
 * Board.h
 *
 *  Created on: Jul 18, 2019
 *      Author: caoyu
 */

#ifndef BOARD_H_
#define BOARD_H_

#include "PinNames.h"

// Usage of board features
#define BOARD_USE_LCD		1
#define BOARD_USE_USB		0
#define BOARD_USE_SERIAL	1
#define BOARD_USE_MOTOR		1
#define BOARD_USE_ENCODER	1
#define BOARD_USE_LED		1
#define BOARD_USE_BUTTON	1


#define BOARD_VERSION 1.0

#if defined(BOARD_USE_MOTOR) && BOARD_USE_MOTOR

// Motor connections, common
#define MOTOR_SCK		PA_5
#define MOTOR_MOSI		PA_7
#define MOTOR_MISO		PA_6

// Motor1 (RA)
#define MOTOR1_CS		PB_6
#define MOTOR1_STEP		PB_2
#define MOTOR1_DIR		PC_7
#define MOTOR1_IREF		PB_7
#define MOTOR1_DIAG		PD_2

#define MOTOR2_CS		PA_8
#define MOTOR2_STEP		PB_0
#define MOTOR2_DIR		NC
#define MOTOR2_IREF		PB_6
#define MOTOR2_DIAG		PA_15

#endif

// LCD related definitions
#if defined(BOARD_USE_LCD) && BOARD_USE_LCD
#define LCD_USE_4W	1

// LCD1602 connections
#define LCD_RS	PC_8
#define LCD_RW	PC_6
#define LCD_EN	PC_9
#define LCD_D4	PB_8
#define LCD_D5	PC_5
#define LCD_D6	PB_9
#define LCD_D7	PB_12

// Only define D0-3 if not using 4W mode
#if defined(LCD_USE_4W) && !(LCD_USE_4W)
#define LCD_D0	PB_0
#define LCD_D1	PB_1
#define LCD_D2	PB_2
#define LCD_D3	PB_3
#endif

// Brightness control (VEE) PWM output pin
#define LCD_BRIGHTNESS	PB_11
#endif

// USB related definitions
#if defined(BOARD_USE_USB) && BOARD_USE_USB
// TODO
#endif

// LED related definitions
#if defined(BOARD_USE_LED) && BOARD_USE_LED

#define LED1	PB_13
#define LED2	PB_14
#define LED3	PB_15

#endif

// BUTTON related definitions
#if defined(BOARD_USE_BUTTON) && BOARD_USE_BUTTON

#define BUTTON1	PA_0
#define BUTTON2	PC_13
#define BUTTON3	PC_12

#endif

// Serial definition
#if defined(BOARD_USE_SERIAL) && BOARD_USE_SERIAL
#define HC_TX	PC_10
#define HC_RX	PC_11
#endif

// Encoder definition
#if defined(BOARD_USE_ENCODER) && BOARD_USE_ENCODER
#define ENCODER_MOSI		PC_3
#define ENCODER_MISO		PC_2
#define ENCODER_SCK			PB_10
#define ENCODER1_CS			PC_1
#define ENCODER2_CS			PC_4
#endif

#endif /* BOARD_H_ */
