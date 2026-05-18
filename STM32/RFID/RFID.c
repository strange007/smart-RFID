#include "RFID.h"
#include "delay.h"
#include "string.h"
#include "usart.h"

u8 RFID_TxPacket[7];				//发送数据包数组:FF 01 02 03 04 FE
u8 RFID_buf[512];				//接收数据包数组
u32 RFID_cnt = 0, RFID_cntPre = 0;
u8 RFID_RxFlag;					//接收数据包数组标志位
u8 RFIDCard[12] = {0};
int8_t RSSI = 0;

TagNode tagCache[TAG_CACHE_SIZE];

// 动态轮询参数（按现场读卡节奏可调）
#define RFID_POLL_MIN_MS      30        // 最快30ms一次
#define RFID_POLL_MAX_MS      300       // 最慢300ms一次
#define RFID_POLL_STEP_MS     20        // 每次调整20ms
#define RFID_POLL_HIT_STREAK  2         // 连续命中多少次就加速
#define RFID_POLL_MISS_STREAK 3         // 连续未命中多少次就减速
#define RFID_POLL_INIT_MS     80        // 初始轮询间隔80ms

// 动态轮询状态
static uint32_t rfid_poll_interval_ms = RFID_POLL_INIT_MS;// 当前轮询间隔
static uint32_t rfid_last_poll_tick = 0;// 上次发送轮询命令的时间
static uint32_t rfid_last_hit_tick = 0;// 上次成功读到标签的时间
static uint8_t rfid_hit_streak = 0;// 连续命中计数
static uint8_t rfid_miss_streak = 0;// 连续未命中计数

//判断标签是否短时间重复读取
uint8_t RFID_CheckDuplicate(uint8_t *epc, int8_t rssi)
{
    uint32_t now = GetTick();   // 1ms计时器

    //滤波
    int8_t filtered_rssi = rssi;

    // 1查找是否已经存在
    for(int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        //若存在
        if(tagCache[i].valid &&
                memcmp(tagCache[i].epc, epc, EPC_LEN) == 0)
        {
            filtered_rssi = RSSI_Filter(tagCache[i].rssi, rssi);
            // 信号过弱,忽略
            if(filtered_rssi < RSSI_THRESHOLD_DEFAULT)
            {
                return 1; // 认为无效
            }

            // 判断时间窗口
            if(now - tagCache[i].last_time < TIME_WINDOW_MS)
            {
                tagCache[i].rssi = filtered_rssi;
                return 1;   // 重复
            }
            else
            {
                tagCache[i].last_time = now;
                tagCache[i].rssi = filtered_rssi;
                return 0;   // 超时,允许触发
            }
        }
    }

    // 2不存在,插入新标签
    for(int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        if(!tagCache[i].valid)
        {
            memcpy(tagCache[i].epc, epc, EPC_LEN);
            tagCache[i].last_time = now;
            tagCache[i].rssi = filtered_rssi;
            tagCache[i].valid = 1;
            return 0;
        }
    }

    // 3如果缓存满了,就用最旧代替(LRU)
    int oldest = 0;
    for(int i = 1; i < TAG_CACHE_SIZE; i++)
    {
        if(tagCache[i].last_time < tagCache[oldest].last_time)
        {
            oldest = i;
        }
    }

    memcpy(tagCache[oldest].epc, epc, EPC_LEN);
    tagCache[oldest].last_time = now;
    tagCache[oldest].rssi = filtered_rssi;
    tagCache[oldest].valid = 1;

    return 0;
}

//定期清理过期标签
void RFID_CleanExpired(void)
{
    uint32_t now = GetTick();

    for(int i = 0; i < TAG_CACHE_SIZE; i++)
    {
        if(tagCache[i].valid &&
                now - tagCache[i].last_time > TIME_WINDOW_MS)
        {
            tagCache[i].valid = 0;
        }
    }
}


int8_t RSSI_Filter(int8_t last_rssi, int8_t new_rssi)
{
    return (last_rssi * 7 + new_rssi) / 8; // 简单一阶低通
}

//校验和检验
u8 Get_Checksum(void)
{
    u8 i = 1;
	u32 sum=0;
    while(i < RFID_cnt - 1)
    {
		sum+=RFID_buf[i++];
    }
	if((sum&0x000000FF)==RFID_buf[RFID_cnt-1])
		return Check_True;
	else return Check_False;
}

/**
  * 函  数:串口发送一个字节
  * 参  数:Byte 要发送的一个字节
  * 返回值:无
  */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);		//将字节数据写入数据寄存器,写入后自动生成USART时序波形
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	//等待发送完成
    /*下次写入数据寄存器会自动清除发送完成标志位，故此循环后，无需清除标志位*/
}

/**
  * 函  数:串口发送一个数组
  * 参  数:Array 要发送的数组的首地址
  * 参  数:Length 要发送的数组的长度
  * 返回值:无
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i ++)		//遍历数组
    {
        Serial_SendByte(Array[i]);		//依次调用Serial_SendByte发送每个字节数据
    }
}

/**
  * 函  数:串口发送一个字符串
  * 参  数:String 要发送的字符串的首地址
  * 返回值:无
  */
void Serial_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i ++)//遍历字符数组(字符串),遇到字符串结束标志位后停止
    {
        Serial_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数据
    }
}

/**
  * 函  数:次方函数(内部使用)
  * 返回值:返回值等于X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;	//设置结果初始值为1
    while (Y --)			//执行Y次
    {
        Result *= X;		//将X累乘到结果
    }
    return Result;
}

/**
  * 函  数:串口发送数字
  * 参  数:Number 要发送的数字，范围：0~4294967295
  * 参  数:Length 要发送的数字的长度
  * 返回值:无
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i ++)		//根据数字长度遍历数字的每一位
    {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');	//依次调用Serial_SendByte发送每个字节数据
    }
}



/**
  * 函  数:串口发送数据包
  * 参  数:无
  * 返回值:无
  * 说  明:调用此函数后，Serial_TxPacket数组的内容加上包头包尾后，作为数据包发送出去
  */

/**
  * 函  数:向RFID发送单次轮询命令
  * 参  数:无
  * 返回值:无
  */
void RFID_SearchOnce(void)
{
    uint32_t now = GetTick();//获取当前时间戳

    // 时间闸门：不到间隔就不发命令
    if ((uint32_t)(now - rfid_last_poll_tick) < rfid_poll_interval_ms)
    {
        return;
    }

    // 超过一个周期未命中，则累计 miss，达到阈值就放慢
    if ((uint32_t)(now - rfid_last_hit_tick) > rfid_poll_interval_ms)
    {
        if (rfid_miss_streak < RFID_POLL_MISS_STREAK) {// 累计 miss
            rfid_miss_streak++;
        }
        rfid_hit_streak = 0;// 重置命中计数

        if (rfid_miss_streak >= RFID_POLL_MISS_STREAK && rfid_poll_interval_ms < RFID_POLL_MAX_MS)// 达到miss阈值，且未达最大间隔，则放慢
        {
            rfid_poll_interval_ms += RFID_POLL_STEP_MS;// 增加间隔
            if (rfid_poll_interval_ms > RFID_POLL_MAX_MS)// 不超过最大间隔
            {
                rfid_poll_interval_ms = RFID_POLL_MAX_MS;// 设置为最大间隔
            }
            rfid_miss_streak = 0;// 重置未命中计数
        }
    }

    rfid_last_poll_tick = now;// 更新上次轮询时间

    Serial_SendByte(0xBB);
    u8 temp[5] = {0x00, 0x22, 0x00, 0x00, 0x22};
    Serial_SendArray(temp, 5);
    Serial_SendByte(0x7E);
}

/**
  * 函  数:获取串口接收包标志位
  * 参  数:无
  * 返回值:串口接收包标志位，范围：0~1，接收到数据包后，标志位置1，读取后标志位自动清零
  */
uint8_t RFID_GetRxFlag(void)
{
    if (RFID_RxFlag == 1)			//如果标志位为1
    {
        RFID_RxFlag = 0;
        return 1;					//则返回1,并自动清零标志位
    }
    return 0;						//如果标志位为0,则返回0
}

/**
  * 函  数:清空串口数据接收缓存
  * 参  数:无
  * 返回值:无
  */
void RFID_Clear(void)
{
    memset(RFID_buf, 0, sizeof(RFID_buf));
    RFID_cnt = 0;
}

/**
  * 函  数:串口数据解包,获取卡号
  * 参  数:无
  * 返回值:成功获取卡号返回1
  */
u8 RFID_Unpacket(void)
{
    if(RFID_buf[2] == 0x22&&Get_Checksum())
    {
        for(u8 i = 0; i < 12; i++)
        {
            RFIDCard[i] = RFID_buf[i + 8];
        }
        RSSI = RFID_buf[5];

        // 命中一次，提速
        rfid_last_hit_tick = GetTick();// 更新上次命中时间
        if (rfid_hit_streak < RFID_POLL_HIT_STREAK)// 累计 hit
        {
            rfid_hit_streak++;
        }
        rfid_miss_streak = 0;// 重置未命中计数
        if (rfid_hit_streak >= RFID_POLL_HIT_STREAK && rfid_poll_interval_ms > RFID_POLL_MIN_MS)// 达到hit阈值，且未达最小间隔，则加速
        {
            if (rfid_poll_interval_ms > RFID_POLL_MIN_MS + RFID_POLL_STEP_MS)// 判断减去一步后会不会低于最小间隔
            {
                rfid_poll_interval_ms -= RFID_POLL_STEP_MS;// 减少间隔
            }
            else
            {
                rfid_poll_interval_ms = RFID_POLL_MIN_MS;// 设置为最小间隔
            }
            rfid_hit_streak = 0;// 重置命中计数
        }
        RFID_Clear();
        return 1;
    }
    else
    {
        RFID_Clear();
        return 0;
    }

}

/**
  * 函  数:USART1中断函数
  * 参  数:无
  * 返回值:无
  */
void USART1_IRQHandler(void)
{

    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) //接收中断
    {
        if(RFID_cnt >= sizeof(RFID_buf))RFID_cnt = 0; //防止串口被刷爆
        RFID_buf[RFID_cnt] = USART1->DR;
        if(RFID_buf[RFID_cnt] == 0x7E)RFID_RxFlag = 1;
        else RFID_cnt++;
        USART_ClearFlag(USART1, USART_FLAG_RXNE);
    }

}
