#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "stm32f4xx_dma.h"
#include "kfifo.h"

#define DATA_PACK_SIZE            256 //max raw data size
#define UART1_BUF_LENGTH         (5+DATA_PACK_SIZE+4+2) //cmd + addr + data + crc + dummy
#define UART1_FIFO_LENGTH         4096 //2^N


#define EN_USART1_RX 			1

extern u8 cmd_buffer[UART1_BUF_LENGTH];
extern kfifo_t uart_fifo;

void uart1_init(u32 bound);
//void uart2_init(u32 bound);
void UART1_Write(u8 *Data, u16 NByte);
void UART1_DMA_Send(u8 *Data, u16 NByte);
//void UART2_DMA_Send(u8 *Data, u16 NByte);
void uart1_enable(void);
void UART1_DMA_Send_Disable( void);
unsigned char UART1_DAM_Done( void );

#endif


