/*
 * KnxTm_stm32.h
 *
 *  Created on: Aug 13, 2025
 *      Author: julianegle
 */

#ifndef KNXTM_STM32_H_
#define KNXTM_STM32_H_

//#ifndef KNXTM__H___INCLUDED
//#define KNXTM__H___INCLUDED


#pragma once
#include <stdint.h>
#include "StdDefs.h"   /* für bool_t */

void     KnxTm_Init(void);
uint32_t KnxTm_GetTimeMs(void);
uint32_t KnxTm_GetDelayMs(uint32_t nOldTime);
bool_t   KnxTm_IsExpiredMs(uint32_t nTimeOut);
void     KnxTm_SleepMs(uint32_t nDelay);

uint32_t KnxTm_GetTimeTicks(void);
uint32_t KnxTm_GetDelayTicks(uint32_t nOldTime);
bool_t   KnxTm_IsExpiredTicks(uint32_t nTimeOut);
void     KnxTm_SleepTicks(uint32_t nDelay);

void     KnxTm_SetSendTimer(uint32_t nTimeOut);
void     KnxTm_SetRcvTimer(uint32_t nTimeOut);


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "config.h"																// Global configuration
#include "StdDefs.h"															// General defines and project settings


////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

// General macros for timer 0
#define PFI_TM_TICKS_PER_MS			(FR_CLOCK/1000)								// Ticks per 1 ms (t_clk = sys_clk)
#define PFI_TM_MS_TIMER_INCREMENT	1											// Increment of 1 ms per interrupt interval

// Macros to get current timer ticks of system/receive timer
#define PFI_TM_TICKS				(TC0->COUNT16.COUNT.reg)					// Counter value register (system timer)
#define PFI_TM_TICKS_REC			(TC0->COUNT16.COUNT.reg)					// Counter value register (receive timer)

// Macros to set compare register for receive timer interrupts
#define PFI_TM_SET_REG_REC(nTicks)	{ TC0->COUNT16.CC[1].reg = nTicks; }		// Access to set register (time stamp of receive timer interrupt)

// Macros to en-/disable system timer interrupts
// System timer _CBT_MS_0 used for send timer interrupts (Link-Layer)
#define PFI_TM_ENABLE_MS_INT		{ TC0->COUNT16.INTENSET.reg = TC_INTENSET_MC(BIT00); }	// Enable ms timer interrupt
#define PFI_TM_DISABLE_MS_INT		{ TC0->COUNT16.INTENCLR.reg = TC_INTENSET_MC(BIT00); }	// Disable ms timer interrupt

#define MAX_TIME					0xFFFF										// Maximum time for timer (redefinded in KnxTm.h)

#endif /* KNXTM_STM32_H_ */
