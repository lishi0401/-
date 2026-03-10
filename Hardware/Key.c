#include "stm32f10x.h"
#include "Key.h"
#include "FreeRTOS.h"
#include "task.h"

#define KEY1_PIN    GPIO_Pin_0
#define KEY2_PIN    GPIO_Pin_1
#define KEY3_PIN    GPIO_Pin_3

void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN | KEY2_PIN | KEY3_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t KEY_Scan(uint8_t mode)
{
    static uint8_t key_up = 1;
    
    if (mode) key_up = 1;
    
    if (key_up && (KEY1 == 0 || KEY2 == 0 || KEY3 == 0))
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        key_up = 0;
        if (KEY1 == 0) return KEY1_PRES;
        else if (KEY2 == 0) return KEY2_PRES;
        else if (KEY3 == 0) return KEY3_PRES;
    }
    else if (KEY1 == 1 && KEY2 == 1 && KEY3 == 1)
    {
        key_up = 1;
    }
    
    return 0;
}
