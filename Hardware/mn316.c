#include "mn316.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Usart.h"
#include <string.h>
#include <stdio.h>

#ifndef USE_WIFI
#define USE_WIFI 0
#endif

static unsigned char mn316_rx_buf[MN316_RX_MAX];
static unsigned short mn316_cnt = 0;
static unsigned short mn316_cntPre = 0;

static char mn316_cmd_buf[768];
static char mn316_topic_buf[128];
static char mn316_payload_buf[512];

uint8_t ESP8266_INIT_OK = 0;

void MN316_Clear(void)
{
	memset(mn316_rx_buf, 0, sizeof(mn316_rx_buf));
	mn316_cnt = 0;
	mn316_cntPre = 0;
}

_Bool MN316_WaitRecive(void)
{
	if(mn316_cnt == 0)
		return REV_WAIT;
		
	if(mn316_cnt == mn316_cntPre)
	{
		return REV_OK;
	}
		
	mn316_cntPre = mn316_cnt;
	return REV_WAIT;
}

_Bool MN316_SendCmdWithTimeout(char *cmd, char *res, unsigned short timeOut)
{
	MN316_Clear();
	Usart_SendString(MN316_USART, (unsigned char *)cmd, strlen((const char *)cmd));
	vTaskDelay(pdMS_TO_TICKS(50));
	
	while(timeOut--)
	{
		if(mn316_cnt > 0)
		{
			if(mn316_cnt < MN316_RX_MAX)
				mn316_rx_buf[mn316_cnt] = '\0';
		DEBUG_LOG("MN316 Response: %s (len: %d)", mn316_rx_buf, mn316_cnt);
			if(strstr((const char *)mn316_rx_buf, res) != NULL)
			{
				MN316_Clear();
				return 0;
			}
			if(strstr((const char *)mn316_rx_buf, "ERROR") != NULL)
				break;
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	if(mn316_cnt > 0 && mn316_cnt < MN316_RX_MAX)
	{
		mn316_rx_buf[mn316_cnt] = '\0';
		
	}
	
	return 1;
}

_Bool MN316_SendCmd(char *cmd, char *res)
{
	return MN316_SendCmdWithTimeout(cmd, res, 200);
}

static _Bool MN316_WaitModuleReady(unsigned short timeout_ms)
{
	unsigned short elapsed = 0;
	
	MN316_Clear();
	DEBUG_LOG("MN316_WaitModuleReady: Waiting for module ready...");
	
	while(elapsed < timeout_ms)
	{
		if(mn316_cnt > 0)
		{
			if(mn316_cnt < MN316_RX_MAX)
				mn316_rx_buf[mn316_cnt] = '\0';
			
			if(strstr((char *)mn316_rx_buf, "MODULE_READY") != NULL ||
			   strstr((char *)mn316_rx_buf, "^SIMST:1") != NULL)
			{
				DEBUG_LOG("MN316_WaitModuleReady: Module is ready, URC: %s", mn316_rx_buf);
				MN316_Clear();
				return 0;
			}
		}
		
		vTaskDelay(pdMS_TO_TICKS(100));
		elapsed += 100;
	}
	
	DEBUG_LOG("MN316_WaitModuleReady: Timeout, module may not be ready yet");
	return 1;
}

static unsigned short MN316_MqttDecodeRemainingLength(const unsigned char *data, unsigned short len, unsigned short *remain_len)
{
	unsigned short multiplier = 1;
	unsigned short value = 0;
	unsigned short i = 0;

	for(i = 1; i < len && i <= 4; i++)
	{
		unsigned char encoded = data[i];
		value += (encoded & 0x7F) * multiplier;
		if((encoded & 0x80) == 0)
		{
			*remain_len = value;
			return i;
		}
		multiplier *= 128;
	}

	return 0;
}

void MN316_SendData(unsigned char *data, unsigned short len)
{
	unsigned short i;
	unsigned short remain_len = 0;
	unsigned short rl_last_index = 0;
	unsigned short vh_start = 0;
	unsigned short topic_len = 0;
	unsigned short payload_start = 0;
	unsigned short payload_len = 0;
	unsigned char qos = 0;
	
	if(len < 5) return;
	
	if((data[0] & 0xF0) != 0x30) return;

	rl_last_index = MN316_MqttDecodeRemainingLength(data, len, &remain_len);
	if(rl_last_index == 0) return;
	vh_start = rl_last_index + 1;
	if(vh_start + 2 > len) return;

	qos = (data[0] & 0x06) >> 1;

	topic_len = ((unsigned short)data[vh_start] << 8) | data[vh_start + 1];
	if(topic_len == 0 || topic_len > 127) return;
	if(vh_start + 2 + topic_len > len) return;

	for(i = 0; i < topic_len && i < 127; i++)
	{
		mn316_topic_buf[i] = data[vh_start + 2 + i];
	}
	mn316_topic_buf[i] = '\0';

	payload_start = vh_start + 2 + topic_len;
	if(qos != 0)
	{
		if(payload_start + 2 > len) return;
		payload_start += 2;
	}
	if(payload_start > len) return;

	payload_len = len - payload_start;
	if(payload_len > 511) payload_len = 511;

	{
		unsigned short j = 0;
		for(i = 0; i < payload_len && j < 500; i++)
		{
			unsigned char c = data[payload_start + i];
			if(c >= 32 && c < 127 && c != '"' && c != '\\')
			{
				mn316_payload_buf[j++] = c;
			}
			else if(c == '"')
			{
				mn316_payload_buf[j++] = '\\';
				mn316_payload_buf[j++] = '"';
			}
			else if(c == '\\')
			{
				mn316_payload_buf[j++] = '\\';
				mn316_payload_buf[j++] = '\\';
			}
			else
			{
				sprintf(&mn316_payload_buf[j], "\\x%02X", c);
				j += 4;
			}
		}
		mn316_payload_buf[j] = '\0';
	}

	MN316_Clear();
	sprintf(mn316_cmd_buf, "AT+MQTTPUB=\"%s\",0,0,0,0,\"%s\"\r\n", mn316_topic_buf, mn316_payload_buf);
	if(!MN316_SendCmd(mn316_cmd_buf, "OK"))
		vTaskDelay(pdMS_TO_TICKS(100));
}

unsigned char *MN316_GetIPD(unsigned short timeOut)
{
	char *ptrIPD = NULL;
	unsigned char *result = NULL;
	
	do
	{
		if(MN316_WaitRecive() == REV_OK)
		{
			DEBUG_LOG("MN316_GetIPD: Received raw data: %s", mn316_rx_buf);
			
			ptrIPD = strstr((char *)mn316_rx_buf, "+MQTTPUBLISH:");
			if(ptrIPD != NULL)
			{
				char *json_start = strchr(ptrIPD, '{');
				if(json_start != NULL)
				{
					char *json_end = strrchr(json_start, '}');
					if(json_end != NULL)
					{
						static char temp_json[512];
						int json_len = json_end - json_start + 1;
						if(json_len < sizeof(temp_json))
						{
							memcpy(temp_json, json_start, json_len);
							temp_json[json_len] = '\0';
							DEBUG_LOG("MN316_GetIPD: Found complete JSON: %s", temp_json);
							MN316_Clear();
							return (unsigned char *)temp_json;
						}
					}
				}
			}
			else if(strstr((char *)mn316_rx_buf, "+MQTTSUBACK:") != NULL)
			{
				DEBUG_LOG("MN316_GetIPD: Received MQTT SUBACK");
				MN316_Clear();
			}
			else if(strstr((char *)mn316_rx_buf, "OK") != NULL)
			{
				DEBUG_LOG("MN316_GetIPD: Received OK response");
				MN316_Clear();
			}
			else if(strstr((char *)mn316_rx_buf, "ERROR") != NULL)
			{
				DEBUG_LOG("MN316_GetIPD: Received ERROR response");
				MN316_Clear();
			}
			else if(mn316_cnt > 0)
			{
				DEBUG_LOG("MN316_GetIPD: Received unknown data, clearing buffer");
				MN316_Clear();
			}
		}
		vTaskDelay(pdMS_TO_TICKS(5));
		timeOut--;
	} while(timeOut > 0);
	
	return NULL;
}

void MN316_Init(void)
{
	MN316_Clear();
	
	DEBUG_LOG("0. AT - Test MCU-MN316 communication");
	
	DEBUG_LOG("Sending: AT\\r\\n");
	
	unsigned char attempt = 0;
	for(attempt = 0; attempt < 10; attempt++)
	{
		if(MN316_SendCmdWithTimeout("AT\r\n", "OK", 500) == 0)
			break;

		DEBUG_LOG("MN316 AT command failed, retrying... (retry: %d)", attempt + 1);
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	if(attempt >= 10)
	{
		DEBUG_LOG("MN316 AT command failed after 10 retries");
		DEBUG_LOG("Please check:");
		DEBUG_LOG("1. MN316 module power supply");
		DEBUG_LOG("2. USART2 TX/RX connection");
		DEBUG_LOG("3. Baud rate (should be 9600)");
		return;
	}

	vTaskDelay(pdMS_TO_TICKS(1000));
	DEBUG_LOG("1. AT+MQTTCFG - Configure MQTT parameters");
	char cfgCmd[256];
	sprintf(cfgCmd, "AT+MQTTCFG=\"%s\",%s,\"%s\",60,\"\",\"\",1\r\n",
					SERVER_HOST, SERVER_PORT, CLIENT_ID);
	for(attempt = 0; attempt < 10; attempt++){
	if(MN316_SendCmdWithTimeout(cfgCmd, "OK", 1000))
		DEBUG_LOG("MN316 MQTTCFG faileds");
	else break;
	vTaskDelay(pdMS_TO_TICKS(3000*attempt));
}
	DEBUG_LOG("MN316 MQTTCFG success");
	vTaskDelay(pdMS_TO_TICKS(2000));
	DEBUG_LOG("2. AT+MQTTOPEN - Connect to MQTT server");
for(attempt = 0; attempt < 10; attempt++){
	if(MN316_SendCmdWithTimeout("AT+MQTTOPEN=0,0,0,0,0,\"\",\"\"\r\n", "OK", 1000)==0)
		break;
		DEBUG_LOG("MN316 MQTTOPEN failed");
		vTaskDelay(pdMS_TO_TICKS(attempt*3000));
	}
	DEBUG_LOG("MN316 MQTTOPEN success");
	vTaskDelay(pdMS_TO_TICKS(3000));
	DEBUG_LOG("3. AT+MQTTSUB - Subscribe to topic");
		char subCmd[128];
		sprintf(subCmd, "AT+MQTTSUB=\"/hxy_1150/sub\",1\r\n");
		for(attempt = 0; attempt < 10; attempt++){
			if(!MN316_SendCmdWithTimeout(subCmd, "OK", 300))
				break;
		DEBUG_LOG("MN316 MQTTSUB failed");
		DEBUG_LOG("MN316 AT command failed, retrying... (retry: %d)", attempt + 1);
		vTaskDelay(pdMS_TO_TICKS(3000));
	}
			
	DEBUG_LOG("MN316 MQTTSUB success");
	vTaskDelay(pdMS_TO_TICKS(500));
	ESP8266_INIT_OK = 1;
	DEBUG_LOG("4. MN316 Init OK - MN316 initialization success");
	DEBUG_LOG("MN316 initialization [OK]");
}

void USART2_IRQHandler_MN316(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		if(mn316_cnt >= sizeof(mn316_rx_buf)) mn316_cnt = 0;
		mn316_rx_buf[mn316_cnt++] = USART2->DR;
		
		USART_ClearFlag(USART2, USART_FLAG_RXNE);
	}
}
