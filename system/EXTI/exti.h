#ifndef __EXTI_H
#define __EXTI_H

#include "stm32f10x.h"

extern uint8_t alarmFlag;
extern uint8_t alarm_is_free;

void EXTIX_Init(void);

#endif
