#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include"uart.h"
#include "stm32f4xx.h"
#include"stm32f4xx_flash.h"
#include "boot_cfg_port.h"
#include "Sys_init.h"

//IMAGE_SADDR is an image's start address to be checked before jump to app which defined in boot_cfg.h
#define  APP_START_ADDRESS  *((volatile uint32_t*) (IMAGE_SADDR ))

 //32bit buffer indicates "upgrade" request when application runs
//#define  COMMON_BUF   __attribute__ ((section (".no_init_ram")))  volatile uint32_t
#define  COMMON_BUF   __attribute__ ((at(0x2001FC00))) volatile uint32_t

//commands used by image supplier and bootloader during communication

#define  ACK          0x55
#define  NACK         0xAA
#define  CMD_GETID    0x90
#define  CMD_ERASE    0xA1
#define  CMD_WRITE    0xB2
#define  CMD_UPDATE   0xC3
#define  CMD_JUMP     0xD4
#define  CMD_WP_ON    0xE5
#define  CMD_WP_OFF   0xF6
       
#define  KEY_UPDATE       0xA0A0B0B0       
#define  UART1_BAUD       115200

void boot_process(void);
void cmdErase(uint8_t*pucData);
void cmdWrite(uint8_t *pucData);
void cmdjump(uint8_t*p);
void write_prot(uint8_t*p);
void Remove_wr_prot(uint8_t*p);
void update(uint8_t*p);

#endif
