#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

uint8_t DHT11_Init(void);
uint8_t DHT11_Read_Data(uint8_t *humiH, uint8_t *humiL, uint8_t *tempH, uint8_t *tempL);
uint8_t DHT11_Read_Byte(void);
uint8_t DHT11_Read_Bit(void);
uint8_t DHT11_Check(void);
void DHT11_Rst(void);

#endif
