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
#define BOARD_USE_SERIAL	1
#define BOARD_USE_SD		1
#define BOARD_USE_MOTOR		1
#define BOARD_USE_ENCODER	1
#define BOARD_USE_LED		1
#define BOARD_USE_BUTTON	1
#define BOARD_USE_ACCEL		1


#define BOARD_VERSION 1.5

#if defined(BOARD_USE_MOTOR) && BOARD_USE_MOTOR

// Motor connections, common
#define MOTOR_SCK		PA_5
#define MOTOR_MOSI		PA_7
#define MOTOR_MISO		PA_6

// Motor1 (RA)
#define MOTOR1_CS		PB_1
#define MOTOR1_STEP		PB_9		// TIM4
#define MOTOR1_DIR		NC
#define MOTOR1_IREF		PB_7_ALT0	// TIM17
#define MOTOR1_DIAG		PD_2

#define MOTOR2_CS		PA_10
#define MOTOR2_STEP		PB_6_ALT0	// TIM16
#define MOTOR2_DIR		NC
#define MOTOR2_IREF		PB_0_ALT0	// TIM3
#define MOTOR2_DIAG		PA_15

#endif

// LCD related definitions
#if defined(BOARD_USE_LCD) && BOARD_USE_LCD
#define LCD_USE_4W	1

// LCD1602 connections
#define LCD_RS	PB_2
#define LCD_RW	PC_8
#define LCD_EN	PC_6
#define LCD_D4	PC_9
#define LCD_D5	PB_8
#define LCD_D6	PC_5
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

#define LED1	PB_15_ALT1				// TIM15
#define LED2	PA_8					// TIM1
#define LED3	PC_7_ALT0				// TIM8

#endif

// BUTTON related definitions
#if defined(BOARD_USE_BUTTON) && BOARD_USE_BUTTON

#define BUTTON_NUM	3
#define BUTTON1		PA_0
#define BUTTON2		PC_13
#define BUTTON3		PC_12
static const PinName Button_All[] = {BUTTON1, BUTTON2, BUTTON3};

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

// SD definition

#if defined(BOARD_USE_SD) && BOARD_USE_SD
#define SD_CS				PA_1
#define SD_SCK				PB_3_ALT0
#define SD_MISO				PB_4_ALT0
#define SD_MOSI				PB_5_ALT0
#endif

// Accelerometer definition
#if defined(BOARD_USE_ACCEL) && BOARD_USE_ACCEL
#define ACCEL_SDA			PB_14
#define ACCEL_SCL			PB_13
#endif

#endif /* BOARD_H_ */
