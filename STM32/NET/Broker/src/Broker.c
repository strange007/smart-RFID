//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "esp8266.h"

//协议文件
#include "Broker.h"
#include "mqttkit.h"
#include "Object.h"

//硬件驱动
#include "usart.h"
#include "delay.h"

//C库
#include <string.h>
#include <stdio.h>
#include "cJSON.h"


#define PROID			"9"

#define ACCESS_KEY		"M"

#define DEVICE_NAME		"t"

extern Object new_objetct[9];
extern u8 tempcard;
extern unsigned char esp8266_buf[512];
u8 respond = 0;
extern Send_Setting send_settting;
extern int flash_w_flag;
extern int8_t RSSI;
/*  ADD BEGIN - 优化后的心跳与断线计数 */
//extern volatile int disconnect;
extern volatile int lost_heartbeat;   
/* ADD END */
extern volatile u8 search;
extern volatile uint8_t clear_from ;
extern volatile uint8_t clear_to   ;
extern volatile uint8_t clear_req  ;

//==========================================================
//	函数名称：	Broker_Link
//
//	函数功能：	与broker创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与broker平台建立连接
//==========================================================
_Bool Broker_Link(void)
{

    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

    unsigned char *dataPtr;

    char authorization_buf[160];

    _Bool status = 1;


    if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 60, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
    {
        ESP8266_SendData(mqttPacket._data, mqttPacket._len);//上传平台
        dataPtr = ESP8266_GetIPD(250);									//等待平台响应
        if(dataPtr != NULL)
        {
            if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
            {
                switch(MQTT_UnPacketConnectAck(dataPtr))
                {
                    case 0:
                        UsartPrintf(USART_DEBUG, "Tips:	连接成功\r\n");
                        status = 0;
                        break;

                    case 1:
                        UsartPrintf(USART_DEBUG, "WARN:	连接失败：协议错误\r\n");
                        break;
                    case 2:
                        UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法的clientid\r\n");
                        break;
                    case 3:
                        UsartPrintf(USART_DEBUG, "WARN:	连接失败：服务器失败\r\n");
                        break;
                    case 4:
                        UsartPrintf(USART_DEBUG, "WARN:	连接失败：用户名或密码错误\r\n");
                        break;
                    case 5:
                        UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法链接(比如token非法)\r\n");
                        break;

                    default:
                        UsartPrintf(USART_DEBUG, "ERR:	连接失败：未知错误\r\n");
                        break;
                }
            }
        }

        MQTT_DeleteBuffer(&mqttPacket);								//删包
    }
    else
        UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");

    return status;

}

uint32_t Broker_FillBuf(char *buf)
{

    char text[48];

    memset(text, 0, sizeof(text));
    sprintf(text,  "{\"send_number\":%d,\"params\":{", send_settting.send_number);
    strcat(buf, text);

    if(send_settting.send_number > 0 && send_settting.sendwhich != 0)
    {

        for(u8 i = 0; i < 9; i++)
        {
            if((send_settting.sendwhich & (0x0001 << i)) != 0 && send_settting.send_number > 1)
            {
                memset(text, 0, sizeof(text));
                sprintf(text, "\"object%d\":{\"value\":{", i + 1);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"id\":%d,", i + 1);
                strcat(buf, text);
                if(new_objetct[i].RSSI != 0)
                {
                    memset(text, 0, sizeof(text));
                    sprintf(text, "\"RSSI\":%d,", new_objetct[i].RSSI);
                    strcat(buf, text);
                }
                memset(text, 0, sizeof(text));
                sprintf(text, "\"name\":\"%s\",", new_objetct[i].name);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"note\":\"%s\",", new_objetct[i].note);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"card\":\"%s\",}}", new_objetct[i].card);
                strcat(buf, text);

                send_settting.sendwhich &= ~(0x0001 << i);
                send_settting.send_number--;
            }
            else if((send_settting.sendwhich & (0x0001 << i)) != 0 && send_settting.send_number == 1)
            {
                memset(text, 0, sizeof(text));
                sprintf(text, "\"object%d\":{\"value\":{", i + 1);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"id\":%d,", i + 1);
                strcat(buf, text);
                if(new_objetct[i].RSSI != 0)
                {
                    memset(text, 0, sizeof(text));
                    sprintf(text, "\"RSSI\":%d,", new_objetct[i].RSSI);
                    strcat(buf, text);
                }
                memset(text, 0, sizeof(text));
                sprintf(text, "\"name\":\"%s\",", new_objetct[i].name);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"note\":\"%s\",", new_objetct[i].note);
                strcat(buf, text);
                memset(text, 0, sizeof(text));
                sprintf(text, "\"card\":\"%s\"}}", new_objetct[i].card);
                strcat(buf, text);

                send_settting.sendwhich &= ~(0x0001 << i);
                send_settting.send_number--;
            }
        }
    }

    strcat(buf, "}}");
    UsartPrintf(USART_DEBUG, "buf:%s\r\n", buf);
    return strlen(buf);
}

//==========================================================
//	函数名称：	Broker_SendData
//
//	函数功能：	上传数据到消息代理服务器
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：
//==========================================================
void Broker_SendData(void)
{

    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//协议包

    char buf[512];

    u32 body_len = 0, i = 0;

    //    UsartPrintf(USART_DEBUG, "Tips:	OneNet_SendData-MQTT\r\n");

    //    UsartPrintf(USART_DEBUG, "SendData: begin\r\n");
    memset(buf, 0, sizeof(buf));
    body_len = Broker_FillBuf(buf);		//获取当前需要发送的数据流的总长度
    //    UsartPrintf(USART_DEBUG, "FillBuf Done, len=%d\r\n", body_len);
    if(body_len)
    {
        //		UsartPrintf(USART_DEBUG, "Packing data...\r\n");
        if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0)				//封包
        {
            for(; i < body_len; i++)
                mqttPacket._data[mqttPacket._len++] = buf[i];

            ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//上传数据
            MQTT_DeleteBuffer(&mqttPacket);															//删包
        }
        else
            UsartPrintf(USART_DEBUG, "WARN:	EDP_NewBuffer Failed\r\n");
    }

}

//==========================================================
//	函数名称：	Broker_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：
//==========================================================
void Broker_Publish(const char *topic, const char *msg)
{

    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包

    UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);

    if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
    {
        ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求

        MQTT_DeleteBuffer(&mqtt_packet);										//删包
    }

}

//==========================================================
//	函数名称：	Broker_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void Broker_Subscribe(void)
{

    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包

    char topic_buf[56];
    const char *topic = topic_buf;

    snprintf(topic_buf, sizeof(topic_buf), "asset/command");

    UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);

    if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
    {
        ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求

        MQTT_DeleteBuffer(&mqtt_packet);										//删包
    }

}

//==========================================================
//	函数名称：	Broker_Ping
//
//	函数功能：	心跳请求
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void Broker_Ping(void)
{

    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包

    if(MQTT_PacketPing(&mqtt_packet) == 0)
    {
        ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//向平台发送订阅请求

        MQTT_DeleteBuffer(&mqtt_packet);										//删包
    }

}

//==========================================================
//	函数名称：	Broker_RevPro
//
//	函数功能：	订阅的消息解析
//
//	入口参数：	dataPtr：订阅的消息
//
//	返回参数：	无
//
//	说明：
//==========================================================
void Broker_RevPro(unsigned char *cmd)
{

    char *req_payload = NULL;
    char *cmdid_topic = NULL;

    unsigned short topic_len = 0;
    unsigned short req_len = 0;

    unsigned char qos = 0;
    static unsigned short pkt_id = 0;

    unsigned char type = 0;

    short result = 0;

    cJSON *raw_json, *params_json,*id_json,*delete_json;
    cJSON *search_json;

    type = MQTT_UnPacketRecv(cmd);
    switch(type)
    {
        case MQTT_PKT_PUBLISH:																//接收的Publish消息

            result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
            if(result == 0)
            {
//                UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
//                            cmdid_topic, topic_len, req_payload, req_len);

                raw_json = cJSON_Parse(req_payload);
                params_json = cJSON_GetObjectItem(raw_json, "params");
                delete_json = cJSON_GetObjectItem(params_json, "delete");
                if(delete_json != NULL)
                {
					id_json = cJSON_GetObjectItem(delete_json, "id");
                    if(id_json->type == cJSON_Number)
                    {
						flash_w_flag = id_json->valueint-1;
                        UsartPrintf(USART_DEBUG, "delete asset%d\r\n", id_json->valueint);
                    }
                }
    
                send_settting.time = GetTick();
                ESP8266_Clear();
                search_json = cJSON_GetObjectItem(params_json, "search");
                if(search_json != NULL)
                {
                    if(search_json->type == cJSON_Number)
                    {
                        if(search_json->valueint != search)
                        {
                            clear_req = 1;
                            clear_from = 1;
                            clear_to = 4;
                        }
                        search = search_json->valueint;
                        UsartPrintf(USART_DEBUG, "search:%d\r\n", search);
                    }
                }

                cJSON_Delete(raw_json);
                break;
            }

        case MQTT_PKT_PUBACK:														//发送Publish消息，服务器回复的Ack

            if(MQTT_UnPacketPublishAck(cmd) == 1)
            {
                disconnect = 0;
                UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
            }

            break;

        case MQTT_PKT_SUBACK:																//发送Subscribe消息的Ack

            if(MQTT_UnPacketSubscribe(cmd) == 0)
                UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
            else
                UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
            break;

        /*ADD BRGIN*/
        /*case MQTT_PKT_PINGRESP:																//发送ping消息的Ack
            disconnect--;
            UsartPrintf(USART_DEBUG, "Tips:	MQTT ping OK\r\n");
            break;*/
        case MQTT_PKT_PINGRESP:                                                             /* 发送ping消息的Ack */
            /* ADD BEGIN - 重置心跳丢失计数 */
            lost_heartbeat = 0;
            /* ADD END */
            UsartPrintf(USART_DEBUG, "Tips:	MQTT ping OK\r\n");
            break;
        /*ADD END*/

        default:
            result = -1;
            break;
    }

    ESP8266_Clear();									//清空缓存

    if(result == -1)
        return;

}

/* ADD BEGIN - 应用层保活消息 */
/**
  * @brief   发送应用层保活消息，防止手机热点 NAT 超时导致 MQTT 连接断开
  * @param   无
  * @retval  无
  * @note    发送空 JSON {} 到 asset/keepalive 主题，后端可忽略
  */
void Broker_SendKeepAlive(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = { NULL, 0, 0, 0 };
    const char* topic = "asset/keepalive";
    const char* msg = "{}";

    if (MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg),
        MQTT_QOS_LEVEL0, 0, 1, &mqttPacket) == 0) {
        ESP8266_SendData(mqttPacket._data, mqttPacket._len);
        MQTT_DeleteBuffer(&mqttPacket);
        UsartPrintf(USART_DEBUG, "Sent KeepAlive\n");
    }
}
/* ADD END */