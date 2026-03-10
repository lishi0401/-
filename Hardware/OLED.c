#include "stm32f10x.h"
#include "stm32f10x_i2c.h"
#include "OLED_Font.h"
#include "System_State.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Usart.h"
#include <string.h>

#define OLED_I2C                I2C1
#define OLED_I2C_ADDR           0x3C
#define OLED_I2C_TIMEOUT        10000U

static uint8_t last_page = 0xFF;

static ErrorStatus OLED_I2C_WaitEvent(uint32_t event)
{
    uint32_t timeout = OLED_I2C_TIMEOUT;

    while (I2C_CheckEvent(OLED_I2C, event) != SUCCESS)
    {
        if (--timeout == 0U)
        {
            return ERROR;
        }
    }

    return SUCCESS;
}

static void OLED_I2C_WriteBytes(uint8_t control, const uint8_t *data, uint16_t size)
{
    uint16_t i;

    if (size == 0U)
    {
        return;
    }

    I2C_GenerateSTART(OLED_I2C, ENABLE);
    if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS)
    {
        goto stop;
    }

    I2C_Send7bitAddress(OLED_I2C, OLED_I2C_ADDR << 1, I2C_Direction_Transmitter);
    if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS)
    {
        goto stop;
    }

    I2C_SendData(OLED_I2C, control);
    if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS)
    {
        goto stop;
    }

    for (i = 0; i < size; i++)
    {
        I2C_SendData(OLED_I2C, data[i]);
        if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS)
        {
            break;
        }
    }

stop:
    I2C_GenerateSTOP(OLED_I2C, ENABLE);
}

static void OLED_ShowStringLimited(uint8_t Line, uint8_t Column, const char *String, uint8_t MaxLen);
static uint32_t OLED_Pow(uint32_t X, uint32_t Y);
static void OLED_ShowPage_Standby(void);
static void OLED_ShowPage_Transport(void);
static void OLED_ShowPage_Alarm(void);
static void OLED_ShowPage_Checkout(void);
static void OLED_ClearTransportPage(void);

void OLED_I2C_Init(void)
{
    GPIO_InitTypeDef gpio_init;
    I2C_InitTypeDef i2c_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);

    gpio_init.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio_init);

    I2C_DeInit(OLED_I2C);
    i2c_init.I2C_ClockSpeed = 100000;
    i2c_init.I2C_Mode = I2C_Mode_I2C;
    i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c_init.I2C_OwnAddress1 = 0x00;
    i2c_init.I2C_Ack = I2C_Ack_Disable;
    i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(OLED_I2C, &i2c_init);
    I2C_Cmd(OLED_I2C, ENABLE);
}

void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_WriteBytes(0x00, &Command, 1);
}

void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_WriteBytes(0x40, &Data, 1);
}

void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));
    OLED_WriteCommand(0x00 | (X & 0x0F));
}

void OLED_Clear(void)
{
    uint8_t i;
    uint8_t j;
    uint8_t zero[16];

    memset(zero, 0, sizeof(zero));
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (i = 0; i < 8; i++)
        {
            OLED_I2C_WriteBytes(0x40, zero, sizeof(zero));
        }
    }
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    uint8_t index;

    if (Char < ' ' || Char > '~')
    {
        Char = ' ';
    }
    index = (uint8_t)(Char - ' ');

    OLED_SetCursor((uint8_t)((Line - 1) * 2), (uint8_t)((Column - 1) * 8));
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[index][i]);
    }

    OLED_SetCursor((uint8_t)((Line - 1) * 2 + 1), (uint8_t)((Column - 1) * 8));
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[index][i + 8]);
    }
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;

    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, (uint8_t)(Column + i), String[i]);
    }
}

static void OLED_ShowStringLimited(uint8_t Line, uint8_t Column, const char *String, uint8_t MaxLen)
{
    uint8_t i;

    for (i = 0; i < MaxLen && String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, (uint8_t)(Column + i), String[i]);
    }
    for (; i < MaxLen; i++)
    {
        OLED_ShowChar(Line, (uint8_t)(Column + i), ' ');
    }
}

static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;

    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;

    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line,
                      (uint8_t)(Column + i),
                      (char)(Number / OLED_Pow(10, Length - i - 1) % 10 + '0'));
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;

    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = (uint32_t)Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = (uint32_t)(-Number);
    }

    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line,
                      (uint8_t)(Column + i + 1),
                      (char)(Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0'));
    }
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    uint8_t SingleNumber;

    for (i = 0; i < Length; i++)
    {
        SingleNumber = (uint8_t)(Number / OLED_Pow(16, Length - i - 1) % 16);
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, (uint8_t)(Column + i), (char)(SingleNumber + '0'));
        }
        else
        {
            OLED_ShowChar(Line, (uint8_t)(Column + i), (char)(SingleNumber - 10 + 'A'));
        }
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;

    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line,
                      (uint8_t)(Column + i),
                      (char)(Number / OLED_Pow(2, Length - i - 1) % 2 + '0'));
    }
}

void OLED_Init(void)
{
    uint32_t i;
    uint32_t j;

    for (i = 0; i < 2000; i++)
    {
        for (j = 0; j < 2000; j++)
        {
        }
    }

    OLED_I2C_Init();

    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xD5);
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);

    OLED_Clear();
}

void OLED_ClearLine(uint8_t Line)
{
    uint8_t i;

    OLED_SetCursor((uint8_t)((Line - 1) * 2), 0);
    for (i = 0; i < 128; i++)
    {
        OLED_WriteData(0x00);
    }

    OLED_SetCursor((uint8_t)((Line - 1) * 2 + 1), 0);
    for (i = 0; i < 128; i++)
    {
        OLED_WriteData(0x00);
    }
}

void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Number, uint8_t IntLen, uint8_t DecLen)
{
    uint8_t i;
    uint32_t intPart;
    uint32_t decPart;
    uint32_t multiplier = 1;

    if (Number < 0)
    {
        OLED_ShowChar(Line, Column, '-');
        Number = -Number;
        Column++;
    }
    else
    {
        OLED_ShowChar(Line, Column, ' ');
        Column++;
    }

    intPart = (uint32_t)Number;

    for (i = 0; i < DecLen; i++)
    {
        multiplier *= 10;
    }
    decPart = (uint32_t)((Number - intPart) * multiplier + 0.5f);
    if (decPart >= multiplier)
    {
        intPart++;
        decPart = 0;
    }

    OLED_ShowNum(Line, Column, intPart, IntLen);
    OLED_ShowChar(Line, (uint8_t)(Column + IntLen), '.');
    OLED_ShowNum(Line, (uint8_t)(Column + IntLen + 1), decPart, DecLen);
}

void OLED_ShowHexBytes(uint8_t Line, uint8_t Column, uint8_t *Data, uint8_t Len)
{
    uint8_t i;

    for (i = 0; i < Len; i++)
    {
        OLED_ShowHexNum(Line, (uint8_t)(Column + i * 3), Data[i], 2);
        if (i < Len - 1)
        {
            OLED_ShowChar(Line, (uint8_t)(Column + i * 3 + 2), ' ');
        }
    }
}

static void OLED_ShowPage_Standby(void)
{
    Storage_State_t *s = &g_system_state.storage;
    static uint8_t last_enrollment_active = 0xFF;
    static Storage_UI_State_t last_ui_state = (Storage_UI_State_t)0xFF;

    if (last_page != PAGE_STANDBY)
    {
        last_enrollment_active = 0xFF;
        last_ui_state = (Storage_UI_State_t)0xFF;
    }

    if (last_enrollment_active != s->enrollment_active ||
        last_ui_state != s->ui_state)
    {
        OLED_ClearLine(1);
        OLED_ClearLine(2);
        OLED_ClearLine(3);
        OLED_ClearLine(4);
        last_enrollment_active = s->enrollment_active;
        last_ui_state = s->ui_state;
    }

    if (!s->enrollment_active)
    {
        OLED_ShowString(1, 1, "Input Closed");
        OLED_ShowString(2, 1, "ID:");
        OLED_ShowString(2, 5, g_system_state.transport.vehicle_id);
        OLED_ShowString(3, 1, "Hold in trans");
        OLED_ShowString(4, 1, "page");
    }
    else if (s->ui_state == STORAGE_UI_CARD_DETECTED)
    {
        OLED_ShowString(1, 1, "Card Detected");
        OLED_ShowHexBytes(2, 1, s->pending_card_id, CARD_ID_LEN);
        OLED_ShowString(3, 1, "WH:");
        OLED_ShowNum(3, 4, s->pending_warehouse, 1);
        OLED_ShowString(3, 7, "K1-K3+");
        OLED_ShowString(4, 1, "K2 Save");
    }
    else if (s->ui_state == STORAGE_UI_DUPLICATE)
    {
        OLED_ShowString(1, 1, "Duplicate Card");
        OLED_ShowHexBytes(2, 1, s->card_id, CARD_ID_LEN);
        OLED_ShowString(3, 1, "Please Retry");
        OLED_ShowString(4, 1, "Count:");
        OLED_ShowNum(4, 7, s->stored_count, 3);
    }
    else
    {
        OLED_ShowString(1, 1, "Wait For Card");
        OLED_ShowString(2, 1, "WH:");
        OLED_ShowNum(2, 4, s->current_warehouse, 1);
        OLED_ShowString(2, 7, "Cnt:");
        OLED_ShowNum(2, 11, s->stored_count, 3);
        OLED_ShowString(3, 1, "Yellow Blink");
        OLED_ShowString(4, 1, "Scan To Input");
    }
}

static void OLED_ShowPage_Transport(void)
{
    Transport_State_t *t = &g_system_state.transport;
    static uint8_t last_transport_view = 0xFF;

    if (last_page != PAGE_TRANSPORT)
    {
        last_transport_view = 0xFF;
    }

    if (last_transport_view != t->show_gps_page)
    {
        OLED_ClearTransportPage();
        last_transport_view = t->show_gps_page;
    }

    if (t->show_gps_page)
    {
        OLED_ShowString(1, 1, "GPS:");
        OLED_ShowString(1, 6, t->gps_valid ? "FIX" : "NOFIX");
        OLED_ShowString(2, 1, "LA:");
        if (t->gps_latitude[0] != '\0')
        {
            OLED_ShowStringLimited(2, 4, t->gps_latitude, 13);
        }
        else
        {
            OLED_ShowString(2, 4, "--");
        }
        OLED_ShowString(3, 1, "LO:");
        if (t->gps_longitude[0] != '\0')
        {
            OLED_ShowStringLimited(3, 4, t->gps_longitude, 13);
        }
        else
        {
            OLED_ShowString(3, 4, "--");
        }
        OLED_ShowString(4, 1, "TM:");
        if (t->gps_time[0] != '\0')
        {
            OLED_ShowStringLimited(4, 4, t->gps_time, 10);
        }
        else
        {
            OLED_ShowString(4, 4, "--");
        }
    }
    else
    {
        OLED_ShowString(1, 1, "ID:");
        OLED_ShowString(1, 5, t->vehicle_id);
        OLED_ShowString(2, 1, "Warehouse:");
        OLED_ShowNum(2, 12, g_system_state.storage.current_warehouse, 1);
        OLED_ShowString(3, 1, "T:");
        OLED_ShowFloat(3, 3, t->temperature, 2, 1);
        OLED_ShowChar(3, 8, 'C');
        OLED_ShowString(3, 11, "H:");
        OLED_ShowFloat(3, 13, t->humidity, 2, 0);
        OLED_ShowChar(3, 16, '%');
        if (!t->sensor_ready)
        {
            OLED_ShowString(4, 1, "DHT11 FAIL");
        }
        else
        {
            OLED_ShowString(4, 1, "GPS in 3 sec");
        }
    }
}

static void OLED_ClearTransportPage(void)
{
    OLED_ClearLine(1);
    OLED_ClearLine(2);
    OLED_ClearLine(3);
    OLED_ClearLine(4);
}

static void OLED_ShowPage_Alarm(void)
{
    Alarm_State_t *a = &g_system_state.alarm;

    OLED_ShowString(1, 1, "!!!! ALARM !!!!!");

    switch (a->alarm_type)
    {
        case ALARM_TEMP_HIGH:
            OLED_ShowString(2, 1, " TEMP HIGH!     ");
            break;
        case ALARM_TEMP_LOW:
            OLED_ShowString(2, 1, " TEMP LOW!      ");
            break;
        case ALARM_HUMIDITY_HIGH:
            OLED_ShowString(2, 1, " HUMIDITY HIGH! ");
            break;
        case ALARM_HUMIDITY_LOW:
            OLED_ShowString(2, 1, " HUMIDITY LOW!  ");
            break;
        case ALARM_GPS_LOST:
            OLED_ShowString(2, 1, " GPS LOST!      ");
            break;
        default:
            OLED_ShowString(2, 1, " UNKNOWN ALARM  ");
            break;
    }

    OLED_ShowString(3, 1, "Value: ");
    OLED_ShowFloat(3, 8, a->alarm_value, 2, 1);

    OLED_ShowString(4, 1, "Report: ");
    if (a->reported)
    {
        OLED_ShowString(4, 9, "[YES]    ");
    }
    else
    {
        OLED_ShowString(4, 9, "[NO]     ");
    }
}

static void OLED_ShowPage_Checkout(void)
{
    Checkout_State_t *c = &g_system_state.checkout;

    OLED_ShowString(1, 1, "=== CHECKOUT ===");
    OLED_ShowString(2, 1, "Card:");
    OLED_ShowHexBytes(2, 6, c->checkout_card_id, 4);

    OLED_ShowString(3, 1, "Warehouse:");
    OLED_ShowNum(3, 11, c->checkout_warehouse, 1);

    OLED_ShowString(4, 1, "Remain:");
    OLED_ShowNum(4, 8, c->remaining_count, 3);
}

void OLED_Display_Init(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "System Ready...");
}

void OLED_Display_Update(void)
{
    if (g_system_state.page_changed || last_page != g_system_state.current_page)
    {
        if (!(last_page == PAGE_TRANSPORT &&
              g_system_state.current_page == PAGE_TRANSPORT))
        {
            OLED_Clear();
        }
        last_page = g_system_state.current_page;
        g_system_state.page_changed = 0;
    }

    if (g_system_state.alarm.active && g_system_state.current_page != PAGE_ALARM)
    {
        OLED_Clear();
        last_page = PAGE_ALARM;
        g_system_state.current_page = PAGE_ALARM;
    }

    switch (g_system_state.current_page)
    {
        case PAGE_STANDBY:
            OLED_ShowPage_Standby();
            break;
        case PAGE_TRANSPORT:
            OLED_ShowPage_Transport();
            break;
        case PAGE_ALARM:
            OLED_ShowPage_Alarm();
            break;
        case PAGE_CHECKOUT:
            OLED_ShowPage_Checkout();
            break;
        default:
            OLED_ShowPage_Standby();
            break;
    }
}

void OLED_Display_NextPage(void)
{
    OLED_Page_t next;

    if (g_system_state.alarm.active)
    {
        return;
    }

    next = (OLED_Page_t)(g_system_state.current_page + 1);
    if (next > PAGE_CHECKOUT)
    {
        next = PAGE_STANDBY;
    }
    System_State_SetPage(next);
}

void OLED_Display_PrevPage(void)
{
    OLED_Page_t prev;

    if (g_system_state.alarm.active)
    {
        return;
    }

    if (g_system_state.current_page == PAGE_STANDBY)
    {
        prev = PAGE_CHECKOUT;
    }
    else
    {
        prev = (OLED_Page_t)(g_system_state.current_page - 1);
    }
    System_State_SetPage(prev);
}

void OLED_Display_Task(void *pvParameters)
{
    (void)pvParameters;

    OLED_Display_Init();
    vTaskDelay(pdMS_TO_TICKS(500));
    OLED_Clear();

    Usart1_Printf("[OLED] Task started\r\n");

    while (1)
    {
        OLED_Display_Update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
