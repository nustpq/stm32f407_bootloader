#ifndef __uart_enum_H_
#define __uart_enum_H_
#include <stdint.h>

typedef enum
{
  USART_CMD_NONE,
  USART_CMD_RECEIVED,

} USART_CMD_STATUS_TYPE;

#define MAX_BUFFER_LENGTH            ((uint32_t) (256+16))
void USART1_Init(uint32_t baud);
void USART1_Enable(void);
void NVIC_Int(void);
void USART1_IRQ_Callback(void);
//void USART1_Process(void);
void USART1_SendChar(const char data);
#endif
