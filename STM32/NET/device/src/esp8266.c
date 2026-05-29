//单片机头文件
#include "stm32f10x.h"

//网络设备驱动
#include "esp8266.h"

//硬件驱动
#include "usart.h"
#include "oled.h"
#include "timer.h"
#include "delay.h"

//C库
#include <string.h>
#include <stdio.h>

/* CHANGE BEGIN */
//#define ESP8266_WIFI_INFO		"AT+CWJAP=\"HiWiFi\",\"5555720720\",\"d4:ee:07:03:e7:54\"\r\n"
/* CHANGE END */

unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;
extern u8 time;
//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void ESP8266_Clear(void)
{

    memset(esp8266_buf, 0, sizeof(esp8266_buf));
    esp8266_cnt = 0;
}

/* ADD BEGIN - 动态连接热点（从FLASH读取SSID/密码） */

/**
  * @brief   连接至 FLASH 中保存的 WiFi 热点
  * @param   无
  * @retval  无
  * @note    会尝试重试10次，每次间隔2秒；失败后 OLED 显示错误并进入死循环（可改为其他错误处理）
  */
void ESP8266_ConnectToAP(void)
{
    char ssid[33] = { 0 };
    char pwd[33] = { 0 };
    char cmd_buf[128];
    uint8_t retry = 0;
    ESP8266_CMD connectCmd;

    /* 从 FLASH 读取 WiFi 配置 */
    if (!WIFI_LoadConfig(ssid, pwd, 33)) {
        OLED_Clear();
        OLED_ShowString(1, 1, "No WiFi config!");
        OLED_ShowString(2, 1, "Please set via");
        OLED_ShowString(3, 1, "USB or UART");
        while (1);  /* 停止执行，等待外部配置 */
    }

    /* 构造 AT+CWJAP 命令 */
    sprintf(cmd_buf, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
    connectCmd.cmd = cmd_buf;
    connectCmd.res = "GOT IP";
    connectCmd.debug = 1;   /* 调试输出到串口，便于观察 */

    OLED_ShowString(4, 1, "Connecting WiFi ");
    while (ESP8266_SendCmd(&connectCmd) && retry++ < 10) {
        DelayXms(2000);
        OLED_ShowString(4, 1, "Retry...       ");
    }

    if (retry >= 10) {
        OLED_ShowString(4, 1, "WiFi Failed!   ");
        while (1);  /* 连接失败，停止 */
    }
    else {
        OLED_ShowString(4, 1, "WiFi Connected ");
        DelayXms(500);
    }
}

/**
  * @brief   检查 ESP8266 是否已连接 AP
  * @param   无
  * @retval  1 - 已连接, 0 - 未连接
  * @note    通过发送 AT+CWJAP? 并解析响应
  */
uint8_t ESP8266_IsConnected(void)
{
    ESP8266_CMD checkCmd;
    checkCmd.cmd = "AT+CWJAP?\r\n";
    checkCmd.res = "+CWJAP:\"";
    checkCmd.debug = 0;

    ESP8266_Clear();
    if (ESP8266_SendCmd(&checkCmd) == 0) {
        /* 找到 +CWJAP:" 说明已连接 */
        return 1;
    }
    return 0;
}
/* ADD END */


//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

    if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
        return REV_WAIT;

    if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
    {
        esp8266_cnt = 0;							//清0接收计数
//        UsartPrintf(USART_DEBUG,"buf: %s\r\n", esp8266_buf);
        return REV_OK;								//返回接收完成标志
    }

    esp8266_cntPre = esp8266_cnt;					//置为相同

    return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：
//==========================================================
_Bool ESP8266_SendCmd(ESP8266_CMD *cmd)
{

    unsigned char timeOut = 200;

    Usart_SendString(USART3, (unsigned char *)cmd->cmd, strlen((const char *)cmd->cmd));

    while(timeOut--)
    {
        if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
        {
            if(cmd->debug==1)
            {
                UsartPrintf(USART_DEBUG, "%s\r\n", esp8266_buf);
            }
            if(strstr((const char *)esp8266_buf, cmd->res) != NULL)		//如果检索到关键词
            {
                ESP8266_Clear();									//清空缓存

                return 0;
            }
        }

        DelayXms(10);
    }

    return 1;

}

//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

    char cmdBuf[32];

    ESP8266_Clear();								//清空接收缓存
    sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
//    UsartPrintf(USART_DEBUG, cmdBuf);
	ESP8266_CMD ESP8266_SendData = {NULL, NULL, 0};
    ESP8266_SendData.cmd =  cmdBuf;
    ESP8266_SendData.res = ">";
    ESP8266_SendData.debug = 0;
    if(!ESP8266_SendCmd(&ESP8266_SendData))//收到‘>’时可以发送数据
    {
//        UsartPrintf(USART_DEBUG, "SEND: MQTT CONNECT Sending\r\n");
        Usart_SendString(USART3, data, len);   //发送设备连接请求数据
    }
}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

    char *ptrIPD = NULL;

    do
    {
        if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
        {
            ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索“IPD”头
            if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
            {
                UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
            }
            else
            {
                ptrIPD = strchr(ptrIPD, ':');							//找到':'
                if(ptrIPD != NULL)
                {
                    ptrIPD++;
                    return (unsigned char *)(ptrIPD);
                }
                else
                    return NULL;

            }
        }

        DelayXms(50);													//延时等待
    }
    while(timeOut--);

    return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void ESP8266_Init(void)
{
    ESP8266_CMD ESP8266_Connect = {NULL, NULL, 0};
    ESP8266_Clear();
    UsartPrintf(USART_DEBUG, "1. AT\r\n");
    OLED_Clear();
    OLED_ShowString(1, 1, "1.AT...");
    ESP8266_Connect.cmd = "AT\r\n";
    ESP8266_Connect.res = "OK";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(500);
    }

//    UsartPrintf(USART_DEBUG, "2. CWMODE\r\n");
    OLED_ShowString(2, 1, "2.CWMODE...");
    ESP8266_Connect.cmd = "AT+CWMODE=1\r\n";
    ESP8266_Connect.res = "OK";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(500);
    }

//    UsartPrintf(USART_DEBUG, "3. AT+CWDHCP\r\n");
    OLED_ShowString(3, 1, "3.AT+CWDHCP...");
    ESP8266_Connect.cmd = "AT+CWDHCP=1,1\r\n";
    ESP8266_Connect.res = "OK";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(500);
    }


    /* ADD BEGIN - 替换为动态热点连接 */
//    UsartPrintf(USART_DEBUG, "4. CWJAP\r\n");
/*  OLED_ShowString(4, 1, "4.CWJAP...");
    ESP8266_Connect.cmd = ESP8266_WIFI_INFO;
    ESP8266_Connect.res = "GOT IP";
    while(ESP8266_SendCmd(&ESP8266_Connect))
    {
        DelayXms(5000);
    }*/

    
    OLED_ShowString(4, 1, "4.CWJAP...");
    ESP8266_ConnectToAP();   /* 使用从FLASH读取配置的连接函数 */
    /* ADD END */

    UsartPrintf(USART_DEBUG, "5. ESP8266 Init OK\r\n");
    OLED_Clear();
    OLED_ShowString(1, 1, "ESP8266 Init OK");
    DelayXms(500);
}

//==========================================================
//	函数名称：	USART3_IRQHandler
//
//	函数功能：	串口3收发中断
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void USART3_IRQHandler(void)
{

    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) //接收中断
    {
        if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //防止串口被刷爆
        esp8266_buf[esp8266_cnt++] = USART3->DR;
        USART_ClearFlag(USART3, USART_FLAG_RXNE);
    }

}

