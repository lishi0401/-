#include "GPS.h"
#include "Usart.h"
#include <string.h>
#include <stdio.h>

#define GPS_BUFFER_SIZE 256

static char GPS_Buffer[GPS_BUFFER_SIZE];
static uint16_t GPS_BufferIndex = 0;
static uint8_t GPS_LogNoFixShown = 0;
static GPS_Data_t GPS_Data;

char *strtok_r(char *s, const char *delim, char **save_ptr)
{
    char *token;

    if (s == NULL)
    {
        s = *save_ptr;
    }

    s += strspn(s, delim);
    if (*s == '\0')
    {
        return NULL;
    }

    token = s;
    s += strcspn(token, delim);
    if (*s == '\0')
    {
        *save_ptr = s;
    }
    else
    {
        *s = '\0';
        *save_ptr = s + 1;
    }
    return token;
}

void GPS_Init(void)
{
    memset(&GPS_Data, 0, sizeof(GPS_Data));
    memset(GPS_Buffer, 0, sizeof(GPS_Buffer));
    GPS_BufferIndex = 0;
    GPS_LogNoFixShown = 0;
}

static void GPS_ParseRMC(char *line)
{
    char *token;
    char *saveptr = NULL;
    uint8_t field_index = 0;

    token = strtok_r(line, ",", &saveptr);
    while (token != NULL)
    {
        switch (field_index)
        {
            case 1:
                strncpy(GPS_Data.time, token, sizeof(GPS_Data.time) - 1);
                GPS_Data.time[sizeof(GPS_Data.time) - 1] = '\0';
                break;

            case 2:
                GPS_Data.valid = (uint8_t)(strcmp(token, "A") == 0);
                break;

            case 3:
                strncpy(GPS_Data.latitude, token, sizeof(GPS_Data.latitude) - 1);
                GPS_Data.latitude[sizeof(GPS_Data.latitude) - 1] = '\0';
                break;

            case 5:
                strncpy(GPS_Data.longitude, token, sizeof(GPS_Data.longitude) - 1);
                GPS_Data.longitude[sizeof(GPS_Data.longitude) - 1] = '\0';
                break;

            default:
                break;
        }

        token = strtok_r(NULL, ",", &saveptr);
        field_index++;
    }

    if (GPS_Data.valid)
    {
        GPS_LogNoFixShown = 0;
    }
    else if (!GPS_LogNoFixShown)
    {
        GPS_LogNoFixShown = 1;
    }
}

void GPS_Process(void)
{
    uint8_t data;

    while (Usart2_ReadByte(&data))
    {
        if (GPS_BufferIndex < (GPS_BUFFER_SIZE - 1))
        {
            GPS_Buffer[GPS_BufferIndex++] = (char)data;
            GPS_Buffer[GPS_BufferIndex] = '\0';
        }
        else
        {
            GPS_BufferIndex = 0;
            memset(GPS_Buffer, 0, sizeof(GPS_Buffer));
            continue;
        }

        if (data != '\n')
        {
            continue;
        }

        if (strstr(GPS_Buffer, "$GPRMC") != NULL || strstr(GPS_Buffer, "$GNRMC") != NULL)
        {
            GPS_ParseRMC(GPS_Buffer);
        }

        GPS_BufferIndex = 0;
        memset(GPS_Buffer, 0, sizeof(GPS_Buffer));
    }
}

GPS_Data_t* GPS_GetData(void)
{
    return &GPS_Data;
}
