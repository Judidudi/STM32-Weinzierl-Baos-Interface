////////////////////////////////////////////////////////////////////////////////
//
// File: KnxBuf.c
//
// Multiple receive buffer for KNX communication
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "config.h"																// Global configuration
#include "StdDefs.h"															// General defines and project settings
#include "KnxBuf.h"																// Telegram buffer for KNX communication
#include "KnxSer_stm32.h"																// Serial Communication
#include "KnxTm_stm32.h"																// Timer functions


////////////////////////////////////////////////////////////////////////////////
// Global data
////////////////////////////////////////////////////////////////////////////////

// Variables used to manage the receive queue (from KNX)
uint8_t* m_pRcvQBegin;															// Begin of receive queue (First used telegram buffer)
uint8_t* m_pRcvQEnd;															// End of receive queue	(First free telegram buffer)
uint8_t	 m_nRcvQCount;															// Number of telegrams in receive queue


// Allocate the buffers
uint8_t m_pRcvBuffer[RCV_BUF_COUNT*RCV_BUF_LENGTH];								// The complete receive buffer

#define FIRST_RCV_BUF	m_pRcvBuffer											// The first receive buffer
#define LAST_RCV_BUF	m_pRcvBuffer + ((RCV_BUF_COUNT-1)*RCV_BUF_LENGTH)		// The last receive buffer


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

/// This function does the initialization
/// of the buffer management system.
///
void KnxBuf_Init(void)
{
	m_pRcvQBegin	= m_pRcvBuffer;												// Pointer to first used receive buffer
	m_pRcvQEnd		= m_pRcvBuffer;												// Pointer to first free receive buffer
	m_nRcvQCount	= 0;														// Count of receive telegrams
}


/// The receive buffer.
/// This function has to be called after writing a complete
/// telegram to the first free receive buffer. It changes the
/// this  buffer to the last used.
/// Attention: Before writing any data to buffer,
///			   KnxBuf_HaveFreerRcv() has to be called
///			   to check, whether the buffer is free!
///
void KnxBuf_IncRcvQueue(void)
{
	if(m_nRcvQCount >= RCV_BUF_COUNT)											// If count out of range
	{
		// In buffer overrun
		return;																	// We stop here
	}

	// Set the buffer pointer
	if(m_pRcvQEnd == LAST_RCV_BUF)												// If we point the end of buffer array
	{
		m_pRcvQEnd = FIRST_RCV_BUF;												// The next one is the first buffer
	}
	else																		// Else: Not the end one
	{
		m_pRcvQEnd += RCV_BUF_LENGTH;											// Set pointer to the next one
	}

	m_nRcvQCount++;																// Increment telegram counter
}


/// This function has to be called after reading
/// a complete telegram from the first used buffer.
/// It frees the telegram place in receive queue.
/// Attention: Before reading any data from buffer,
///			   KnxBuf_HaveRcvTel() has to be called
///			   to check, whether a telegram is available!
///
void KnxBuf_DecRcvQueue(void)
{
	if(m_nRcvQCount == 0)														// If no buffer is used
	{
		// receive buffer under run
		return;																	// We stop here
	}

	// Set the buffer pointer
	if(m_pRcvQBegin == LAST_RCV_BUF)											// If we point the end of buffer array
	{
		m_pRcvQBegin = FIRST_RCV_BUF;											// the next one is the first buffer
	}
	else																		// Else: Not the end one
	{
		m_pRcvQBegin += RCV_BUF_LENGTH;											// Set pointer to the next one
	}

	m_nRcvQCount--;																// Decrement telegram counter
}

/// This function is used to check whether
/// we have at least one free receive buffer.
///
/// @return TRUE if we have a free receive buffer
///			FALSE if all in buffers are used
///
bool_t KnxBuf_HaveFreeRcvBuffer(void)
{
	if(m_nRcvQCount < RCV_BUF_COUNT)											// If not max telegrams counted
	{
		return TRUE;															// Return: Buffer free
	}
	else																		// Else: Counter on the max
	{
		// receive buffer full
		return FALSE;															// Return: All buffers full
	}
}

/// This function is used to check whether
/// we have at least one receive telegram in buffer.
///
/// @return TRUE if we have a telegram in buffer
///			FALSE if all receive buffers are free
///
bool_t KnxBuf_HaveRcvTel(void)
{
	if(m_nRcvQCount)															// If count not 0
	{
		return TRUE;															// Return: Telegram available
	}
	else																		// Else: Counter = 0
	{
		return FALSE;															// Return: No telegram available
	}
}


// End of KnxBuf.c
