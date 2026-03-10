#include "freertos_demo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Usart.h"
#include "LED.h"
#include "OLED.h"
#include "System_State.h"
#include "rc522.h"
#include "beep.h"
#include "Key.h"
#include "dht11.h"
#include "GPS.h"
#include <string.h>

/* 启动任务相关配置 */
void startTask(void *args);
#define START_TASK_NAME "start"
#define START_TASK_STACK_DEPTH 256
#define START_TASK_PRIORITY 1
TaskHandle_t startTaskTCB;

/* OLED显示任务 */
#define OLED_TASK_NAME "OLED_Display"
#define OLED_TASK_STACK_DEPTH 256
#define OLED_TASK_PRIORITY 2
TaskHandle_t oledTaskTCB;

/* RFID录入任务 */
void RFID_Task(void *args);
#define RFID_TASK_NAME "RFID_Scan"
#define RFID_TASK_STACK_DEPTH 256
#define RFID_TASK_PRIORITY 3
TaskHandle_t rfidTaskTCB;

void Transport_Task(void *args);
#define TRANSPORT_TASK_NAME "Transport"
#define TRANSPORT_TASK_STACK_DEPTH 256
#define TRANSPORT_TASK_PRIORITY 2
TaskHandle_t transportTaskTCB;


#define WAIT_BLINK_PERIOD_MS 300
#define SHORT_BEEP_MS 50
#define LONG_BEEP_MS 180
#define DUPLICATE_HINT_MS 1000
#define LONG_PRESS_MS 1500
#define SENSOR_REFRESH_MS 1000
#define GPS_SWITCH_MS 6000
#define TRANSPORT_TASK_PERIOD_MS 100
#define RFID_TASK_PERIOD_MS 50

static void Transport_ReadTempHumidity(float *temp, float *humi)
{
    uint8_t humi_h = 0;
    uint8_t humi_l = 0;
    uint8_t temp_h = 0;
    uint8_t temp_l = 0;

    if (temp == NULL || humi == NULL)
    {
        return;
    }

    if (DHT11_Read_Data(&humi_h, &humi_l, &temp_h, &temp_l) == 0)
    {
        *temp = (float)temp_h + ((float)temp_l / 10.0f);
        *humi = (float)humi_h + ((float)humi_l / 10.0f);
    }
    else
    {
        *temp = g_system_state.transport.temperature;
        *humi = g_system_state.transport.humidity;
    }
}

static void RFID_Beep(uint16_t on_ms)
{
    BEEP_On();
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    BEEP_Off();
}

static void RFID_BeepTwice(void)
{
    RFID_Beep(SHORT_BEEP_MS);
    vTaskDelay(pdMS_TO_TICKS(60));
    RFID_Beep(SHORT_BEEP_MS);
}


static void RFID_ShowWaitingState(void)
{
    System_State_SetStorageUIState(STORAGE_UI_WAITING);
    System_State_ClearPendingCard();
    LED_All_Off();
    LED_Yellow_On();
}

static void RFID_FinishEnrollment(void)
{
    RFID_Beep(LONG_BEEP_MS);
    System_State_SetEnrollmentActive(0);
    System_State_SetTransportView(0);
    System_State_UpdateTripID();
    System_State_SetPage(PAGE_TRANSPORT);
    LED_All_Off();
    LED_Green_On();
    Usart1_Printf("[RFID] Enrollment finished, Trip=%s\r\n",
        g_system_state.transport.vehicle_id);
}

static uint8_t RFID_CheckFinishEnrollmentRequest(void)
{
    uint16_t hold_ms = 0;

    if (KEY_Scan(0) != KEY2_PRES)
    {
        return 0;
    }

    while (KEY2 == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(RFID_TASK_PERIOD_MS));
        hold_ms += RFID_TASK_PERIOD_MS;
        if (hold_ms >= LONG_PRESS_MS)
        {
            return 1;
        }
    }

    return 0;
}

void FreeRTOS_Start(void)
{
    xTaskCreate(
        startTask,
        START_TASK_NAME,
        START_TASK_STACK_DEPTH,
        NULL,
        START_TASK_PRIORITY,
        &startTaskTCB
    );

    vTaskStartScheduler();
}

void startTask(void *args)
{
    vPortEnterCritical();      
    Usart1_Printf("[RTOS] Starting tasks...\r\n");         
    System_State_Init();

    xTaskCreate(
        OLED_Display_Task,
        OLED_TASK_NAME,
        OLED_TASK_STACK_DEPTH,
        NULL,
        OLED_TASK_PRIORITY,
        &oledTaskTCB
    );
    
    xTaskCreate(
        RFID_Task,
        RFID_TASK_NAME,
        RFID_TASK_STACK_DEPTH,
        NULL,
        RFID_TASK_PRIORITY,
        &rfidTaskTCB
    );

    xTaskCreate(
        Transport_Task,
        TRANSPORT_TASK_NAME,
        TRANSPORT_TASK_STACK_DEPTH,
        NULL,
        TRANSPORT_TASK_PRIORITY,
        &transportTaskTCB
    );
    
    Usart1_Printf("[RTOS] Tasks created\r\n");
    vPortExitCritical();

    vTaskDelete(NULL);
}

void RFID_Task(void *args)
{
    uint8_t card_id[5];
    uint8_t status;
    uint8_t last_card_id[5] = {0};
    uint8_t card_present = 0;
    TickType_t last_blink_tick = 0;
    
    (void)args;
    
    MFRC522_Init();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    RFID_ShowWaitingState();
    
    Usart1_Printf("[RFID] Task started\r\n");
    Usart1_Printf("[RFID] Waiting for card...\r\n");
    
    while (1)
    {
        if (!g_system_state.storage.enrollment_active)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (g_system_state.storage.ui_state == STORAGE_UI_WAITING)
        {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_blink_tick) >= pdMS_TO_TICKS(WAIT_BLINK_PERIOD_MS))
            {
                LED_Yellow_Toggle();
                last_blink_tick = now;
            }

            if (RFID_CheckFinishEnrollmentRequest())
            {
                memset(last_card_id, 0, sizeof(last_card_id));
                card_present = 0;
                System_State_SetCardID(NULL);
                RFID_FinishEnrollment();
                vTaskDelay(pdMS_TO_TICKS(300));
                continue;
            }
        }

        status = MFRC522_Check(card_id);
        
        if (status == MI_OK)
        {
            uint8_t same = 1;
            for (uint8_t i = 0; i < 5; i++)
            {
                if (card_id[i] != last_card_id[i])
                {
                    same = 0;
                    break;
                }
            }
            
            if (!same)
            {
                memcpy(last_card_id, card_id, 5);
                System_State_SetCardID(card_id);
                
                Usart1_Printf("[RFID] New card: %02X%02X%02X%02X%02X\r\n",
                    card_id[0], card_id[1], card_id[2], card_id[3], card_id[4]);
                
                if (System_State_CheckCardExists(card_id))
                {
                    LED_All_Off();
                    LED_Red_On();
                    System_State_SetStorageUIState(STORAGE_UI_DUPLICATE);
                    g_system_state.storage.last_card_status = CARD_STATUS_DUPLICATE;
                    Usart1_Printf("[RFID] Duplicate!\r\n");

                    RFID_BeepTwice();
                    vTaskDelay(pdMS_TO_TICKS(DUPLICATE_HINT_MS));
                    RFID_ShowWaitingState();
                }
                else
                {
                    LED_All_Off();
                    LED_Yellow_On();
                    System_State_SetPendingCard(card_id);
                    System_State_SetStorageUIState(STORAGE_UI_CARD_DETECTED);
                    System_State_SetWarehouse(g_system_state.storage.current_warehouse);

                    while (1)
                    {
                        uint8_t key = KEY_Scan(0);

                        if (key == KEY1_PRES)
                        {
                            uint8_t warehouse = g_system_state.storage.pending_warehouse;
                            if (warehouse > 1)
                            {
                                System_State_SetWarehouse(warehouse - 1);
                            }
                            RFID_Beep(SHORT_BEEP_MS);
                        }
                        else if (key == KEY3_PRES)
                        {
                            uint8_t warehouse = g_system_state.storage.pending_warehouse;
                            if (warehouse < 4)
                            {
                                System_State_SetWarehouse(warehouse + 1);
                            }
                            RFID_Beep(SHORT_BEEP_MS);
                        }
                        else if (key == KEY2_PRES)
                        {
                            RFID_Beep(LONG_BEEP_MS);
                            LED_All_Off();
                            LED_Green_On();

                            if (System_State_AddCard(card_id, g_system_state.storage.pending_warehouse))
                            {
                                Usart1_Printf("[RFID] Added! Warehouse: %d Total: %d\r\n",
                                    g_system_state.storage.current_warehouse,
                                    g_system_state.storage.stored_count);
                            }

                            vTaskDelay(pdMS_TO_TICKS(200));
                            RFID_ShowWaitingState();
                            break;
                        }

                        vTaskDelay(pdMS_TO_TICKS(20));
                    }
                }
                
                card_present = 1;
            }
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            if (card_present)
            {
                memset(last_card_id, 0, 5);
                card_present = 0;
                System_State_SetCardID(NULL);
                
                Usart1_Printf("[RFID] Card removed\r\n");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(RFID_TASK_PERIOD_MS));
    }
}

void Transport_Task(void *args)
{
    float temp;
    float humi;
    uint8_t dht_ready;
    TickType_t last_sensor_tick;
    TickType_t last_switch_tick;
    GPS_Data_t *gps;

    (void)args;

    GPS_Init();
    dht_ready = (uint8_t)(DHT11_Init() == 0);
    Usart1_Printf("[DHT11] Init %s\r\n", dht_ready ? "OK" : "FAIL");
    if (dht_ready)
    {
        Transport_ReadTempHumidity(&temp, &humi);
        System_State_SetTransportTH(temp, humi);
    }
    last_sensor_tick = xTaskGetTickCount();
    last_switch_tick = last_sensor_tick;

    while (1)
    {
        GPS_Process();
        gps = GPS_GetData();
        System_State_SetGPS(g_system_state.transport.temperature,
            g_system_state.transport.humidity,
            gps->valid,
            gps->longitude,
            gps->latitude,
            gps->time);

        if (!g_system_state.storage.enrollment_active)
        {
            if (!dht_ready)
            {
                dht_ready = (uint8_t)(DHT11_Init() == 0);
                Usart1_Printf("[DHT11] Retry %s\r\n", dht_ready ? "OK" : "FAIL");
            }

            if (g_system_state.current_page != PAGE_TRANSPORT)
            {
                System_State_SetPage(PAGE_TRANSPORT);
            }

            if ((xTaskGetTickCount() - last_switch_tick) >= pdMS_TO_TICKS(GPS_SWITCH_MS))
            {
                System_State_SetTransportView(!g_system_state.transport.show_gps_page);
                last_switch_tick = xTaskGetTickCount();
            }

            if ((xTaskGetTickCount() - last_sensor_tick) >= pdMS_TO_TICKS(SENSOR_REFRESH_MS))
            {
                Transport_ReadTempHumidity(&temp, &humi);
                if (dht_ready)
                {
                    System_State_SetTransportTH(temp, humi);
                    System_State_UpdateTripID();

                    Usart1_Printf("[ENV] Trip=%s T=%d.%dC H=%d.%d%%\r\n",
                        g_system_state.transport.vehicle_id,
                        (int)temp,
                        ((int)(temp * 10.0f)) % 10,
                        (int)humi,
                        ((int)(humi * 10.0f)) % 10);
                }
                else
                {
                    System_State_SetTransportTH(0.0f, 0.0f);
                    Usart1_Printf("[ENV] DHT11 read skipped\r\n");
                }
                last_sensor_tick = xTaskGetTickCount();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TRANSPORT_TASK_PERIOD_MS));
    }
}
