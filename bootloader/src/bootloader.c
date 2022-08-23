#include <string.h>
#include "bootloader.h"
#include "stm32f4xx_gpio.h"
#include "gpio.h"
#include "uart.h"
#include "delay.h"


unsigned char fw_version[]  = "[FW:V0.1]"; //fixed size string 
 
extern USART_CMD_STATUS_TYPE current_cmd_Status;//CMD received status which is defined in uart.c
extern uint8_t RxBuffer[MAX_BUFFER_LENGTH + 1];//cmd buffer which is defined in uart.c

void boot_process()
{  
    NVIC_Int();
		delay_init(168); 
		led_init();		
    USART1_Init(UART1_BAUD);    
    USART1_Enable();
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;//enable crc  
		
		delay_ms(500);	
    printf("Enter bootloader mode ...\r\n");
    printf("--  Bootloader version: %s\r\n", fw_version);  
    printf("--  Compiled on %s %s by PQ\r\n", __DATE__, __TIME__);
	
    while(1){
        switch(current_cmd_Status)
        {
            case USART_CMD_RECEIVED:
                switch(RxBuffer[0]){
                    case 's': // start session
                        current_cmd_Status = USART_CMD_NONE;
                    break;

                    case CMD_ERASE:                        
                        cmdErase(RxBuffer);                        
                    break;

                    case CMD_WRITE:                        
                        cmdWrite(RxBuffer);                        
                    break;

                    case CMD_JUMP:                        
                        cmdjump(RxBuffer);                        
                        break;

                    case CMD_WP_ON:                        
                        write_prot(RxBuffer);                        
                    break;

                    case CMD_WP_OFF:                        
                        Remove_wr_prot(RxBuffer);                        
                    break;

                    case CMD_UPDATE:                        
                        update(RxBuffer);                        
                    break;
                }
            case USART_CMD_NONE:                
                LED_ON;

            break;            
        }		
    }
}

void cmdErase(uint8_t *pucData)
{
    current_cmd_Status = USART_CMD_NONE;
    uint32_t crc_val=0;
    uint32_t pulCmd[] = { pucData[0], pucData[1] };
    memcpy(&crc_val, pucData + 2, sizeof(uint32_t));
    CRC_ResetDR();
    if(crc_val==CRC_CalcBlockCRC(pulCmd, 2))
    {
        FLASH_Unlock();
        if(RxBuffer[1]==0xff)
        {	
            FLASH_EraseAllSectors(VoltageRange_3);
        }
        else
        {	
            FLASH_EraseSector(RxBuffer[1], VoltageRange_3);
        }
        FLASH_Lock();
        USART1_SendChar(ACK);

    } else {
        USART1_SendChar(NACK);
        return;

    }

}

void cmdWrite(uint8_t*pucData)
{
    current_cmd_Status = USART_CMD_NONE;
    uint32_t ulSaddr = 0, ulCrc = 0,val;

    memcpy(&ulSaddr, pucData + 1, sizeof(uint32_t));
    memcpy(&ulCrc, pucData + 5, sizeof(uint32_t));
    uint32_t pulData[5];
    uint32_t pullData[16]={0x55};
    for(int i = 0; i < 5; i++)
        pulData[i] = pucData[i];
    CRC_ResetDR();
    val=CRC_CalcBlockCRC(pulData, 5);

    if(ulCrc==val)
    {
        CRC_ResetDR();
        memset(RxBuffer, 0, 20*sizeof(char));
        USART1_SendChar(ACK);
        while(current_cmd_Status==USART_CMD_NONE);//wait for the rest of command
        current_cmd_Status = USART_CMD_NONE;
        memcpy(&ulCrc, pucData + 16, sizeof(uint32_t));
        for(int i = 0; i < 16; i++)       
            pullData[i] = pucData[i]; 
        CRC_ResetDR();
        val=CRC_CalcBlockCRC((uint32_t*)pullData, 16);
        if(ulCrc==val)
        {
            FLASH_Unlock();
            for (uint8_t i = 0; i < 16; i++) 
            {
                FLASH_ProgramByte(ulSaddr,pucData[i]);
                ulSaddr += 1;
            }
            FLASH_Lock();
            USART1_SendChar(ACK);
        } else {
            USART1_SendChar(NACK);
            return;
        }

    } else {
        USART1_SendChar(NACK);
        return;

    }

}

void cmdjump(uint8_t*p)
{
    uint32_t crc=0,val=0;
    current_cmd_Status = USART_CMD_NONE;
    memcpy(&crc, p + 5, sizeof(uint32_t));
    uint32_t pulData[5];
    for(int i = 0; i < 5; i++)
        pulData[i] = p[i];

    CRC_ResetDR();    
    val=CRC_CalcBlockCRC(pulData, 5);
    if(crc==val)
    {   /* Get jump address */
        uint32_t *address = (uint32_t*) *(uint32_t *)(p+1);
        jump_to_new_app(address, USART1_SendChar);

    } else {
        USART1_SendChar(NACK);
        return;

    }
}

void write_prot(uint8_t*p)
{
    current_cmd_Status = USART_CMD_NONE;
    uint32_t crc_val=0;
    uint32_t pulCmd[] = { p[0], p[1] };
    memcpy(&crc_val, p+2, sizeof(uint32_t));
    CRC_ResetDR();
    if(crc_val==CRC_CalcBlockCRC(pulCmd, 2))
    {
        FLASH_Unlock();
        FLASH_OB_Unlock();
        add_write_prot(p[1]);
        while(FLASH_WaitForLastOperation()==FLASH_BUSY);
        FLASH_OB_Lock();
        FLASH_Lock();
        USART1_SendChar(ACK);

    } else {
        USART1_SendChar(NACK);
        return;

    }

}

void Remove_wr_prot(uint8_t*p)
{
    current_cmd_Status = USART_CMD_NONE;
    uint32_t crc_val=0;
    uint32_t pulCmd[] = { p[0], p[1] };
    memcpy(&crc_val, p+2, sizeof(uint32_t));
    CRC_ResetDR();
    if(crc_val==CRC_CalcBlockCRC(pulCmd, 2))
    {
        FLASH_Unlock();
        FLASH_OB_Unlock();
        remove_write_prot(p[1]);
        FLASH_OB_Lock();
        FLASH_Lock();
        USART1_SendChar(ACK);

    } else {
        USART1_SendChar(NACK);
        return;

    }
}

void update(uint8_t*p)
{
    uint32_t crc=0,val=0;
    current_cmd_Status = USART_CMD_NONE;
    memcpy(&crc, p + 5, sizeof(uint32_t));
    uint32_t pulData[5];
    for(int i = 0; i < 5; i++)
        pulData[i] = p[i];
    CRC_ResetDR();
    val=CRC_CalcBlockCRC(pulData, 5);
    if(crc==val){
        uint32_t address =  *(uint32_t *)(p+1);
        if(FLASH_SADDR == (address & FLASH_SADDR)) {//valid image   
            USART1_SendChar(ACK);
            //save the new App address
            FLASH_Unlock();
            FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);
            FLASH_Lock();
            FLASH_Unlock();
            FLASH_ProgramWord((uint32_t)(IMAGE_SADDR),address);
            FLASH_Lock();

        } else {
            USART1_SendChar(NACK);

        }

    } else {
        USART1_SendChar(NACK);

    }
}
