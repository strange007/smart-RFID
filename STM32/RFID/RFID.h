#ifndef __RFID_H__
#define __RFID_H__

#include "stm32f10x.h"

#define TAG_CACHE_SIZE   10         // 标签缓存大小
#define EPC_LEN          12         // EPC长度  
#define TIME_WINDOW_MS   100        // 时间窗口，单位毫秒，同一标签在此时间内再次出现视为重复
#define RSSI_THRESHOLD_DEFAULT -70  // dBm RSSI数值,设定大于-70dBM的信号良好

#define Check_True   1              //校验成功
#define Check_False   0             //校验失败
typedef struct
{
    uint8_t  epc[EPC_LEN];          // 标签EPC
    uint32_t last_time;             // 上次识别时间
    uint8_t  valid;                 // 是否有效
    int8_t   rssi;                  // 信号强度
} TagNode;


void RFID_SearchOnce(void);                                 //向RFID发送单次轮询命令

uint8_t RFID_GetRxFlag(void);                               //获取串口接收包标志位

void Serial_SendByte(uint8_t Byte);                         //串口发送一个字节给RFID

void Serial_SendArray(uint8_t *Array, uint16_t Length);     //串口发送一个数组给RFID

void Serial_SendString(char *String);                       //串口发送一个字符串给RFID

void Serial_SendNumber(uint32_t Number, uint8_t Length);    //串口发送数字给RFID

void RFID_Clear(void);                                      //清空串口数据接收缓存

u8 RFID_Unpacket(void);                                     //串口数据解包,获取卡号

uint8_t RFID_CheckDuplicate(uint8_t *epc,int8_t rssi);      //判断标签是否短时间重复读取

void RFID_CleanExpired(void);                               //定期清理过期标签

int8_t RSSI_Filter(int8_t last_rssi, int8_t new_rssi);      //简单一阶低通滤波

u8 Get_Checksum(void);                                      //校验和检查

#endif

