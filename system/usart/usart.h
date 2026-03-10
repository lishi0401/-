#ifndef _USART_H_
#define _USART_H_


#include "stm32f10x.h"

#define USE_OLED_DISPLAY 1

#define USART_DEBUG		USART1

void Usart1_Init(unsigned int baud);
void Usart2_Init(unsigned int baud);

#if !USE_OLED_DISPLAY
void Usart3_Init(unsigned int baud);
void USART3_IRQHandler(void);
#endif

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);
void DEBUG_LOG(char *fmt,...);

#endif
