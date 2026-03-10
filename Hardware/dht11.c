#include "dht11.h"

#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_Pin_8

#define DHT11_DQ_OUT(state) GPIO_WriteBit(DHT11_PORT, DHT11_PIN, (BitAction)(state))
#define DHT11_DQ_IN()       GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)

static void DHT11_SetPinOutput(void)
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = DHT11_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio_init);
}

static void DHT11_SetPinInput(void)
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = DHT11_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio_init);
}

static void DHT11_DelayUs(uint32_t us)
{
    uint32_t ticks;
    uint32_t start;
    uint32_t reload;

    reload = SysTick->LOAD + 1U;
    ticks = us * (SystemCoreClock / 1000000U);
    start = SysTick->VAL;

    while (ticks > 0U)
    {
        uint32_t now = SysTick->VAL;
        uint32_t elapsed;

        if (start >= now)
        {
            elapsed = start - now;
        }
        else
        {
            elapsed = start + (reload - now);
        }

        if (elapsed >= ticks)
        {
            break;
        }

        ticks -= elapsed;
        start = now;
    }
}

static void DHT11_DelayMs(uint32_t ms)
{
    while (ms--)
    {
        DHT11_DelayUs(1000U);
    }
}

void DHT11_Rst(void)
{
    DHT11_SetPinOutput();
    DHT11_DQ_OUT(0);
    DHT11_DelayMs(20U);
    DHT11_DQ_OUT(1);
    DHT11_DelayUs(30U);
}

uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;

    DHT11_SetPinInput();
    while (DHT11_DQ_IN() && retry < 100U)
    {
        retry++;
        DHT11_DelayUs(1U);
    }
    if (retry >= 100U)
    {
        return 1U;
    }

    retry = 0;
    while (!DHT11_DQ_IN() && retry < 100U)
    {
        retry++;
        DHT11_DelayUs(1U);
    }
    if (retry >= 100U)
    {
        return 1U;
    }

    return 0U;
}

uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;

    while (DHT11_DQ_IN() && retry < 100U)
    {
        retry++;
        DHT11_DelayUs(1U);
    }

    retry = 0;
    while (!DHT11_DQ_IN() && retry < 100U)
    {
        retry++;
        DHT11_DelayUs(1U);
    }

    DHT11_DelayUs(40U);
    return (uint8_t)DHT11_DQ_IN();
}

uint8_t DHT11_Read_Byte(void)
{
    uint8_t i;
    uint8_t data = 0;

    for (i = 0; i < 8U; i++)
    {
        data <<= 1;
        data |= DHT11_Read_Bit();
    }

    return data;
}

uint8_t DHT11_Read_Data(uint8_t *humiH, uint8_t *humiL, uint8_t *tempH, uint8_t *tempL)
{
    uint8_t buf[5];
    uint8_t i;

    if (humiH == 0 || humiL == 0 || tempH == 0 || tempL == 0)
    {
        return 1U;
    }

    DHT11_Rst();
    if (DHT11_Check() != 0U)
    {
        return 1U;
    }

    for (i = 0; i < 5U; i++)
    {
        buf[i] = DHT11_Read_Byte();
    }

    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
    {
        return 1U;
    }

    *humiH = buf[0];
    *humiL = buf[1];
    *tempH = buf[2];
    *tempL = buf[3];

    return 0U;
}

uint8_t DHT11_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio_init.GPIO_Pin = DHT11_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio_init);
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);

    DHT11_DelayMs(1200U);
    DHT11_Rst();
    return DHT11_Check();
}
