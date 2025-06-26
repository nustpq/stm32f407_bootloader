#ifndef __boot_cfg__
#define __boot_cfg__

#define IMAGE_SADDR      (uint32_t)0x08004000  //image's start address
#define FLASH_SADDR      (uint32_t)0x08000000
#define FREE_IMAGE       (uint32_t)0xffffffff

#define COEF_SADDR       (uint32_t)0x08004100 //offset = 256 
#define COEF_SIZE        64 //size =  64B

#endif
