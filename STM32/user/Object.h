#ifndef _OBJECT_H
#define _OBJECT_H

#include <stdio.h>
#include <string.h>
typedef struct buffer1
{
	
	uint8_t	serial_number;		//序号
	
	char card[64];		//卡号
	
	char name[16];			//名字
	
	char note[32];		//备注
	
	int RSSI;
	
}  Object;

typedef struct buffer2
{
	
	uint8_t	send_reagy;		//准备发送标志位
	
	u32 time;		//上一次发送时间
	
	u16 sendwhich;   //一次发送哪几个object
	
	u8 send_number;  //总共发多少个
	
}  Send_Setting;

#endif
