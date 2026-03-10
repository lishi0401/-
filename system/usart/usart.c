
// Hardware library
#include "usart.h"

// C library
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

extern void USART3_IRQHandler_LoRa(void);
/*
************************************************************
*	Function Name:	Usart1_Init
*
*	Function:		USART1 initialization
*
*	Parameters:		baud: Set baud rate
*
*	Return:			None
*
*	Note:			TX-PA9		RX-PA10
************************************************************
*/
void Usart1_Init(unsigned int baud)
{

	GPIO_InitTypeDef gpio_initstruct;
	USART_InitTypeDef usart_initstruct;
	NVIC_InitTypeDef nvic_initstruct;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	//PA9	TXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_9;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);
	
	//PA10	RXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_10;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);
	
	usart_initstruct.USART_BaudRate = baud;
	usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;		// No hardware flow control
	usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;						// Receive and transmit
	usart_initstruct.USART_Parity = USART_Parity_No;									// No parity
	usart_initstruct.USART_StopBits = USART_StopBits_1;								// 1 stop bit
	usart_initstruct.USART_WordLength = USART_WordLength_8b;							// 8 data bits
	USART_Init(USART1, &usart_initstruct);
	
	USART_Cmd(USART1, ENABLE);														// Enable USART
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);									// Enable receive interrupt
	
	nvic_initstruct.NVIC_IRQChannel = USART1_IRQn;
	nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
	nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 0;
	nvic_initstruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&nvic_initstruct);

}

/*
************************************************************
*	Function Name:	Usart2_Init
*
*	Function:		USART2 initialization
*
*	Parameters:		baud: Set baud rate
*
*	Return:			None
*
*	Note:			TX-PA2		RX-PA3
************************************************************
*/
void Usart2_Init(unsigned int baud)
{

	GPIO_InitTypeDef gpio_initstruct;
	USART_InitTypeDef usart_initstruct;
	NVIC_InitTypeDef nvic_initstruct;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	//PA2	TXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_2;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);
	
	//PA3	RXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_3;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);
	
	usart_initstruct.USART_BaudRate = baud;
	usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;		// No hardware flow control
	usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;						// Receive and transmit
	usart_initstruct.USART_Parity = USART_Parity_No;									// No parity
	usart_initstruct.USART_StopBits = USART_StopBits_1;								// 1 stop bit
	usart_initstruct.USART_WordLength = USART_WordLength_8b;							// 8 data bits
	USART_Init(USART2, &usart_initstruct);
	
	USART_Cmd(USART2, ENABLE);														// Enable USART
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);									// Enable receive interrupt
	
	nvic_initstruct.NVIC_IRQChannel = USART2_IRQn;
	nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
	nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 0;
	nvic_initstruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic_initstruct);

}

/*
************************************************************
*	Function Name:	Usart_SendString
*
*	Function:		Send data string
*
*	Parameters:		USARTx: USART peripheral
*				str: Data to send
*				len: Data length
*
*	Return:			None
*
*	Note:			
************************************************************
*/
void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len)
{

	unsigned short count = 0;
	
	for(; count < len; count++)
	{
		USART_SendData(USARTx, *str++);									// Send data
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);		// Wait for transmission complete
	}

}


/*
************************************************************
*	Function Name:	UsartPrintf
*
*	Function:		Formatted print
*
*	Parameters:		USARTx: USART peripheral
*				fmt: Format string
*
*	Return:			None
*
*	Note:			
************************************************************
*/
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...)
{

	unsigned char UsartPrintfBuf[296];
	va_list ap;
	unsigned char *pStr = UsartPrintfBuf;
	
	va_start(ap, fmt);
	vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);							// Format string
	va_end(ap);
	
	while(*pStr != 0)
	{
		USART_SendData(USARTx, *pStr++);
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
	}
}

/*
************************************************************
*	Function Name:	DEBUG_LOG
*
*	Function:		Formatted debug information print
*
*	Parameters:		fmt: Format string

*	Return:			None
*
*	Note:			
************************************************************
*/
void DEBUG_LOG(char *fmt,...) // Format string function
{
	unsigned char UsartPrintfBuf[296]; // Formatted print buffer
	va_list ap;
	unsigned char *pStr = UsartPrintfBuf; // Print pointer
	
	va_start(ap, fmt);
	vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);							// Format string
	va_end(ap);
	UsartPrintf(USART_DEBUG, "[LOG] /> ");
	while(*pStr != 0)
	{
		USART_SendData(USART_DEBUG, *pStr++);
		while(USART_GetFlagStatus(USART_DEBUG, USART_FLAG_TC) == RESET);
	}
	UsartPrintf(USART_DEBUG, "\r\n");
}

/*
************************************************************
*	Function Name:	USART1_IRQHandler
*
*	Function:		USART1 receive interrupt
*
*	Parameters:		None
*
*	Return:			None
*
*	Note:			
************************************************************
*/
void USART1_IRQHandler(void)
{

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) // Receive interrupt
	{
		USART_ClearFlag(USART1, USART_FLAG_RXNE);
	}

}
#if !USE_OLED_DISPLAY
/*
************************************************************
*	Function Name:	Usart3_Init
*
*	Function:		USART3 initialization
*
*	Parameters:		baud: Set baud rate
*
*	Return:			None
*
*	Note:			TX-PB10		RX-PB11
*				Note: PB10/PB11 are multiplexed with OLED I2C pins
*				Auto enabled when USE_OLED_DISPLAY is 0
************************************************************
*/
void Usart3_Init(unsigned int baud)
{
	GPIO_InitTypeDef gpio_initstruct;
	USART_InitTypeDef usart_initstruct;
	NVIC_InitTypeDef nvic_initstruct;
	
	// Enable GPIOB clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	// Enable USART3 clock (USART3 is on APB1 bus)
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	// PB10 - TX (Alternate function push-pull output)
	gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_10;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_initstruct);
	
	// PB11 - RX (Pull-up input for better noise immunity)
	gpio_initstruct.GPIO_Mode = GPIO_Mode_IPU; // Pull-up input instead of floating
	gpio_initstruct.GPIO_Pin = GPIO_Pin_11;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_initstruct);
	
	usart_initstruct.USART_BaudRate = baud;
	usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart_initstruct.USART_Parity = USART_Parity_No;
	usart_initstruct.USART_StopBits = USART_StopBits_1;
	usart_initstruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3, &usart_initstruct);
	
	// Clear any pending flags before enabling
	USART_ClearFlag(USART3, USART_FLAG_RXNE | USART_FLAG_ORE);
	
	USART_Cmd(USART3, ENABLE);
	
	// Enable receive interrupt BEFORE enabling USART (some STM32 versions require this order)
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	
	// Verify interrupt is enabled
	if((USART3->CR1 & 0x0020) == 0) // USART_CR1_RXNEIE = 0x0020
	{
		// If not enabled, try again
		USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	}
	
	nvic_initstruct.NVIC_IRQChannel = USART3_IRQn;
	nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
	nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 0;
	nvic_initstruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&nvic_initstruct);
	
	// Final verification
	if((USART3->CR1 & 0x0020) != 0)
	{
		// Interrupt is enabled
	}
	else
	{
		// Still not enabled - this is a problem
	}
}

/*
************************************************************
*	Function Name:	USART3_IRQHandler
*
*	Function:		USART3 receive/transmit interrupt
*
*	Parameters:		None
*
*	Return:			None
*
*	Note:			
************************************************************
*/
void USART3_IRQHandler(void)
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		USART3_IRQHandler_LoRa();
	}
}
#endif // !USE_OLED_DISPLAY
