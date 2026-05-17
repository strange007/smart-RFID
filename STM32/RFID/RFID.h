#ifndef __RFID_H__
#define __RFID_H__

/**

*/

#include "stm32f10x.h"

#define TAG_CACHE_SIZE   10
#define EPC_LEN          12
#define TIME_WINDOW_MS   10   // 10秒(单位毫秒)
#define RSSI_THRESHOLD_DEFAULT -70  // dBm RSSI数值,设定大于-70dBM的信号良好

#define Check_True   1
#define Check_False   0
typedef struct
{
    uint8_t  epc[EPC_LEN];
    uint32_t last_time;     // 上次识别时间
    uint8_t  valid;
    int8_t   rssi;        // 信号强度
} TagNode;


void RFID_SearchOnce(void);
uint8_t RFID_GetRxFlag(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void RFID_Clear(void);
u8 RFID_Unpacket(void);

uint8_t RFID_CheckDuplicate(uint8_t *epc,int8_t rssi);
void RFID_CleanExpired(void);
int8_t RSSI_Filter(int8_t last_rssi, int8_t new_rssi);
u8 Get_Checksum(void);
#endif

