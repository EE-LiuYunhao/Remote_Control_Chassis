/*
 * @brief      this file contains the macro for values of keys
 * @note         
 * @Version    V1.0.0
 * @Date       NOV-11-2018      
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 */
 
 
#define FAST_SPEED			 260 // speed ref for keyboard control
#define NORMAL_SPEED		 100 // speed ref for keyboard control
#define KEY_W						 0x01// buffer[14]
#define KEY_S						 0x02// buffer[14]
#define KEY_A						 0x04// buffer[14]
#define KEY_D						 0x08// buffer[14]
#define KEY_SHIFT				 0x10// buffer[14]
#define KEY_CTRL				 0x20// buffer[14]
#define SHIFT            1   // for rc->kb_sc, when shift is pressed
#define CTRL						 2   // for rc->kb_sc, when ctrl is pressed
