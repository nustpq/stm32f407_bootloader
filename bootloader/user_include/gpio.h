#ifndef __GPIO_enum_H_
#define __GPIO_enum_H_
#include <stddef.h>
#include "stm32f4xx.h"
#include "stm32f4xx.h"

#define LED_OFF     GPIO_SetBits(GPIOG,GPIO_Pin_6)
#define LED_ON      GPIO_ResetBits(GPIOG,GPIO_Pin_6)
#define LED_TOGGLE  GPIO_ToggleBits(GPIOG,GPIO_Pin_6)
void USART1_GPIO_Config(void);
void led_init(void);

#endif
