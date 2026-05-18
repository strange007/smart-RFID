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

/* ADD BEGIN - 手持化改造：WiFi 配置存储地址（使用 FLASH_ADDR9 页内的偏移） */
#define FLASH_WIFI_SSID_ADDR     (FLASH_ADDR9 + 0x000)   /* SSID 存储起始地址（最大32字节） */
#define FLASH_WIFI_PWD_ADDR      (FLASH_ADDR9 + 0x040)   /* 密码存储起始地址（最大32字节） */
#define FLASH_WIFI_VALID_FLAG    (FLASH_ADDR9 + 0x080)   /* 配置有效标志（0x5A5A 表示已配置） */
/* ADD END */

void FLASH_W(uint32_t add,uint8_t dat1,uint8_t dat2,uint8_t dat3,uint8_t dat4);
uint16_t FLASH_R(uint32_t add);
uint8_t FLASH_R2(uint32_t add);
void FLASH_Clear(uint32_t add);
void FLASH_W2(uint32_t add,uint8_t dat);
void FLASH_W3(uint32_t add,uint8_t dat1,uint8_t dat2,uint8_t dat3,uint8_t dat4,uint8_t number);
void FLASH_W4(uint32_t add, u8* dat);
#endif

