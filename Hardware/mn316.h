#ifndef _MN316_H_
#define _MN316_H_

#include "stm32f10x.h"

#define REV_OK		0
#define REV_WAIT	1

#define MN316_RX_MAX	1024

#define MN316_USART		USART2
#define MN316_BAUDRATE	9600

#define SERVER_HOST		"47.112.120.143"
#define SERVER_PORT		"1883"
#define CLIENT_ID		"nb-iot_MN316_Client"

void MN316_Init(void);
void MN316_Clear(void);
void MN316_SendData(unsigned char *data, unsigned short len);
unsigned char *MN316_GetIPD(unsigned short timeOut);
_Bool MN316_SendCmdWithTimeout(char *cmd, char *res, unsigned short timeOut);
void USART2_IRQHandler_MN316(void);

#endif
