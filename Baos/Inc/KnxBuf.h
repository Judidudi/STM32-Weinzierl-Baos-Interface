////////////////////////////////////////////////////////////////////////////////
//
// File: KnxBuf.h
//
// Multiple receive buffer for KNX communication
//
// (c) 2001-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////

#ifndef KNXBUF__H___INCLUDED
#define KNXBUF__H___INCLUDED


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "config.h"																// Global configuration
#include "StdDefs.h"															// General defines and project settings
#include "KnxFt12.h"															// Interface protocol FT1.2 (PEI10)
#include "stdint.h"

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

// Set the buffer sizes
#define RCV_BUF_LENGTH	MAX_FRAME_LENGTH										// Size of buffer (in bytes) for one KNX frame
#define RCV_BUF_COUNT	3														// Max count of frames to be buffered simultaneously


////////////////////////////////////////////////////////////////////////////////
// Global data
////////////////////////////////////////////////////////////////////////////////

// Exports and macros to access the receive queue (from KNX)
extern uint8_t* m_pRcvQBegin;													// Begin of receive queue (First used telegram buffer)
extern uint8_t* m_pRcvQEnd;														// End of receive queue	(First free telegram buffer)

// This macro give pointer to first free receive buffer
#define GET_FREE_RCV_BUF m_pRcvQEnd
// This macro give pointer to first used receive buffer
#define GET_USED_RCV_BUF m_pRcvQBegin


////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

// Functions to initialize the KNX buffer
extern void		KnxBuf_Init(void);												// Initialize the buffers management system

// Functions to manage the receive queue (from KNX)
extern void		KnxBuf_IncRcvQueue(void);										// Increase telegram counter of receive queue
extern void		KnxBuf_DecRcvQueue(void);										// Decrease telegram counter of receive queue
extern bool_t	KnxBuf_HaveFreeRcvBuffer(void);									// Check whether there are free receive buffer
extern bool_t	KnxBuf_HaveRcvTel(void);										// Check whether telegram has been received


#endif /* KNXBUF__H___INCLUDED */

// End of KnxBuf.h
