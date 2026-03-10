 #ifndef __SYSTEM_STATE_H
#define __SYSTEM_STATE_H

#include "stm32f10x.h"

#define MAX_STORAGE_COUNT   100
#define CARD_ID_LEN         5
#define VEHICLE_ID_LEN      20

typedef enum {
    PAGE_STANDBY = 0,
    PAGE_TRANSPORT,
    PAGE_ALARM,
    PAGE_CHECKOUT
} OLED_Page_t;

typedef enum {
    ALARM_NONE = 0,
    ALARM_TEMP_HIGH,
    ALARM_TEMP_LOW,
    ALARM_HUMIDITY_HIGH,
    ALARM_HUMIDITY_LOW,
    ALARM_GPS_LOST
} Alarm_Type_t;

typedef enum {
    CARD_STATUS_NONE = 0,
    CARD_STATUS_NEW,
    CARD_STATUS_DUPLICATE,
    CARD_STATUS_REMOVED
} Card_Status_t;

typedef enum {
    STORAGE_UI_WAITING = 0,
    STORAGE_UI_CARD_DETECTED,
    STORAGE_UI_DUPLICATE
} Storage_UI_State_t;

typedef struct {
    uint8_t current_warehouse;
    uint8_t stored_count;
    uint8_t card_id[CARD_ID_LEN];
    uint8_t pending_card_id[CARD_ID_LEN];
    uint8_t card_present;
    uint8_t enrollment_active;
    Card_Status_t last_card_status;
    uint8_t stored_cards[MAX_STORAGE_COUNT][CARD_ID_LEN];
    uint8_t stored_warehouse_bits[(MAX_STORAGE_COUNT + 3) / 4];
    uint8_t pending_warehouse;
    Storage_UI_State_t ui_state;
} Storage_State_t;

typedef struct {
    float temperature;
    float humidity;
    uint8_t sensor_ready;
    uint8_t gps_valid;
    uint8_t show_gps_page;
    char gps_longitude[15];
    char gps_latitude[15];
    char gps_time[10];
    char vehicle_id[VEHICLE_ID_LEN];
} Transport_State_t;

typedef struct {
    Alarm_Type_t alarm_type;
    float alarm_value;
    uint8_t reported;
    uint8_t active;
} Alarm_State_t;

typedef struct {
    uint8_t checkout_card_id[CARD_ID_LEN];
    uint8_t checkout_warehouse;
    uint8_t remaining_count;
    uint8_t checkout_active;
} Checkout_State_t;

typedef struct {
    OLED_Page_t current_page;
    OLED_Page_t previous_page;
    Storage_State_t storage;
    Transport_State_t transport;
    Alarm_State_t alarm;
    Checkout_State_t checkout;
    uint8_t page_changed;
} System_State_t;

extern volatile System_State_t g_system_state;

void System_State_Init(void);
void System_State_SetPage(OLED_Page_t page);
void System_State_SetAlarm(Alarm_Type_t type, float value);
void System_State_ClearAlarm(void);
void System_State_SetCardID(uint8_t *id);
void System_State_SetGPS(float temp, float humi, uint8_t valid, char *lon, char *lat, char *time);

uint8_t System_State_AddCard(uint8_t *id, uint8_t warehouse);
uint8_t System_State_CheckCardExists(uint8_t *id);
void System_State_ClearAllCards(void);
void System_State_SetWarehouse(uint8_t warehouse);
void System_State_SetPendingCard(uint8_t *id);
void System_State_ClearPendingCard(void);
void System_State_SetStorageUIState(Storage_UI_State_t state);
void System_State_SetTransportTH(float temp, float humi);
void System_State_SetEnrollmentActive(uint8_t active);
void System_State_UpdateTripID(void);
void System_State_SetTransportView(uint8_t show_gps);

#endif
