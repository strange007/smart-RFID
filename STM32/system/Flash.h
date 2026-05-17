#ifndef __FLASH_H__
#define __FLASH_H__
#include "stdint.h"

//#define FLASH_ADDR1 0x0801F000
//#define FLASH_ADDR2 0x0801F400
//#define FLASH_ADDR3 0x0801F800
//#define FLASH_ADDR4 0x0801FC00

#define EXIST_ADDR 0x0801D400

#define FLASH_ADDR0 0x0801D800
#define FLASH_ADDR1 0x0801DC00
#define FLASH_ADDR2 0x0801E000
#define FLASH_ADDR3 0x0801E400
#define FLASH_ADDR4 0x0801E800
#define FLASH_ADDR5 0x0801EC00
#define FLASH_ADDR6 0x0801F000
#define FLASH_ADDR7 0x0801F400
#define FLASH_ADDR8 0x0801F800
#define FLASH_ADDR9 0x0801FC00

void FLASH_W(uint32_t add,uint8_t dat1,uint8_t dat2,uint8_t dat3,uint8_t dat4);
uint16_t FLASH_R(uint32_t add);
uint8_t FLASH_R2(uint32_t add);
void FLASH_Clear(uint32_t add);
void FLASH_W2(uint32_t add,uint8_t dat);
void FLASH_W3(uint32_t add,uint8_t dat1,uint8_t dat2,uint8_t dat3,uint8_t dat4,uint8_t number);
void FLASH_W4(uint32_t add, u8* dat);
#endif

