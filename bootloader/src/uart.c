#include <stddef.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "uart.h"
#include "stm32f4xx_gpio.h"
#include "gpio.h"

#pragma import(__use_no_semihosting)

void _sys_exit(int x)
{
	x = x;
}

struct __FILE
{
	int handle;
	/* Whatever you require here. If the only file you are using is */
	/* standard output using printf() for debugging, no file handling */
	/* is required. */
};

FILE __stdout;	/* FILE is typedef’ d in stdio.h. */

static int index=0;
uint8_t RxBuffer[MAX_BUFFER_LENGTH + 1];
static char RxChar = 0;

USART_CMD_STATUS_TYPE current_cmd_Status = USART_CMD_NONE;//cmd receiving status


void USART1_SendChar(char data)
{
    // Write data into transmit data register
    USART1->DR = data;
    //to ensure that byte has sent successfully
    while(!(USART1->SR & USART_SR_TXE))
    {
        //Wait for transmission buffer empty flag
    }

}

int fputc(int ch, FILE *f)
{
    USART1_SendChar((char) ch); 
    return (int)ch;
}

void USART1_IRQ_Callback(void)
{
    if((USART1->SR & USART_SR_IDLE) == USART_SR_IDLE)
    {   /* Read data register to clear idle line flag "cmd receiving has finished "*/
        (void)USART1->DR;
        //end of rx buffer
        RxBuffer[index]=0;
        //index=USART1->DR;
        index=0;//clean to receive new cmd
        // set led and Set CMD status   
        LED_OFF;
        current_cmd_Status = USART_CMD_RECEIVED;
        return;
    }

    /* Check USART receiver */
    if((USART1->SR & USART_SR_RXNE) == USART_SR_RXNE)
	{
        // Read character
        RxChar = USART1->DR;
        //FILL RECIVER BUFFER
        RxBuffer[index++]=RxChar;

    } else {
        // No new data received
    }

}

void USART1_Init(uint32_t bound)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    //NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟

    //串口1对应引脚复用映射
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9复用为USART1
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10复用为USART1

    //USART1端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
    GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA9，PA10

    //USART1 初始化设置
    USART_InitStructure.USART_BaudRate = bound;//波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(USART1, &USART_InitStructure); //初始化串口1
         
    //USART_Cmd(USART1, ENABLE);  //使能串口1 
    //USART_ClearFlag(USART1, USART_FLAG_TC);
	
 	
    // USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//开启相关中断

    // //Usart1 NVIC 配置
    // NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//串口1中断通道
    // NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
    // NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
    // NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
    // NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、
	
}


void USART1_Enable(void)
{

    /* Enable USART1 */
    USART1->CR1 |= USART_CR1_UE;

    /* Enable transmitter */
    USART1->CR1 |= USART_CR1_TE;

    /* Enable receiver */
    USART1->CR1 |= USART_CR1_RE;

    /* Enable idle line detection interrupt */
    USART1->CR1 |= USART_CR1_IDLEIE;

    /* Enable reception buffer not empty flag interrupt */
    USART1->CR1 |= USART_CR1_RXNEIE;



}
void NVIC_Int(void)
{
    /* Set priority group to 3
    * bits[3:0] are the sub-priority,
    * bits[7:4] are the pre-empt priority (0-15) */
    NVIC_SetPriorityGrouping(3);
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);

}

