#include "sys.h"
#include "usart.h"	
#include "kfifo.h"
#include "string.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif

#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (u8) ch;      
	return ch;
}
#endif
 

////////////////////////////////////////////////////////////////////////////////
#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
// u8 RxBuffer2[UART2_BUF_LENGTH];
// u8 TxBuffer2[UART2_BUF_LENGTH];
u8 RxBuffer1[UART1_BUF_LENGTH];
u8 TxBuffer1[UART1_BUF_LENGTH];

u8 cmd_buffer[UART1_BUF_LENGTH];
u8 RxBuffer1_FIFO[UART1_FIFO_LENGTH];
kfifo_t uart_fifo;


void UART1_DMA_Send(u8 *Data, u16 NByte)
{
    //while( UART1_DAM_Done() != 0 ); 
    //DMA_Cmd(DMA2_Stream7,DISABLE);
    //TIM3_Stop();
	memcpy(TxBuffer1,Data,NByte);
	DMA_SetCurrDataCounter(DMA2_Stream7,NByte);
	DMA_Cmd(DMA2_Stream7,ENABLE);

}

void UART1_DMA_Send_Disable( void)
{
    DMA_Cmd(DMA2_Stream7,DISABLE);
    //TIM3_Stop();
}

unsigned char UART1_DAM_Done( void )
{
    return DMA_GetCurrDataCounter(DMA2_Stream7);
}

void UART1_Write(u8 *Data, u16 NByte)
{
	if(NByte)
	{
		unsigned int i;
		for(i=0;i<NByte;i++)
		{
			while((USART1->SR&0X40)==0); 
			USART1->DR = Data[i]; 
		}			
	}		
}

void USART1_SendChar(char data)
{
    // // Write data into transmit data register
    // USART1->DR = data;
    // //to ensure that byte has sent successfully
    // while(!(USART1->SR & USART_SR_TXE));  //Wait for transmission buffer empty
    UART1_DMA_Send(&data,1);

}
/////////////////////////////////////////////////////////////////////////

void BSP_DMAUsar1Rx_Init(void)
{
    DMA_InitTypeDef   DMA_InitStructure;	
            
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);//uart1
    DMA_DeInit(DMA2_Stream2);
	
    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               /* ??DMA?? */
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   /* ? */
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)RxBuffer1;             /* ?? */
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralToMemory;    /* ?? */
    DMA_InitStructure.DMA_BufferSize          = UART1_BUF_LENGTH;                    /* ?? */                  
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    /* ???????? */
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         /* ???????? */
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      /* ?????? */
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      /* ????? */
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Circular;              /* ??????/?????? */
    DMA_InitStructure.DMA_Priority            = DMA_Priority_VeryHigh;        /* DMA??? */
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          /* FIFO??/???? */
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; /* FIFO?? */
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       /* ???? */
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;

    DMA_Init(DMA2_Stream2, &DMA_InitStructure);
}


void BSP_DMAUsar1Tx_Init(void)
{
    DMA_InitTypeDef   DMA_InitStructure;	
    NVIC_InitTypeDef NVIC_InitStructure;
            
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);//uart1
    DMA_DeInit(DMA2_Stream7);
	
    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               /* ??DMA?? */
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   /* ? */
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)TxBuffer1;
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_MemoryToPeripheral;    /* ?? */
    DMA_InitStructure.DMA_BufferSize          = UART1_BUF_LENGTH;                    /* ?? */                  
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    /* ???????? */
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         /* ???????? */
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      /* ?????? */
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      /* ????? */
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;              /* ??????/?????? */
    DMA_InitStructure.DMA_Priority            = DMA_Priority_VeryHigh;        /* DMA??? */
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          /* FIFO??/???? */
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; /* FIFO?? */
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       /* ???? */
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;

    DMA_Init(DMA2_Stream7, &DMA_InitStructure);
  	DMA_ITConfig(DMA2_Stream7,DMA_IT_TC,ENABLE);//开启传输完成中断
  
		
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn; 
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =0x00;//抢占优先级0
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;//子优先级1
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure);//配置
}



//USART1 Tx  DMA IRQ
void DMA2_Stream7_IRQHandler(void)
{      
	if(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7)==SET)	//DMA1_Stream3,传输完成标志
	{ 
		DMA_ClearITPendingBit(DMA2_Stream7,DMA_IT_TCIF7);	//清除传输完成中断
        DMA_Cmd(DMA2_Stream7,DISABLE);
        //TIM3_Start();
	}

} 


//USART1 Rx  DMA IRQ
void USART1_IRQHandler(void)                	//uart1
{
	//static uint16_t RxPos=0;
	uint16_t num=0;
#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
        num = UART1_BUF_LENGTH - DMA_GetCurrDataCounter(DMA2_Stream2); 
        DMA_Cmd(DMA2_Stream2,DISABLE);
		USART1->SR;USART1->DR;
        uint16_t counter = kfifo_get_free_space(&uart_fifo);
        if (counter < num)
        {
            kfifo_release(&uart_fifo, num - counter); //discard old data       
        }
        kfifo_put(&uart_fifo, RxBuffer1, num);
        DMA_MemoryTargetConfig(DMA2_Stream2,(uint32_t)RxBuffer1, DMA_Memory_0);
		//BSP_DMAUsar1Rx_Init();
        DMA_Cmd(DMA2_Stream2,ENABLE);        
         
	}
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
} 



//初始化IO 串口1 
//bound:波特率
void uart1_init(u32 bound)
{
   //GPIO端口设置
  	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	

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

  /* Configure RX DMA */
  	BSP_DMAUsar1Rx_Init();
  	BSP_DMAUsar1Tx_Init();

  /* Enable USART DMA RX Requsts */
  	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
  	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA2_Stream2,ENABLE);
	DMA_Cmd(DMA2_Stream7,DISABLE);
         
  	//USART_Cmd(USART1, ENABLE);  //使能串口1 
	//USART_ClearFlag(USART1, USART_FLAG_TC);
	
#if EN_USART1_RX	
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
  	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、
#endif
    kfifo_init_static(&uart_fifo, &RxBuffer1_FIFO[0], UART1_FIFO_LENGTH);
    
	
}

void uart1_enable(void)
{
    USART_Cmd(USART1, ENABLE);  //使能串口1 
}


#endif	

