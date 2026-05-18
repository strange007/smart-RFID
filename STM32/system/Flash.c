#include "stm32f10x.h"                  // Device header
#include "Flash.h"
//  Attention: flash擦写次数为10万次，故不可在死循环中反复调用flash函数  //

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数据
  * @retval  无
  */
void FLASH_W(uint32_t add, uint8_t dat1, uint8_t dat2, uint8_t dat3, uint8_t dat4)
{
    FLASH_Unlock(); //解锁FLASH编程擦除控制器
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_ErasePage(add);    //擦除指定地址页
    FLASH_ProgramHalfWord(add, dat1); //从指定页的addr地址开始写
    FLASH_ProgramHalfWord(add + 2, dat2);
    FLASH_ProgramHalfWord(add + 4, dat3);
    FLASH_ProgramHalfWord(add + 6, dat4);
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_Lock();    //锁定FLASH编程擦除控制器
}

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数据，合计64位卡号，number数量
  * @retval  无
  */
void FLASH_W3(uint32_t add, uint8_t dat1, uint8_t dat2, uint8_t dat3, uint8_t dat4, uint8_t number)
{
    FLASH_Unlock(); //解锁FLASH编程擦除控制器
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_ErasePage(add);    //擦除指定地址页
    FLASH_ProgramHalfWord(add, dat1); //从指定页的addr地址开始写
    FLASH_ProgramHalfWord(add + 2, dat2);
    FLASH_ProgramHalfWord(add + 4, dat3);
    FLASH_ProgramHalfWord(add + 6, dat4);
    FLASH_ProgramHalfWord(add + 8, number);
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_Lock();    //锁定FLASH编程擦除控制器
}

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数组，合计12个字节卡号
  * @retval  无
  */
void FLASH_W4(uint32_t add, u8* dat)
{
    FLASH_Unlock(); //解锁FLASH编程擦除控制器
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_ErasePage(add);    //擦除指定地址页
    FLASH_ProgramHalfWord(add, dat[0]); //从指定页的addr地址开始写
    FLASH_ProgramHalfWord(add + 2, dat[1]);
    FLASH_ProgramHalfWord(add + 4, dat[2]);
    FLASH_ProgramHalfWord(add + 6, dat[3]);
    FLASH_ProgramHalfWord(add + 8, dat[4]);
    FLASH_ProgramHalfWord(add + 10, dat[5]);
    FLASH_ProgramHalfWord(add + 12, dat[6]);
    FLASH_ProgramHalfWord(add + 14, dat[7]);
    FLASH_ProgramHalfWord(add + 16, dat[8]);
    FLASH_ProgramHalfWord(add + 18, dat[9]);
    FLASH_ProgramHalfWord(add + 20, dat[10]);
    FLASH_ProgramHalfWord(add + 22, dat[11]);
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_Lock();    //锁定FLASH编程擦除控制器
}

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数据
  * @retval  无
  */
void FLASH_W2(uint32_t add, uint8_t dat)
{
    FLASH_Unlock(); //解锁FLASH编程擦除控制器
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_ErasePage(add);    //擦除指定地址页
    FLASH_ProgramHalfWord(add, dat); //从指定页的addr地址开始写
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_Lock();    //锁定FLASH编程擦除控制器
}

/**
  * @brief    FLASH读出数据
  * @param    add 32位读出FLASH地址
  * @retval   16位数据
  */
uint16_t FLASH_R(uint32_t add)
{
    uint16_t a;
    a = *(uint16_t*)add;
    return a;
}

/**
  * @brief    FLASH读出数据
  * @param    add 32位读出FLASH地址
  * @retval   8位数据
  */
uint8_t FLASH_R2(uint32_t add)
{
    uint8_t a;
    a = *(uint8_t*)add;
    return a;
}

/**
  * @brief    擦除指定FLASH地址页内的内容
  * @param    add 32位FLASH地址
  * @retval   无
  */
void FLASH_Clear(uint32_t add)
{
    FLASH_Unlock(); //解锁FLASH编程擦除控制器
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_ErasePage(add);    //擦除指定地址页
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); //清除标志位
    FLASH_Lock();
}

/* ADD BEGIN - WiFi 配置存储函数（基于方案A：使用 FLASH_ADDR9 页） */

/**
  * @brief   保存 WiFi SSID 和密码到内部 FLASH（使用 FLASH_ADDR9 页）
  * @param   ssid: 字符串，最大32字节
  * @param   pwd:  字符串，最大32字节
  * @retval  无
  * @note    会自动擦除 FLASH_ADDR9 所在整页（1024字节），然后写入 SSID、密码和有效标志。
  *          FLASH_ADDR9 原本未被使用，擦除后仅前128字节被本函数占用，剩余空间保留。
  */
void WIFI_SaveConfig(char* ssid, char* pwd)
{
    uint8_t i;

    FLASH_Unlock();

    /* 擦除 FLASH_ADDR9 所在的整个页（1024字节） */
    FLASH_ErasePage(FLASH_ADDR9);

    /* 写入 SSID（最多32字节，每个半字存一个字符） */
    for (i = 0; i < 32 && ssid[i] != '\0'; i++) {
        FLASH_ProgramHalfWord(FLASH_WIFI_SSID_ADDR + i * 2, (uint16_t)ssid[i]);
    }
    /* 写入字符串结束符 */
    FLASH_ProgramHalfWord(FLASH_WIFI_SSID_ADDR + i * 2, 0x0000);

    /* 写入密码（最多32字节） */
    for (i = 0; i < 32 && pwd[i] != '\0'; i++) {
        FLASH_ProgramHalfWord(FLASH_WIFI_PWD_ADDR + i * 2, (uint16_t)pwd[i]);
    }
    FLASH_ProgramHalfWord(FLASH_WIFI_PWD_ADDR + i * 2, 0x0000);

    /* 写入有效标志 0x5A5A */
    FLASH_ProgramHalfWord(FLASH_WIFI_VALID_FLAG, 0x5A5A);

    FLASH_Lock();
}

/**
  * @brief   从内部 FLASH 读取 WiFi SSID 和密码
  * @param   ssid: 输出缓冲区，至少33字节
  * @param   pwd:  输出缓冲区，至少33字节
  * @param   max_len: 缓冲区最大长度
  * @retval  1 - 读取成功（有效标志存在）, 0 - 无有效配置
  */
uint8_t WIFI_LoadConfig(char* ssid, char* pwd, uint8_t max_len)
{
    uint8_t i;
    uint16_t ch;

    /* 检查有效标志 */
    if (FLASH_R(FLASH_WIFI_VALID_FLAG) != 0x5A5A) {
        return 0;
    }

    /* 读取 SSID */
    for (i = 0; i < max_len - 1; i++) {
        ch = FLASH_R(FLASH_WIFI_SSID_ADDR + i * 2);
        if (ch == 0xFFFF || ch == 0x0000) break;
        ssid[i] = (char)ch;
    }
    ssid[i] = '\0';

    /* 读取密码 */
    for (i = 0; i < max_len - 1; i++) {
        ch = FLASH_R(FLASH_WIFI_PWD_ADDR + i * 2);
        if (ch == 0xFFFF || ch == 0x0000) break;
        pwd[i] = (char)ch;
    }
    pwd[i] = '\0';

    return 1;
}
/* ADD END */