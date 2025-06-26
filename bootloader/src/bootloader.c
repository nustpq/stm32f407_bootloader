#include <string.h>
#include "bootloader.h"
#include "stm32f4xx_gpio.h"
#include "gpio.h"
#include "usart.h"
#include "delay.h"


unsigned char fw_version[]  = "[FW:V0.3]"; //fixed size string 

void boot_process()
{  
    delay_init(168); 
    led_init();		
    uart1_init(UART1_BAUD);       
    uart1_enable();
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;  //enable crc  
		
    delay_ms(500);	
    printf("Enter bootloader mode ...\r\n");
    printf("--  Bootloader version: %s\r\n", fw_version);  
    printf("--  Compiled on %s %s by PQ\r\n", __DATE__, __TIME__);

	kfifo_reset(&uart_fifo); //clear buffered data  
    uint32_t led_cnt = 0;
    while(1){
        
        //LED update
        if(++led_cnt >= LED_FREQ_IDLE)  
        {
            led_cnt = 0;	 
            LED_TOGGLE;			 
        }
        uart_cmd_parser();

    }
}
      


void uart_cmd_parser(void)
{	
    uint16_t len, counter;
    counter = kfifo_get_data_size(&uart_fifo);
    if(counter == 0 ) return;

    len = kfifo_get(&uart_fifo, &cmd_buffer[0], counter);
    if(len == 0) return;

    LED_ON;
    switch(cmd_buffer[0]){
        // case 's': // start session
        //     current_cmd_Status = USART_CMD_NONE;
        // break;

        case CMD_ERASE:                        
            cmdErase(cmd_buffer);                        
        break;

        case CMD_WRITE:                        
            cmdWrite(cmd_buffer);                        
        break;

        case CMD_JUMP:                        
            cmdjump(cmd_buffer);                        
            break;

        case CMD_WP_ON:                        
            write_prot(cmd_buffer);                        
        break;

        case CMD_WP_OFF:                        
            Remove_wr_prot(cmd_buffer);                        
        break;

        case CMD_UPDATE:                        
            update(cmd_buffer);                        
        break;   
        
        case CMD_VERSION: 
            get_fw_verison();                     
        break;

        default:
        break;
    }
    LED_OFF;		
    
}


void cmdErase(uint8_t *pucData)
{
    uint32_t crc_val=0;
    uint32_t pulCmd[] = { pucData[0], pucData[1] };
    memcpy(&crc_val, pucData + 2, sizeof(uint32_t));
    CRC_ResetDR();
    if( (crc_val==CRC_CalcBlockCRC(pulCmd, 2)) && (pulCmd[1] > FLASH_Sector_1) )//Sector 0 and 1 not allowed to be erased
    {
        FLASH_Unlock();
        if(pulCmd[1]==0xff)
        {	
            FLASH_EraseAllSectors(VoltageRange_3);
        }
        else
        {	
            FLASH_EraseSector(pulCmd[1], VoltageRange_3);
        }
        FLASH_Lock();
        USART1_SendChar(ACK);

    } else {
        USART1_SendChar(NACK);
        return;

    }

}

void cmdWrite(uint8_t *pucData)
{
    uint32_t ulSaddr = 0, ulCrc = 0, val;

    memcpy(&ulSaddr, pucData + 1, sizeof(uint32_t));
    memcpy(&ulCrc, pucData + (5+DATA_PACK_SIZE), sizeof(uint32_t));

    uint32_t pulData[5+DATA_PACK_SIZE];
    for(int i = 0; i < (5+DATA_PACK_SIZE); i++)
        pulData[i] = pucData[i];
    CRC_ResetDR();
    val=CRC_CalcBlockCRC(pulData, 5+DATA_PACK_SIZE);
    if(ulCrc==val)
    {
        FLASH_Unlock();
        for (int i = 0; i < DATA_PACK_SIZE; i++) 
        {
            FLASH_ProgramByte(ulSaddr,pucData[i+5]);
            ulSaddr += 1;
        }

        //spped up 
        // for (uint8_t i = 0; i < (128>>2); i++) 
        // {
        //     FLASH_ProgramWord(ulSaddr,pucData[i*4]);
        //     ulSaddr += 4;
        // }      
        FLASH_Lock();
        USART1_SendChar(ACK);
    } else {
        USART1_SendChar(NACK);
        return;
    }

}

void cmdjump(uint8_t *p)
{
    uint32_t crc=0,val=0;
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

void write_prot(uint8_t *p)
{

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

void Remove_wr_prot(uint8_t *p)
{

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

void update(uint8_t *p)
{
    uint32_t crc=0,val=0;

    memcpy(&crc, p + 5, sizeof(uint32_t));
    uint32_t pulData[5];
    for(int i = 0; i < 5; i++)
        pulData[i] = p[i];
    CRC_ResetDR();
    val=CRC_CalcBlockCRC(pulData, 5);
    if(crc==val){
        uint32_t address =  *(uint32_t *)(p+1);
        if((FLASH_SADDR == (address & FLASH_SADDR)) || (address == 0xFFFFFFFF)) {//valid image or clear address CMD 
            USART1_SendChar(ACK);
            
            //save the coef data in FLASH_Sector_1
            uint32_t coef_data[COEF_SIZE>>2];
            for(int i = 0; i < COEF_SIZE>>2; i++)
            {
                coef_data[i] = *((volatile uint32_t*) (COEF_SADDR + i*4 ));
            }

            FLASH_Unlock();
            //FLASH_DataCacheCmd(DISABLE); // FLASH擦除期间,必须禁止数据缓存
            FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);
         
            //write the new App start address
            FLASH_ProgramWord((uint32_t)(IMAGE_SADDR), address);

            //restore the coef data to FLASH_Sector_1
            for(int i = 0; i < COEF_SIZE>>2; i++)
            {
                FLASH_ProgramWord((uint32_t)(COEF_SADDR + i*4), coef_data[i]);
            }
            
            //FLASH_DataCacheCmd(ENABLE); // FLASH擦除结束,开启数据缓存
            FLASH_Lock();               // 上锁

        } else {
            USART1_SendChar(NACK);
        }

    } else {
        USART1_SendChar(NACK);

    }
}


void get_fw_verison(void)
{
    UART1_DMA_Send(&fw_version[0], sizeof(fw_version));                   
    // for(uint8_t k = 0; k<sizeof(fw_version); k++){
    //     USART1_SendChar(fw_version[k]);
    // }
}