//单片机头文件
#include "stm32f10x.h"
//网络
#include "Broker.h"
#include "esp8266.h"

//硬件驱动
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include "Timer.h"
#include "Flash.h"
#include "RFID.h"

//C库
#include <string.h>
#include <stdio.h>

#define Broker_Address		"AT+CIPSTART=\"TCP\",\"192.168.199.100\",1883\r\n"//MQTT代理服务器地址
#include "Object.h"
void Hardware_Init(void);
void Read_Card(void);
void Check_Card(void);

char oled_str[50];
volatile uint8_t clear_from = 0;
volatile uint8_t clear_to   = 0;
volatile uint8_t clear_req  = 0;
u8 tempcard = 10;
extern u8 RFIDCard[12];
u8 ID_temp[10][12];
Object new_objetct[9];
int flash_w_flag = -1;
extern u8 RFID_buf[512];
extern int8_t RSSI;



/* ADD BEGIN - 手持化改造：外部声明 */
extern uint8_t ESP8266_IsConnected(void);
extern void ESP8266_ConnectToAP(void);
/* ADD END */

Send_Setting send_settting;
/* ADD BEGIN - 优化后的心跳与断线计数 */
//volatile int disconnect = 0;

#define HEARTBEAT_INTERVAL_MS   30000   /* 心跳间隔改为30秒（原60秒） */
#define MAX_LOST_HEARTBEAT      3       /* 连续3次无响应判定断线（原10次） */

volatile int lost_heartbeat = 0;        /* 替代原来的 disconnect */
static uint32_t last_keepalive = 0;     /* 应用层保活计时（5分钟发送一次） */
/* ADD END */

volatile u8 search = 0;

int main(void)
{
    unsigned char *dataPtr = NULL;

    memset(ID_temp, 0xFF, sizeof(ID_temp));
    Hardware_Init();				//初始化外围硬件

    /*ADD BEGIN*/
    // 插入临时调用,之后可删去
    WIFI_SaveConfig("SSID", "password");
	/*ADD END*/

    ESP8266_Init();					//初始化ESP8266

    OLED_Clear();
    OLED_ShowString(1, 1, "Connect MQTTs");
    OLED_ShowString(2, 1, "Server...");
    ESP8266_CMD Connect = {NULL, NULL, 0};
    Connect.cmd =  Broker_Address;
    Connect.res = "CONNECT";
    while(ESP8266_SendCmd(&Connect))
        DelayXms(500);

    OLED_ShowString(3, 1, "Connect MQTT ");
    OLED_ShowString(4, 1, "Server Success");
    DelayXms(500);

    OLED_Clear();
    OLED_ShowString(1, 1, "Device login ...");

    Connect.cmd =  Broker_Address;
    Connect.res = "CONNECT";
    Connect.debug = 1;
    while(Broker_Link())			//接入broker
    {
        ESP8266_SendCmd(&Connect);
        DelayXms(500);
    }
    OLED_ShowString(3, 1, "login finish");
    Broker_Subscribe();
    OLED_Clear();


    while(1)
    {
        /* ADD BEGIN - 定期检查 WiFi 连接，断开则自动重连（每10秒检查一次） */
        static uint32_t last_wifi_check = 0;
        if (GetTick() - last_wifi_check >= 10000) {
            if (!ESP8266_IsConnected()) {
                OLED_Clear();
                OLED_ShowString(1, 1, "WiFi Lost!     ");
                OLED_ShowString(2, 1, "Reconnecting...");
                ESP8266_ConnectToAP();                /* 重连热点 */
                /* 重连后需要重新建立 MQTT 连接 */
                OLED_ShowString(3, 1, "MQTT Reconnect ");
                while (Broker_Link()) {
                    DelayXms(500);
                }
                Broker_Subscribe();
                OLED_Clear();
                OLED_ShowString(1, 1, "WiFi & MQTT OK ");
                DelayXms(1000);
                OLED_Clear();
            }
            last_wifi_check = GetTick();
        }
        /* ADD END */

        if(search)
        {
            /***********读卡中*************/
            OLED_ShowCNString(1, 1, (uint32_t[])
            {
                0, 1, 2
            }, 3);
            OLED_ShowString(1, 7, "..."); //读卡中...
            RFID_SearchOnce();
            RFID_CleanExpired();
            Read_Card();
            if(RFID_GetRxFlag() == 1)
            {
//			UsartPrintf(USART_DEBUG, "%02X\r\n", RFID_buf[2]);
                if(RFID_Unpacket() == 1)
                {
                    sprintf(oled_str, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X ",
                            RFIDCard[0], RFIDCard[1], RFIDCard[2], RFIDCard[3],
                            RFIDCard[4], RFIDCard[5], RFIDCard[6], RFIDCard[7],
                            RFIDCard[8], RFIDCard[9], RFIDCard[10], RFIDCard[11]);

                    OLED_ShowCNString(2, 1, (uint32_t[])
                    {
                        1, 3
                    }, 2);
                    OLED_ShowString(2, 5, ":"); //卡号:
                    OLED_ShowString(3, 1, oled_str);
//                    UsartPrintf(USART_DEBUG, "%s\r\n", oled_str);
//                    UsartPrintf(USART_DEBUG, "%d\r\n", RSSI);
                    u32 address = 0;
                    Check_Card();
//                UsartPrintf(USART_DEBUG, "tempcard=%d\r\n", tempcard);
                    if(tempcard > 8)
                    {
                        OLED_ShowCNString(4, 1, (uint32_t[])
                        {
                            1, 3, 8, 61
                        }, 4); //卡号已满;
                        clear_from = 3;
                        clear_to   = 4;
                        TIM_Cmd(TIM2, ENABLE);
                    }
                    else
                    {
                        switch(tempcard)
                        {
                            case 0:
                                address = FLASH_ADDR0;
                                break;
                            case 1:
                                address = FLASH_ADDR1;
                                break;
                            case 2:
                                address = FLASH_ADDR2;
                                break;
                            case 3:
                                address = FLASH_ADDR3;
                                break;
                            case 4:
                                address = FLASH_ADDR4;
                                break;
                            case 5:
                                address = FLASH_ADDR5;
                                break;
                            case 6:
                                address = FLASH_ADDR6;
                                break;
                            case 7:
                                address = FLASH_ADDR7;
                                break;
                            case 8:
                                address = FLASH_ADDR8;
                                break;
                            default:
                                break;
                        }
                        send_settting.send_reagy = 1;
                        send_settting.sendwhich |= (0x0001 << tempcard);
                        send_settting.send_number++;
                        FLASH_W4(address, RFIDCard);
                        Read_Card();

                        clear_from = 3;
                        clear_to   = 4;
                        TIM_Cmd(TIM2, ENABLE);

                        new_objetct[tempcard].RSSI = RSSI;
                        new_objetct[tempcard].serial_number = tempcard;
                        strcpy(new_objetct[tempcard].card, oled_str);
                    }
                }
            }
        }
        else
        {
            OLED_ShowCNString(1, 1, (uint32_t[])
            {
                0, 1, 84, 85
            }, 4);//读卡暂停
        }

        if (clear_req)
        {
            OLED_ClearLines(clear_from, clear_to);
            clear_from = clear_to = 0;
            clear_req = 0;
        }

        if(flash_w_flag != -1 ) //0<=flash_w_flag<=9,flash_w_flag:objectX-1
        {

            UsartPrintf(USART_DEBUG, "Rewrite FLASH%d\r\n", flash_w_flag);
            u32 tempaddress = FLASH_ADDR0 + flash_w_flag * 0x00000400;
            FLASH_Clear(tempaddress);
            Read_Card();

            flash_w_flag = -1;
        }

        /* ADD BEGIN - 使用新心跳间隔和 lost_heartbeat */
        /*if (GetTick() - send_settting.time >= 60000 && !send_settting.send_reagy)
        {
            Broker_Ping();
            send_settting.time = GetTick();
            disconnect++;
        }
        if(send_settting.send_reagy)
        {
            Broker_SendData();							//发送数据
            send_settting.send_reagy = 0;
            disconnect++;
            ESP8266_Clear();
        }*/
        if (GetTick() - send_settting.time >= HEARTBEAT_INTERVAL_MS && !send_settting.send_reagy)
        {
            Broker_Ping();
            send_settting.time = GetTick();
            lost_heartbeat++;
        }
        /* ADD END */

        /* ADD BEGIN - 应用层保活：每5分钟发送一次空消息，防止手机热点NAT超时 */
        if (GetTick() - last_keepalive >= 300000) {
            Broker_SendKeepAlive();   /* 此函数需要在 Broker.c 中实现 */
            last_keepalive = GetTick();
        }
        /* ADD END */
        
        /* ADD BEGIN - 缩短断线判断阈值 */
          /*if (disconnect >= 10) //认为断连
        {
            UsartPrintf(USART_DEBUG, "Disconnect\r\n");
            ESP8266_Init();
            Connect.cmd =  Broker_Address;
            Connect.res = "CONNECT";
            Connect.debug = 0;
            while(Broker_Link())			//接入OneNET
            {
                ESP8266_SendCmd(&Connect);
                DelayXms(500);
            }
            UsartPrintf(USART_DEBUG, "Connect again\r\n");
            disconnect = 0;
            OLED_Clear();
        }*/
        if (lost_heartbeat >= MAX_LOST_HEARTBEAT) //认为断连
        {
            UsartPrintf(USART_DEBUG, "Disconnect\r\n");
            ESP8266_Init();
            Connect.cmd = Broker_Address;
            Connect.res = "CONNECT";
            Connect.debug = 0;
            while (Broker_Link())			//接入OneNET
            {
                ESP8266_SendCmd(&Connect);
                DelayXms(500);
            }
            UsartPrintf(USART_DEBUG, "Connect again\r\n");
            lost_heartbeat = 0;   /* 重置计数 */
            OLED_Clear();
        }
        /* ADD END */
      
        dataPtr = ESP8266_GetIPD(0);
        if(dataPtr != NULL)
            Broker_RevPro(dataPtr);

    }

}
/*
************************************************************
*	函数名称：	Hardware_Init
*
*	函数功能：	硬件初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		初始化单片机功能以及外接设备
************************************************************
*/
void Hardware_Init(void)
{

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);	//中断控制器分组设置

    Delay_Init();									//systick初始化

    Usart1_Init(115200);

    Usart2_Init(115200);							

    Usart3_Init(115200);

    Timer_Init();

    OLED_Init();			//初始化OLED

    Read_Card();
}
/*
************************************************************
*	函数名称：	Check_Card
*
*	函数功能：	检查EPC是否已经存在
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		检查EPC是否已经存在
************************************************************
*/
void Check_Card(void)
{
    Read_Card();
    u8 i = 0;
    tempcard = 0xFF;
    /* 1. 先判断是否已存在 */
    for(i = 0; i <= 8; i++)
    {
        if(RFIDCard[0] == ID_temp[i][0] &&
                RFIDCard[1] == ID_temp[i][1] &&
                RFIDCard[2] == ID_temp[i][2] &&
                RFIDCard[3] == ID_temp[i][3] &&
                RFIDCard[4] == ID_temp[i][4] &&
                RFIDCard[5] == ID_temp[i][5] &&
                RFIDCard[6] == ID_temp[i][6] &&
                RFIDCard[7] == ID_temp[i][7] &&
                RFIDCard[8] == ID_temp[i][8] &&
                RFIDCard[9] == ID_temp[i][9] &&
                RFIDCard[10] == ID_temp[i][10] &&
                RFIDCard[11] == ID_temp[i][11] )
        {
            tempcard = i;
            return;      // 已存在，直接返回
        }
    }

    /* 2. 再找第一个空位 */
    for(i = 0; i <= 8; i++)
    {
        if(ID_temp[i][0] == 0xFF &&
                ID_temp[i][1] == 0xFF &&
                ID_temp[i][2] == 0xFF &&
                ID_temp[i][3] == 0xFF &&
                ID_temp[i][4] == 0xFF &&
                ID_temp[i][5] == 0xFF &&
                ID_temp[i][6] == 0xFF &&
                ID_temp[i][7] == 0xFF &&
                ID_temp[i][8] == 0xFF &&
                ID_temp[i][9] == 0xFF &&
                ID_temp[i][10] == 0xFF &&
                ID_temp[i][11] == 0xFF
          )
        {
            tempcard = i;
            return;
        }
    }

    /* 3. 满了 */
    tempcard = 9;
}
/*
************************************************************
*	函数名称：	Read_Card
*
*	函数功能：	将EPC存储在FLASH
*
*	入口参数：	无
*
*	返回参数：	无
************************************************************
*/
void Read_Card(void)
{
    u8 i = 0;
    u32 address;
    while(i <= 8)
    {
        switch(i)
        {
            case 0:
                address = FLASH_ADDR0;
                break;
            case 1:
                address = FLASH_ADDR1;
                break;
            case 2:
                address = FLASH_ADDR2;
                break;
            case 3:
                address = FLASH_ADDR3;
                break;
            case 4:
                address = FLASH_ADDR4;
                break;
            case 5:
                address = FLASH_ADDR5;
                break;
            case 6:
                address = FLASH_ADDR6;
                break;
            case 7:
                address = FLASH_ADDR7;
                break;
            case 8:
                address = FLASH_ADDR8;
                break;
            default:
                break;
        }
        ID_temp[i][0] = FLASH_R(address);
        ID_temp[i][1] = FLASH_R(address + 2);
        ID_temp[i][2] = FLASH_R(address + 4);
        ID_temp[i][3] = FLASH_R(address + 6);
        ID_temp[i][4] = FLASH_R(address + 8);
        ID_temp[i][5] = FLASH_R(address + 10);
        ID_temp[i][6] = FLASH_R(address + 12);
        ID_temp[i][7] = FLASH_R(address + 14);
        ID_temp[i][8] = FLASH_R(address + 16);
        ID_temp[i][9] = FLASH_R(address + 18);
        ID_temp[i][10] = FLASH_R(address + 20);
        ID_temp[i][11] = FLASH_R(address + 22); //读取64位卡号
        i++;
    }

}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        clear_req = 1;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        TIM_Cmd(TIM2, DISABLE);
    }
}
