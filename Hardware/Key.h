#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define KEY1    GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)
#define KEY2    GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)
#define KEY3    GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3)

#define KEY1_PRES   1
#define KEY2_PRES   2
#define KEY3_PRES   3

void KEY_Init(void);
uint8_t KEY_Scan(uint8_t mode);

#endif
