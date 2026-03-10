#include "System_State.h"
#include <string.h>

System_State_t g_system_state;

static void System_State_SaveWarehouse(uint8_t index, uint8_t warehouse)
{
    uint8_t byte_index;
    uint8_t bit_shift;
    uint8_t encoded;

    if (index >= MAX_STORAGE_COUNT || warehouse < 1 || warehouse > 4)
    {
        return;
    }

    byte_index = index / 4;
    bit_shift = (index % 4) * 2;
    encoded = (uint8_t)(warehouse - 1);

    g_system_state.storage.stored_warehouse_bits[byte_index] &=
        (uint8_t)~(0x03U << bit_shift);
    g_system_state.storage.stored_warehouse_bits[byte_index] |=
        (uint8_t)(encoded << bit_shift);
}

void System_State_UpdateTripID(void)
{
    uint16_t hash;
    uint8_t i;
    uint8_t j;

    hash = 0x1357U;
    for (i = 0; i < g_system_state.storage.stored_count; i++)
    {
        for (j = 0; j < CARD_ID_LEN; j++)
        {
            hash = (uint16_t)((hash << 5) - hash + g_system_state.storage.stored_cards[i][j]);
        }
        hash = (uint16_t)((hash << 5) - hash +
            g_system_state.storage.stored_warehouse_bits[i / 4] + i);
    }

    if (g_system_state.storage.stored_count == 0)
    {
        strcpy(g_system_state.transport.vehicle_id, "TR0000-00");
    }
    else
    {
        sprintf(g_system_state.transport.vehicle_id,
                "TR%04X-%02d",
                hash,
                g_system_state.storage.stored_count);
    }
    g_system_state.page_changed = 1;
}

void System_State_Init(void)
{
    memset(&g_system_state, 0, sizeof(System_State_t));
    
    g_system_state.current_page = PAGE_STANDBY;
    g_system_state.previous_page = PAGE_STANDBY;
    g_system_state.storage.current_warehouse = 1;
    g_system_state.transport.temperature = 0.0f;
    g_system_state.transport.humidity = 0.0f;
    g_system_state.transport.sensor_ready = 0;
    g_system_state.transport.gps_valid = 0;
    g_system_state.transport.show_gps_page = 0;
    strcpy(g_system_state.transport.vehicle_id, "VH-001");
    g_system_state.page_changed = 1;
    g_system_state.storage.last_card_status = CARD_STATUS_NONE;
    g_system_state.storage.pending_warehouse = 1;
    g_system_state.storage.ui_state = STORAGE_UI_WAITING;
    g_system_state.storage.enrollment_active = 1;
    System_State_UpdateTripID();
}

void System_State_SetPage(OLED_Page_t page)
{
    if (g_system_state.current_page != page)
    {
        g_system_state.previous_page = g_system_state.current_page;
        g_system_state.current_page = page;
        g_system_state.page_changed = 1;
    }
}

void System_State_SetAlarm(Alarm_Type_t type, float value)
{
    g_system_state.alarm.alarm_type = type;
    g_system_state.alarm.alarm_value = value;
    g_system_state.alarm.active = 1;
    g_system_state.alarm.reported = 0;
    
    if (g_system_state.current_page != PAGE_ALARM)
    {
        g_system_state.previous_page = g_system_state.current_page;
        g_system_state.current_page = PAGE_ALARM;
    }
    g_system_state.page_changed = 1;
}

void System_State_ClearAlarm(void)
{
    g_system_state.alarm.active = 0;
    g_system_state.alarm.alarm_type = ALARM_NONE;
    
    if (g_system_state.current_page == PAGE_ALARM)
    {
        g_system_state.current_page = g_system_state.previous_page;
    }
    g_system_state.page_changed = 1;
}

void System_State_SetCardID(uint8_t *id)
{
    if (id != NULL)
    {
        memcpy(g_system_state.storage.card_id, id, CARD_ID_LEN);
        g_system_state.storage.card_present = 1;
    }
    else
    {
        memset(g_system_state.storage.card_id, 0, CARD_ID_LEN);
        g_system_state.storage.card_present = 0;
    }
}

void System_State_SetGPS(float temp, float humi, uint8_t valid, char *lon, char *lat, char *time)
{
    g_system_state.transport.temperature = temp;
    g_system_state.transport.humidity = humi;
    g_system_state.transport.gps_valid = valid;
    
    if (lon != NULL)
        strncpy(g_system_state.transport.gps_longitude, lon, 14);
    if (lat != NULL)
        strncpy(g_system_state.transport.gps_latitude, lat, 14);
    if (time != NULL)
        strncpy(g_system_state.transport.gps_time, time, 9);
    g_system_state.transport.gps_longitude[14] = '\0';
    g_system_state.transport.gps_latitude[14] = '\0';
    g_system_state.transport.gps_time[9] = '\0';
}

void System_State_SetPendingCard(uint8_t *id)
{
    if (id != NULL)
    {
        memcpy(g_system_state.storage.pending_card_id, id, CARD_ID_LEN);
    }
    else
    {
        memset(g_system_state.storage.pending_card_id, 0, CARD_ID_LEN);
    }
    g_system_state.page_changed = 1;
}

void System_State_ClearPendingCard(void)
{
    memset(g_system_state.storage.pending_card_id, 0, CARD_ID_LEN);
    g_system_state.page_changed = 1;
}

void System_State_SetStorageUIState(Storage_UI_State_t state)
{
    if (g_system_state.storage.ui_state != state)
    {
        g_system_state.storage.ui_state = state;
        g_system_state.page_changed = 1;
    }
}

void System_State_SetTransportTH(float temp, float humi)
{
    g_system_state.transport.temperature = temp;
    g_system_state.transport.humidity = humi;
    g_system_state.transport.sensor_ready = 1;
}

void System_State_SetTransportView(uint8_t show_gps)
{
    show_gps = show_gps ? 1U : 0U;
    if (g_system_state.transport.show_gps_page != show_gps)
    {
        g_system_state.transport.show_gps_page = show_gps;
        g_system_state.page_changed = 1;
    }
}

void System_State_SetEnrollmentActive(uint8_t active)
{
    g_system_state.storage.enrollment_active = active ? 1U : 0U;
    g_system_state.page_changed = 1;
}

uint8_t System_State_CheckCardExists(uint8_t *id)
{
    uint8_t i, j;
    
    if (id == NULL || g_system_state.storage.stored_count == 0)
        return 0;
    
    for (i = 0; i < g_system_state.storage.stored_count; i++)
    {
        uint8_t match = 1;
        for (j = 0; j < CARD_ID_LEN; j++)
        {
            if (g_system_state.storage.stored_cards[i][j] != id[j])
            {
                match = 0;
                break;
            }
        }
        if (match)
            return 1;
    }
    
    return 0;
}

uint8_t System_State_AddCard(uint8_t *id, uint8_t warehouse)
{
    if (id == NULL)
        return 0;
    
    if (g_system_state.storage.stored_count >= MAX_STORAGE_COUNT)
        return 0;
    
    if (System_State_CheckCardExists(id))
    {
        memcpy(g_system_state.storage.card_id, id, CARD_ID_LEN);
        g_system_state.storage.card_present = 1;
        g_system_state.storage.last_card_status = CARD_STATUS_DUPLICATE;
        return 0;
    }
    
    memcpy(g_system_state.storage.stored_cards[g_system_state.storage.stored_count], id, CARD_ID_LEN);
    System_State_SaveWarehouse(g_system_state.storage.stored_count, warehouse);
    g_system_state.storage.stored_count++;
    
    memcpy(g_system_state.storage.card_id, id, CARD_ID_LEN);
    g_system_state.storage.card_present = 1;
    g_system_state.storage.current_warehouse = warehouse;
    g_system_state.storage.last_card_status = CARD_STATUS_NEW;
    System_State_UpdateTripID();
    
    return 1;
}

void System_State_ClearAllCards(void)
{
    g_system_state.storage.stored_count = 0;
    g_system_state.storage.card_present = 0;
    g_system_state.storage.last_card_status = CARD_STATUS_NONE;
    memset(g_system_state.storage.stored_cards, 0, sizeof(g_system_state.storage.stored_cards));
    memset(g_system_state.storage.stored_warehouse_bits, 0, sizeof(g_system_state.storage.stored_warehouse_bits));
    memset(g_system_state.storage.card_id, 0, CARD_ID_LEN);
    memset(g_system_state.storage.pending_card_id, 0, CARD_ID_LEN);
    g_system_state.storage.pending_warehouse = 1;
    g_system_state.storage.ui_state = STORAGE_UI_WAITING;
    g_system_state.storage.enrollment_active = 1;
    System_State_UpdateTripID();
}

void System_State_SetWarehouse(uint8_t warehouse)
{
    if (warehouse >= 1 && warehouse <= 4)
    {
        g_system_state.storage.current_warehouse = warehouse;
        g_system_state.storage.pending_warehouse = warehouse;
        g_system_state.page_changed = 1;
    }
}
