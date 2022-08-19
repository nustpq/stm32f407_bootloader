#include "gpio.h"
#include "stm32f4xx_rcc.h"
#include"stm32f4xx_gpio.h"


void USART1_GPIO_Config(void)
{
  /* Enable port b clock */
    //RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟

    /* Select alternate function mode */
    GPIOA->MODER &= ~(GPIO_MODER_MODER10 | GPIO_MODER_MODER11);
    GPIOA->MODER |= (GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1);

    /* Select output type push-pull for Tx(Pb10) */
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT_10);

    /* Select output speed medium for Tx(Pb10) */
    GPIOA->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR10);
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10_0;

    /* Select pull up */
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR10 | GPIO_PUPDR_PUPDR11);
    GPIOA->PUPDR |= (GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0);

    //alternative function for portb, pins 10 and 11
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

}


  
//初始化PG6为输出口,LED
void led_init(void)
{   

  GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//使能GPIOG时钟 
  //GPIO G6
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉  
  GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化 

}
