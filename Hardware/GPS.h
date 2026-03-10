#ifndef __GPS_H
#define __GPS_H

#include "stm32f10x.h"

// 定义GPS数据结构体
typedef struct {
    char longitude[15]; // 经度
    char latitude[15];  // 纬度
    char time[10];      // 时间
    uint8_t valid;      // 定位状态 (1: 有效, 0: 无效)
} GPS_Data_t;

void GPS_Init(void);
void GPS_Process(void);
GPS_Data_t* GPS_GetData(void);
void GPS_Task(void *pvParameters); // 新增任务函数声明

#endif
