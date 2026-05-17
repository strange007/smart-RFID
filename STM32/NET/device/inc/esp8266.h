#ifndef _ESP8266_H_
#define _ESP8266_H_
#include "stdint.h"
#define WIFI_SSID_ADDR 0x0801E800
#define WIFI_PWD_ADDR 0x0801EC00

#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志

typedef struct buffer
{

    char *cmd;//命令
    char *res;//希望看到的回复
	u8 debug;//1表示串口调试输出
}  ESP8266_CMD;

void ESP8266_Init(void);

void ESP8266_Clear(void);

//_Bool ESP8266_SendCmd(char *cmd, char *res);
_Bool ESP8266_SendCmd(ESP8266_CMD* cmd);
void ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);

void WIFI_W(uint32_t add1, uint32_t add2, char* SSID, char* PWD);

void WIFI_R(uint32_t add1, uint32_t add2);

#endif
