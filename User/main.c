#include "Usart.h"
#include "freertos_demo.h"
#include "OLED.h"
#include "GPS.h"
#include "LED.h"
#include "FreeRTOS.h"
#include "task.h"
#include "BSP.h"

#include "string.h"
#include <stdio.h>

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    Usart1_Printf("Stack Overflow: %s\r\n", pcTaskName);
    while(1);
}

void vApplicationMallocFailedHook(void)
{
    Usart1_Printf("Malloc Failed!\r\n");
    while(1);
}

int main()
{
    BSP_Init();
    
    Usart1_Printf("\r\n========================================\r\n");
    Usart1_Printf("System Starting...\r\n");
    Usart1_Printf("========================================\r\n");
    
    LED_Green_On();
    
    FreeRTOS_Start();

    while (1)
    {
    }

    return 0;
}
