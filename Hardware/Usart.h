#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>
#define USART1_BaudRate 115200
#define USART2_BaudRate 9600
#define USART3_BaudRate 9600

#define DEBUG_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)  
// 串口初始化函数
void Usart1_Init(uint32_t baudrate);
void Usart2_Init(uint32_t baudrate);
void Usart3_Init(uint32_t baudrate);

// 串口发送函数
void Usart1_SendByte(uint8_t Byte);
void Usart1_SendArray(uint8_t *Array, uint16_t Length);
void Usart1_SendString(char *String);
void Usart1_Printf(char *format, ...);

void Usart2_SendByte(uint8_t Byte);
void Usart2_SendArray(uint8_t *Array, uint16_t Length);
void Usart2_SendString(char *String);
void Usart2_Printf(char *format, ...);

void Usart3_SendByte(uint8_t Byte);
void Usart3_SendArray(uint8_t *Array, uint16_t Length);
void Usart3_SendString(char *String);
void Usart3_Printf(char *format, ...);

void Usart_SendString(USART_TypeDef* USARTx, uint8_t *str, uint16_t len);

// 串口接收相关
uint8_t Usart2_GetReceivedFlag(void);
uint8_t Usart2_GetRxData(void);
void Usart2_ClearReceivedFlag(void);

uint8_t Usart3_GetReceivedFlag(void);
uint8_t Usart3_GetRxData(void);
void Usart3_ClearReceivedFlag(void);

#endif
