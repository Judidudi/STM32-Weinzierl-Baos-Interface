#pragma once
#include "stm32f4xx.h"
#include <stdint.h>



////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "config.h"																// Global configuration
#include "StdDefs.h"															// General defines and project settings


////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

/* ==== STM32/USART3 Makro-Ersatz für die alten SERCOM-Makros ==== */

/* IRQs der KNX-UART (USART3) steuern */
#define DISABLE_KNX_SERIAL_INT  do { USART3->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_RXNEIE); } while(0)
#define ENABLE_KNX_SERIAL_INT   do { USART3->CR1 |=  (USART_CR1_TXEIE | USART_CR1_RXNEIE); } while(0)

/* TX-Complete-Flag löschen – auf STM32 nicht nötig */
#define CLEAR_KNX_THE_INT       do { /* no-op on STM32 */ } while(0)

/* Statusflags */
#define IS_KNX_THE              ((USART3->SR & USART_SR_TXE)  != 0)   /* TX data register empty */
#define IS_KNX_RDA              ((USART3->SR & USART_SR_RXNE) != 0)   /* RX data register not empty */

/* Empfangsbyte (liest DR, räumt RXNE) */
#define KNX_RCV_CHAR            ((uint8_t)USART3->DR)







/* ==== UART-API, die vom BAOS/FT1.2-Stack aufgerufen wird ==== */
uint32_t get_pclk(void);
void KnxSer_Init(void);                 // setzt PC10/PC11 auf AF7, USART3 @ 19200 8E1 + IRQ
void KnxSer_TransmitChar(uint8_t nChar);

