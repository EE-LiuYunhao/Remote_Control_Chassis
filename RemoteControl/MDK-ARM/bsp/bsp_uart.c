/**
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 * @file       bsp_uart.c
 * @brief      this file contains rc data receive and processing function
 * @note       
 * @Version    V1.0.0
 * @Date       Jan-30-2018
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 */
                                                                                                              
#include "string.h"
#include "stdlib.h"
#include "bsp_uart.h"
#include "usart.h"
#include "main.h"
#include "keyboard_def.h"

#define RIGHT_LEFT_POSI  KEY_D
#define RIGHT_LEFT_NEGI  KEY_A

uint8_t   dbus_buf[DBUS_BUFLEN];
rc_info_t rc;
chassis_ctrl chassis_ref;



/**
  * @brief      enable global uart it and do not use DMA transfer done it
  * @param[in]  huart: uart IRQHandler id
  * @param[in]  pData: receive buff 
  * @param[in]  Size:  buff size
  * @retval     set success or fail
  */
static int uart_receive_dma_no_it(UART_HandleTypeDef* huart, uint8_t* pData, uint32_t Size)
{
  uint32_t tmp1 = 0;

  tmp1 = huart->RxState;
	
	if (tmp1 == HAL_UART_STATE_READY)
	{
		if ((pData == NULL) || (Size == 0))
		{
			return HAL_ERROR;
		}

		huart->pRxBuffPtr = pData;
		huart->RxXferSize = Size;
		huart->ErrorCode  = HAL_UART_ERROR_NONE;

		/* Enable the DMA Stream */
		HAL_DMA_Start(huart->hdmarx, (uint32_t)&huart->Instance->DR, (uint32_t)pData, Size);

		/* 
		 * Enable the DMA transfer for the receiver request by setting the DMAR bit
		 * in the UART CR3 register 
		 */
		SET_BIT(huart->Instance->CR3, USART_CR3_DMAR);

		return HAL_OK;
	}
	else
	{
		return HAL_BUSY;
	}
}

/**
  * @brief      returns the number of remaining data units in the current DMAy Streamx transfer.
  * @param[in]  dma_stream: where y can be 1 or 2 to select the DMA and x can be 0
  *             to 7 to select the DMA Stream.
  * @retval     The number of remaining data units in the current DMAy Streamx transfer.
  */
uint16_t dma_current_data_counter(DMA_Stream_TypeDef *dma_stream)
{
  /* Return the number of remaining data units for DMAy Streamx */
  return ((uint16_t)(dma_stream->NDTR));
}



/**
  * @brief       handle received rc data
  * @param[out]  rc:   structure to save handled rc data
  * @param[in]   buff: the buff which saved raw rc data
  * @retval 
  */
void rc_callback_handler(rc_info_t *rc, uint8_t *buff)
{
  rc->ch1 = (buff[0] | buff[1] << 8) & 0x07FF;
  rc->ch1 -= 1024;
  rc->ch2 = (buff[1] >> 3 | buff[2] << 5) & 0x07FF;
  rc->ch2 -= 1024;
  rc->ch3 = (buff[2] >> 6 | buff[3] << 2 | buff[4] << 10) & 0x07FF;
  rc->ch3 -= 1024;
  rc->ch4 = (buff[4] >> 1 | buff[5] << 7) & 0x07FF;
  rc->ch4 -= 1024;
	
	rc->kb_ws = (buff[14] == KEY_W)*1 + (buff[14]== KEY_S)*(-1);
	rc->kb_ad = (buff[14] == RIGHT_LEFT_POSI)*1 + (buff[14] == RIGHT_LEFT_NEGI)*(-1);
	
	rc->kb_sc = 0;
	if(buff[14] == KEY_SHIFT)				rc->kb_sc |= SHIFT; // kb_sc = 0...01 
	if(buff[14] == KEY_CTRL)		    rc->kb_sc |= CTRL;  // kb_sc = 0...10
																											// if shift and control is pressed together, kb_sc = 0...11
	
  rc->sw1 = ((buff[5] >> 4) & 0x000C) >> 2;
  rc->sw2 = (buff[5] >> 4) & 0x0003;
  
  if ((abs(rc->ch1) > 660) || \
      (abs(rc->ch2) > 660) || \
      (abs(rc->ch3) > 660) || \
      (abs(rc->ch4) > 660))
  {
    memset(rc, 0, sizeof(rc_info_t));
  }
	
	rc_dealler(rc);
}

/**
  * @brief      clear idle it flag after uart receive a frame data
  * @param[in]  huart: uart IRQHandler id
  * @retval  
  */
static void uart_rx_idle_callback(UART_HandleTypeDef* huart)
{
	/* clear idle it flag avoid idle interrupt all the time */
	__HAL_UART_CLEAR_IDLEFLAG(huart);

	/* handle received data in idle interrupt */
	if (huart == &DBUS_HUART)
	{
		/* clear DMA transfer complete flag */
		__HAL_DMA_DISABLE(huart->hdmarx);

		/* handle dbus data dbus_buf from DMA */
		if ((DBUS_MAX_LEN - dma_current_data_counter(huart->hdmarx->Instance)) == DBUS_BUFLEN)
		{
			rc_callback_handler(&rc, dbus_buf);	
		}
		
		/* restart dma transmission */
		__HAL_DMA_SET_COUNTER(huart->hdmarx, DBUS_MAX_LEN);
		__HAL_DMA_ENABLE(huart->hdmarx);
	}
}

/**
  * @brief      callback this function when uart interrupt 
  * @param[in]  huart: uart IRQHandler id
  * @retval  
  */
void uart_receive_handler(UART_HandleTypeDef *huart)
{  
	if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) && 
			__HAL_UART_GET_IT_SOURCE(huart, UART_IT_IDLE))
	{
		uart_rx_idle_callback(huart);
	}
}

/**
  * @brief   initialize dbus uart device 
  * @param   
  * @retval  
  */
void dbus_uart_init(void)
{
	/* open uart idle it */
	__HAL_UART_CLEAR_IDLEFLAG(&DBUS_HUART);
	__HAL_UART_ENABLE_IT(&DBUS_HUART, UART_IT_IDLE);

	uart_receive_dma_no_it(&DBUS_HUART, dbus_buf, DBUS_MAX_LEN);
}


/**
	* @brief  calcu speed ref basing on rc data
	* @param	remote: rc data to be passed in
	* @retval
*/
void rc_dealler(const rc_info_t * remote)
{
	//ch2 for forward_back speed ref
	//ch1 for left_right speed ref
	//ch3 for rotation_ref
	//rea means true value
	//abs means absolute value
	int16_t ch1_abs;
	int16_t ch2_abs;
	int16_t ch3_abs;
	
	//keyboard is not active
	//keyboard has a higher priority over remote controller
	if(remote->kb_ad == 0 && remote->kb_ws == 0)
	{
		ch1_abs = remote->ch1<0 ? -(remote->ch1):remote->ch1;
	  ch2_abs = remote->ch2<0 ? -(remote->ch2):remote->ch2;
		ch3_abs = remote->ch3<0 ? -(remote->ch3):remote->ch3;
		chassis_ref.forward_back_speed_ref = remote->ch2>=10? remote->ch2 * ch2_abs * SPEED_CONST    : 0;
		chassis_ref.left_right_speed_ref   = remote->ch1>=10? remote->ch1 * ch1_abs * SPEED_CONST    : 0;
		chassis_ref.rotation_speed_ref     = remote->ch3>=10? remote->ch3 * ch3_abs * ROTATION_CONST : 0;
	}
	//keyboard is active
	else
	{
		/*
		to be added later
		* two value mode: fast and normal
		* control by shift
		*/
		chassis_ref.forward_back_speed_ref = remote->kb_ws*((remote->kb_sc == (uint8_t)SHIFT)? FAST_SPEED : NORMAL_SPEED);
		chassis_ref.left_right_speed_ref   = remote->kb_ad*((remote->kb_sc == (uint8_t)SHIFT)? FAST_SPEED : NORMAL_SPEED);
	}
}
