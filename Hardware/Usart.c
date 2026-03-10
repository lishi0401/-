#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define USART2_RX_BUFFER_SIZE 256

static uint8_t Usart2_RxData = 0;
static uint8_t Usart2_RxFlag = 0;
static uint8_t Usart2_RxBuffer[USART2_RX_BUFFER_SIZE];
static volatile uint16_t Usart2_RxWriteIndex = 0;
static volatile uint16_t Usart2_RxReadIndex = 0;

static uint8_t Usart3_RxData = 0;
static uint8_t Usart3_RxFlag = 0;

void Usart1_Init(uint32_t baudrate) {}
void Usart2_Init(uint32_t baudrate) {}
void Usart3_Init(uint32_t baudrate) {}

void Usart_SendString(USART_TypeDef* USARTx, uint8_t *str, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++)
    {
        USART_SendData(USARTx, str[i]);
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    }
}

/**
  * 函    数：USART1发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Usart1_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

/**
  * 函    数：USART1发送一个数组
  * 参    数：Array 要发送数组的首地址
  * 参    数：Length 要发送数组的长度
  * 返 回 值：无
  */
void Usart1_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Usart1_SendByte(Array[i]);
    }
}

/**
  * 函    数：USART1发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Usart1_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        Usart1_SendByte(String[i]);
    }
}

/**
  * 函    数：使用printf需要重定向的底层函数（用于USART1）
  * 参    数：保持原始格式即可，无需变动
  * 返 回 值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
    Usart1_SendByte(ch);
    return ch;
}

/**
  * 函    数：自己封装的printf函数（用于USART1）
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Usart1_Printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Usart1_SendString(String);
}

/**
  * 函    数：USART2发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Usart2_SendByte(uint8_t Byte)
{
    USART_SendData(USART2, Byte);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/**
  * 函    数：USART2发送一个数组
  * 参    数：Array 要发送数组的首地址
  * 参    数：Length 要发送数组的长度
  * 返 回 值：无
  */
void Usart2_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Usart2_SendByte(Array[i]);
    }
}

/**
  * 函    数：USART2发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Usart2_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        Usart2_SendByte(String[i]);
    }
}

/**
  * 函    数：自己封装的printf函数（用于USART2）
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Usart2_Printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Usart2_SendString(String);
}

/**
  * 函    数：USART3发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Usart3_SendByte(uint8_t Byte)
{
    USART_SendData(USART3, Byte);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

/**
  * 函    数：USART3发送一个数组
  * 参    数：Array 要发送数组的首地址
  * 参    数：Length 要发送数组的长度
  * 返 回 值：无
  */
void Usart3_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Usart3_SendByte(Array[i]);
    }
}

/**
  * 函    数：USART3发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Usart3_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        Usart3_SendByte(String[i]);
    }
}

/**
  * 函    数：自己封装的printf函数（用于USART3）
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Usart3_Printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Usart3_SendString(String);
}

/**
  * 函    数：获取USART2接收标志
  * 参    数：无
  * 返 回 值：接收标志
  */
uint8_t Usart2_GetReceivedFlag(void)
{
    return (uint8_t)(Usart2_RxWriteIndex != Usart2_RxReadIndex);
}

/**
  * 函    数：获取USART2接收数据
  * 参    数：无
  * 返 回 值：接收的数据
  */
uint8_t Usart2_GetRxData(void)
{
    return Usart2_RxData;
}

uint8_t Usart2_ReadByte(uint8_t *data)
{
    if (data == 0 || Usart2_RxWriteIndex == Usart2_RxReadIndex)
    {
        return 0;
    }

    *data = Usart2_RxBuffer[Usart2_RxReadIndex];
    Usart2_RxReadIndex = (uint16_t)((Usart2_RxReadIndex + 1U) % USART2_RX_BUFFER_SIZE);
    Usart2_RxData = *data;
    Usart2_RxFlag = (uint8_t)(Usart2_RxWriteIndex != Usart2_RxReadIndex);
    return 1;
}

/**
  * 函    数：清除USART2接收标志
  * 参    数：无
  * 返 回 值：无
  */
void Usart2_ClearReceivedFlag(void)
{
    Usart2_RxFlag = 0;
    Usart2_RxReadIndex = Usart2_RxWriteIndex;
}

/**
  * 函    数：获取USART3接收标志
  * 参    数：无
  * 返 回 值：接收标志
  */
uint8_t Usart3_GetReceivedFlag(void)
{
    if (Usart3_RxFlag == 1)
    {
        Usart3_RxFlag = 0;
        return 1;
    }
    return 0;
}

/**
  * 函    数：获取USART3接收数据
  * 参    数：无
  * 返 回 值：接收的数据
  */
uint8_t Usart3_GetRxData(void)
{
    return Usart3_RxData;
}

/**
  * 函    数：清除USART3接收标志
  * 参    数：无
  * 返 回 值：无
  */
void Usart3_ClearReceivedFlag(void)
{
    Usart3_RxFlag = 0;
}

/**
  * 函    数：USART2中断服务函数
  * 参    数：无
  * 返 回 值：无
  */
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint16_t next_index;

        Usart2_RxData = (uint8_t)USART_ReceiveData(USART2);
        Usart2_RxFlag = 1;
        next_index = (uint16_t)((Usart2_RxWriteIndex + 1U) % USART2_RX_BUFFER_SIZE);
        if (next_index != Usart2_RxReadIndex)
        {
            Usart2_RxBuffer[Usart2_RxWriteIndex] = Usart2_RxData;
            Usart2_RxWriteIndex = next_index;
        }
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}

/**
  * 函    数：USART3中断服务函数
  * 参    数：无
  * 返 回 值：无
  */
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        Usart3_RxData = USART_ReceiveData(USART3);
        Usart3_RxFlag = 1;
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}
