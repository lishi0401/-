#include "stm32f10x.h"

#define LED_RED_PIN     GPIO_Pin_11
#define LED_YELLOW_PIN  GPIO_Pin_13
#define LED_GREEN_PIN   GPIO_Pin_14

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_InitStructure.GPIO_Pin = LED_RED_PIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, LED_RED_PIN);
    
    GPIO_InitStructure.GPIO_Pin = LED_YELLOW_PIN | LED_GREEN_PIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, LED_YELLOW_PIN | LED_GREEN_PIN);
}

void LED_Red_On(void)
{
    GPIO_ResetBits(GPIOA, LED_RED_PIN);
}

void LED_Red_Off(void)
{
    GPIO_SetBits(GPIOA, LED_RED_PIN);
}

void LED_Red_Toggle(void)
{
    GPIO_WriteBit(GPIOA, LED_RED_PIN, (BitAction)!GPIO_ReadOutputDataBit(GPIOA, LED_RED_PIN));
}

void LED_Yellow_On(void)
{
    GPIO_ResetBits(GPIOB, LED_YELLOW_PIN);
}

void LED_Yellow_Off(void)
{
    GPIO_SetBits(GPIOB, LED_YELLOW_PIN);
}

void LED_Yellow_Toggle(void)
{
    GPIO_WriteBit(GPIOB, LED_YELLOW_PIN, (BitAction)!GPIO_ReadOutputDataBit(GPIOB, LED_YELLOW_PIN));
}

void LED_Green_On(void)
{
    GPIO_ResetBits(GPIOB, LED_GREEN_PIN);
}

void LED_Green_Off(void)
{
    GPIO_SetBits(GPIOB, LED_GREEN_PIN);
}

void LED_Green_Toggle(void)
{
    GPIO_WriteBit(GPIOB, LED_GREEN_PIN, (BitAction)!GPIO_ReadOutputDataBit(GPIOB, LED_GREEN_PIN));
}

void LED_All_Off(void)
{
    GPIO_SetBits(GPIOA, LED_RED_PIN);
    GPIO_SetBits(GPIOB, LED_YELLOW_PIN | LED_GREEN_PIN);
}
