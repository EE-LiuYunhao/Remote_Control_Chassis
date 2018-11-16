/**
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 * @file       rc.h
 * @brief      this file contains the common defines and functions prototypes for 
 *             the rc.c driver
 * @note         
 * @Version    V1.0.0
 * @Date       Nov-15-2018      
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 */

#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#include "usart.h"
#include "keyboard_def.h"
#include <math.h>

#define UART_RX_DMA_SIZE (1024)
#define DBUS_MAX_LEN     (50)
#define DBUS_BUFLEN      (18)
#define DBUS_HUART       huart1 /* for dji remote controler reciever */
#define SPEED_CONST			 0.00068 //the const for rc_dealer to calcu the speed ref
#define ROTATION_CONST 	 0.00045 //the const for rc_dealer to calcu the rotate ref
#define MOUSE_CONST			 100 //the const for calcu the gimbal angel ref when mouse is used
#define REMOTE_ANGLE_CONST					50//the const for calcu the gimbal angel ref when remote controller is used
#define GIMBAL_HORIZONTAL_CONST	0.0005//the const to calcu the gimbal angle ref
#define GIMBAL_VERTICAL_CONST		0.0005//the const to calcu the gimbal angle ref

/** 
  * @brief  remote control information
  */

typedef __packed struct
{
	int16_t forward_back_direction;
	int16_t left_right_direction;
	int16_t rotation_direction;
	int16_t speed_mood;
}KbCtrl;

#define NO_KEY_PRESSED(Control)			(Control.forward_back_direction==0 && Control.left_right_direction==0 && Control.rotation_direction==0)		
#define COMPLEMENT(x)   						abs(x)==180?180:(360-abs(x))*(-1)*(x/abs(x))
#define ANGLE_DEALER(pangle)				abs(pangle)>=180?COMPLEMENT(pangle):pangle	


typedef __packed struct
{
	uint8_t state;
  /* rocker channel information */
  int16_t ch1;
  int16_t ch2;
  int16_t ch3;
  int16_t ch4;
	
	/*keyboard-mouse information*/
	int16_t ch5;//mouse-y
	int16_t ch6;//mouse-x
	int16_t ch7;//mouse-z
	uint8_t ch8;//mouse-l
	uint8_t ch9;//mouse-r
	KbCtrl kb_ctrl;//buffer channel 14, for chassis direction control
	uint8_t kb_othe;//buffer channel 15, for other functions

  /* left and right lever information */
  uint8_t sw1;
  uint8_t sw2;
} rc_info_t;

typedef __packed struct
{
	int16_t forward_back_speed_ref;
	int16_t left_right_speed_ref;
	int16_t rotation_speed_ref;
} chassis_ctrl;

typedef __packed struct
{
	int16_t horizontal_angle_ref;
	int16_t vertical_angle_ref;
} gimbal_ctrl;

void uart_receive_handler(UART_HandleTypeDef *huart);
void dbus_uart_init(void);
void USART_SendData(USART_TypeDef* USARTx, uint16_t Data);
void rc_dealler(const rc_info_t *);
int16_t gim_calcu(int16_t,int16_t,uint8_t,double);
void gimbal_dealer(rc_info_t *);
#endif

