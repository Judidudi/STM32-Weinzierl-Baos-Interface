////////////////////////////////////////////////////////////////////////////////
//
// File: KnxFt12.c
//
// Interface to BAOS over FT1.2 protocol
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include <string.h>																// For memmove

#include "config.h"																// Global configuration
#include "StdDefs.h"															// Global defines
#include "KnxSer_stm32.h"																// Serial Communication
#include "KnxFt12.h"															// KNX Ft1.2 functions
#include "KnxBuf.h"																// KNX telegram buffer
#include "KnxTm_stm32.h"														// Timer functions

////////////////////////////////////////////////////////////////////////////////
// Global data
////////////////////////////////////////////////////////////////////////////////

// Variables for KNX-Communication

// Send & receive state machine
enum FT12_SEND_STATE m_nFt12SendState;											// Actual state of send state machine
enum FT12_RCV_STATE	 m_nFt12RcvState;											// Actual state of receive state machine

// Different controls
uint8_t m_nLastSendFcb;															// Buffer to store the last frame count bit (0010 0000) for sending
uint8_t m_nLastRcvFcb;															// Buffer to store the last frame count bit (0010 0000) for receiving
uint8_t m_nRcvdLength;															// Length in received var. telegrams
uint8_t m_nRcvdControl;															// Control byte in received fixed or var. telegrams;

// checksum used by FT1.2 protocol
uint8_t m_nSendCheckSum;														// Buffer to calculate the checksum while sending
uint8_t m_nRcvCheckSum;															// Buffer to calculate the checksum while receiving

// variables used by state machines
uint8_t m_nBytesSent;															// Byte-Counter for actual send telegram
uint8_t m_nBytesRcvd;															// Byte-Counter for actual receive telegram
uint8_t m_nRetryCount;															// Counter for send repetition

uint8_t m_pSndBuffer[SND_BUF_LENGTH];											// The send buffer

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

void   KnxFt12_Init(void);
void   KnxFt12_Close(void);
bool_t KnxFt12_Read(uint8_t* pBuffer);
bool_t KnxFt12_Write(uint8_t* pBuffer);

// Declaration of "private" functions
// should not be called by external modules
bool_t	KnxFt12_ResetRequest(void);												// Send reset request to BAOS
void	KnxFt12_SendNextTel(void);												// Tries to send telegram from send buffer
uint8_t KnxFt12_SendAck(void);													// Send ACK to BAOS
void	KnxFt12_OnReceivedAck(void);											// Called when ACK from BAOS received
void	KnxFt12_OnFixedRcvd(uint8_t nControl);									// Called when fixed frame telegram received


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

/// Initializes FT 1.2 communication
///
void KnxFt12_Init(void)
{
	DISABLE_KNX_SERIAL_INT;														// Disable send & receive interrupt
	m_nBytesSent		= 0;													// Byte-Counter for sending
	m_nBytesRcvd		= 0;													// Byte-Counter for receiving
	m_nRetryCount		= 0;													// Initialize error counter
	m_nSendCheckSum		= 0x00;													// Reset send checksum
	m_nRcvCheckSum		= 0x00;													// Reset receive checksum

	m_nLastSendFcb		= 0x00;													// Next Send FCB will be TRUE
	m_nLastRcvFcb		= 0x00;													// Next receive	 FCB must be TRUE
	m_nFt12SendState	= FT12_SEND_STARTING;									// Initialize of state machine
	m_nFt12RcvState		= FT12_RCV_STARTING;									// Initialize of state machine

	KnxBuf_Init();																// Initialize of KNX receive buffer
	m_pSndBuffer[POS_LENGTH] = 0x00;											// Initialize send buffer

	KnxSer_Init();																// Initialize the on chip UART
	KnxTm_SleepMs(50);															// Wait for BAOS to start

	KnxFt12_ResetRequest();														// Force an FT1.2 reset.
	KnxTm_SetSendTimer(500);													// Wait till BAOS is ready
	KnxTm_SetRcvTimer(30);														// Wait till BAOS is ready
}

/// Stops FT 1.2 communication
///
void KnxFt12_Close()
{
	DISABLE_KNX_SERIAL_INT;														// Disable send/receive interrupt

	KnxBuf_Init();																// Clear KNX buffer

	m_nFt12SendState	= FT12_SEND_STOPPED;									// Stop state machine
	m_nFt12RcvState		= FT12_RCV_STOPPED;										// Stop state machine
}

/// This function writes a reset request to our BAOS
/// via a fixed length frame.
/// Sending here is realized synchronous to avoid
/// additional states in the send state machine.
///
/// @return TRUE if successful
///			FALSE on error (No ACK)
///
bool_t KnxFt12_ResetRequest(void)
{
	uint16_t nTimeStamp;														// Time stamp
	DISABLE_KNX_SERIAL_INT;														// Disable send & receive interrupt

	CLEAR_KNX_THE_INT;															// Clear transmit interrupt flag
	KnxSer_TransmitChar(FT12_START_FIX_FRAME);									// Send the start character
	while(!IS_KNX_THE);															// Wait for byte to be sent

	CLEAR_KNX_THE_INT;															// Clear transmit interrupt flag
	KnxSer_TransmitChar(FT12_RESET_REQ);										// Send reset request
	while(!IS_KNX_THE);															// Wait for byte to be sent

	CLEAR_KNX_THE_INT;															// Clear transmit interrupt flag
	KnxSer_TransmitChar(FT12_RESET_REQ);										// Send checksum (same byte as control)
	while(!IS_KNX_THE);															// Wait for byte to be sent

	CLEAR_KNX_THE_INT;															// Clear transmit interrupt flag
	KnxSer_TransmitChar(FT12_END_CHAR);											// Send end character
	while(!IS_KNX_THE);															// Wait for byte to be sent

	CLEAR_KNX_THE_INT;															// Clear transmit interrupt flag

	nTimeStamp = KnxTm_GetTimeMs();												// Get the time stamp
	while(KnxTm_GetDelayMs(nTimeStamp) < 50)									// While time not expired
	{
		if(IS_KNX_RDA)															// If a byte received
		{
			if(KNX_RCV_CHAR == FT12_ACK)										// If it is ACK
			{
				ENABLE_KNX_SERIAL_INT;											// Enable send & receive interrupt
				return TRUE;													// Return success
			}
		}
	}
	ENABLE_KNX_SERIAL_INT;														// Enable send & receive interrupt
	return FALSE;																// Return error
}

/// This function writes a telegram from pBuffer to the
/// FT1.2 send buffer.
///
/// @param[in] pBuffer Pointer to telegram
/// @return TRUE if successful
///			FALSE on error
///
bool_t KnxFt12_Write(uint8_t* pBuffer)
{
	if(m_nFt12SendState == FT12_SEND_STOPPED)									// If sending is stopped
	{
		// BAOS is stopped
		pBuffer[POS_LENGTH] = 0x00;												// Clear buffer
		return FALSE;															// Return error
	}

	if(m_pSndBuffer[POS_LENGTH] != 0x00)										// If buffer is full
	{
		// Send buffer full
		return FALSE;															// Return error
	}

	if(pBuffer[POS_LENGTH] > MAX_FRAME_LENGTH - 1)								// If length is too large
	{
		// E-Service to long
		pBuffer[POS_LENGTH] = 0x00;												// Clear buffer
		return FALSE;															// Return error
	}

	//memmove(m_pSndBuffer, pBuffer, MAX_FRAME_LENGTH);							// Copy it


	// neu (kompakt & sicher):
	uint16_t len = pBuffer[POS_LENGTH];                 // 0..(MAX_FRAME_LENGTH-1)
	if (len == 0 || len > (MAX_FRAME_LENGTH - 1)) {     // doppelte Absicherung
	    pBuffer[POS_LENGTH] = 0x00;
	    return FALSE;
	}
	uint16_t copy_len = (uint16_t)len + 1;              // +1 für das Längenbyte selbst
	if (copy_len > MAX_FRAME_LENGTH) copy_len = MAX_FRAME_LENGTH;

	memcpy(m_pSndBuffer, pBuffer, copy_len);            // Überlappung gibt's hier nicht → memcpy reicht
	// (optional) Rest nullen, falls Downstream irgendwo über das Längenfeld hinausliest:
	// memset(m_pSndBuffer + copy_len, 0, MAX_FRAME_LENGTH - copy_len);


	if(m_nFt12SendState == FT12_SEND_IDLE)										// If state machine is idle
	{
		KnxTm_SetSendTimer(1);													// Start sending in KnxFt12_OnSendTimer()
	}

	return TRUE;																// Return number of bytes written to buffer
}

/// Read a telegram from receive buffer.
///
/// @param[out] pBuffer Pointer to store telegram to
/// Return: TRUE if a new telegram was read
///			FALSE if no new telegram in buffer or error occurs
///
bool_t KnxFt12_Read(uint8_t* pBuffer)
{
	if(m_nFt12RcvState == FT12_RCV_STOPPED)										// If receiving is stopped
	{
		// BAOS is stopped
		return FALSE;															// Return error
	}

	if(!KnxBuf_HaveRcvTel())													// If receive queue is empty
	{
		return FALSE;															// Return no bytes read
	}

	memmove(pBuffer, GET_USED_RCV_BUF, MAX_FRAME_LENGTH);						// Copy telegram
	KnxBuf_DecRcvQueue();														// Free buffer in receive queue
	return TRUE;																// Return: New telegram read
}

/// Functions of the send state machine.
///
/// Called by interrupt function if
/// transmit hold register is empty.
///
void KnxFt12_OnByteSent(void)
{
	switch(m_nFt12SendState)													// Depend to state of the send StateMachine
	{
		case WAIT_TO_SEND_LENGTH1:
			KnxSer_TransmitChar(m_pSndBuffer[0]+1);								// Send length (including control)
			m_nFt12SendState = WAIT_TO_SEND_LENGTH2;							// Wait for serial interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			break;

		case WAIT_TO_SEND_LENGTH2:
			KnxSer_TransmitChar(m_pSndBuffer[0]+1);								// Send length (including control)
			m_nFt12SendState = WAIT_TO_SEND_START2;								// Wait for serial interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			break;

		case WAIT_TO_SEND_START2:
			KnxSer_TransmitChar(FT12_START_VAR_FRAME);							// Send start byte
			m_nFt12SendState = WAIT_TO_SEND_CONTROL;							// Wait for serial interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			break;

		case WAIT_TO_SEND_CONTROL:
			KnxSer_TransmitChar(FT12_CONTROL_SEND|FT12_NEXT_SEND_FCB);			// Send control byte
			m_nSendCheckSum = FT12_CONTROL_SEND|FT12_LAST_SEND_FCB;				// Calculate checksum
			m_nFt12SendState = WAIT_TO_SEND_DATA;								// Wait for serial interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			break;

		case WAIT_TO_SEND_DATA:
			KnxSer_TransmitChar(m_pSndBuffer[m_nBytesSent+1]);					// Send data byte
			m_nSendCheckSum += m_pSndBuffer[m_nBytesSent+1];					// Calculate checksum
			m_nBytesSent++;														// Increment data byte counter

			if(m_nBytesSent != m_pSndBuffer[0])									// If bytes left to send
			{
				m_nFt12SendState = WAIT_TO_SEND_DATA;							// Wait for serial interrupt
			}
			else
			{
				m_nFt12SendState = WAIT_TO_SEND_CHECKSUM;						// Wait for serial interrupt
			}

			KnxTm_SetSendTimer(3);												// Start timeout timer
			break;

		case WAIT_TO_SEND_CHECKSUM:
			KnxSer_TransmitChar(m_nSendCheckSum);								// Send checksum
			m_nSendCheckSum = 0x00;												// Reset checksum
			m_nFt12SendState = WAIT_TO_SEND_END;								// Wait for serial interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			break;

		case WAIT_TO_SEND_END:
			KnxSer_TransmitChar(FT12_END_CHAR);									// Send end char
			m_nFt12SendState = WAIT_FOR_SENT_END;								// Wait for serial interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			break;

		case WAIT_FOR_SENT_END:

			// Now transmitter is free
			// Do service for receive state machine
			if(m_nFt12RcvState == WAIT_TO_SEND_ACK)								// If we have to send the ACK
			{
				KnxSer_TransmitChar(FT12_ACK);									// Send ACK
				m_nFt12RcvState	 = FT12_RCV_IDLE;								// Receiving is complete
				m_nFt12SendState = WAIT_FOR_SENT_RCVD_ACK;						// Wait for serial interrupt
				KnxTm_SetSendTimer(3);											// Start timeout timer
			}
			else																// Else: No ACK pending
			{
				m_nFt12SendState = WAIT_FOR_RCVD_ACK;							// End byte transmitted
				KnxTm_SetSendTimer(50);											// Start timeout timer for ACK
			}

			break;

		case WAIT_FOR_SENT_RCVD_ACK:											// If we are waiting for sent & receive ACK
			m_nFt12SendState = WAIT_FOR_RCVD_ACK;								// We are still waiting to receive ACK
			KnxTm_SetSendTimer(50);												// Start timeout timer for ACK
			break;

		case WAIT_FOR_SENT_ACK:													// If we are waiting for ACK sent
			m_nFt12SendState = FT12_SEND_IDLE;									// We are still waiting to rcv ACK
			KnxTm_SetSendTimer(2);												// Start timer for next telegram
			break;

		default:																// These stopping compiler warnings
			break;
	}
}

/// Called by receive state machine
/// if a single bit ACK was received.
///
void KnxFt12_OnReceivedAck(void)
{
	switch(m_nFt12SendState)													// Depend to state of the send StateMachine
	{
		case WAIT_FOR_RCVD_ACK:													// If we are waiting for ACK
			m_nRetryCount = 0;													// Reset error counter
			m_nBytesSent  = 0;													// Reset byte counter

			m_pSndBuffer[POS_LENGTH] = 0x00;									// Empty send buffer
			m_nFt12SendState = FT12_SEND_PAUSE;									// End byte transmitted
			KnxTm_SetSendTimer(2);												// Start timer to send next telegram
			break;

		case WAIT_FOR_SENT_RCVD_ACK:											// If we are waiting for sent & receive ACK
			m_nRetryCount = 0;													// Reset error counter
			m_nBytesSent  = 0;													// Reset byte counter
			m_pSndBuffer[POS_LENGTH] = 0x00;									// Empty send buffer
			m_nFt12SendState = WAIT_FOR_SENT_ACK;								// We are still waiting for the send ACK
			KnxTm_SetSendTimer(2);												// Start Timer to send next telegram
			break;

		default:																// These stopping compiler warnings
			break;
	}
}

/// Called to start telegram transmission.
///
void KnxFt12_SendNextTel(void)
{
	if(m_pSndBuffer[POS_LENGTH] != 0x00)										// Send buffer is not empty
	{
		if(m_nRetryCount < 3)													// If we repeated not more than 3 times
		{
			m_nBytesSent = 0;													// Reset byte counter

			if(m_nRetryCount > 0)
			{
				FT12_NEXT_SEND_FCB;												// Prevent the toggle of FCB bit
			}

			KnxSer_TransmitChar(FT12_START_VAR_FRAME);							// Send start char
			m_nFt12SendState = WAIT_TO_SEND_LENGTH1;							// Wait for serial interrupt
			KnxTm_SetSendTimer(3);												// Start timer
		}
		else																	// Else: Nothing to send
		{
			// E-Sending failed 3x
			m_nRetryCount = 0;													// Reset retry counter
			m_pSndBuffer[POS_LENGTH] = 0x00;									// Empty send buffer
			m_nFt12SendState = FT12_SEND_IDLE;									// Go to IDLE state
			KnxTm_SetSendTimer(3);												// Start timer
		}
	}
}

/// Called by receive state machine to send ACK character.
///
/// @return TRUE if ACK has be sent
///			FALSE if ACK has not be sent
///
bool_t KnxFt12_SendAck(void)
{
	switch(m_nFt12SendState)													// Depend to state of the send StateMachine
	{
		case FT12_SEND_STARTING:												// Only we are not sending
		case FT12_SEND_PAUSE:													// We begin to transmit
		case FT12_SEND_IDLE:													// immediately
			KnxSer_TransmitChar(FT12_ACK);										// Send ACK
			m_nFt12SendState = WAIT_FOR_SENT_ACK;								// We are waiting for THE & RDA interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			return TRUE;														// Return: ACK sent

		case WAIT_FOR_RCVD_ACK:													// We are waiting to receive ACK
			KnxSer_TransmitChar(FT12_ACK);										// Send ACK
			m_nFt12SendState = WAIT_FOR_SENT_RCVD_ACK;							// We are waiting for THE & RDA interrupt
			KnxTm_SetSendTimer(3);												// Start timeout timer
			return TRUE;														// Return: ACK sent

		default:																// On any other state
			return FALSE;														// Return: ACK can not be sent
	}
}

/// Called by interrupt timer function.
///
void KnxFt12_OnSendTimer(void)
{
	switch(m_nFt12SendState)													// Depend to state of the send StateMachine
	{
		case FT12_SEND_STARTING:												// We are just starting
		case FT12_SEND_IDLE:													// Called via timer overflow every 65 s
		case FT12_SEND_PAUSE:													// We made a break
			m_nFt12SendState = FT12_SEND_IDLE;									// Go to IDLE state
			KnxFt12_SendNextTel();												// Try to send next telegram
			break;

		case FT12_SEND_STOPPED:													// Sending is stopped
			break;																// Maybe due to timer overflow

		case WAIT_FOR_RCVD_ACK:
			// ACK missing
			m_nBytesSent = 0;													// Reset byte counter
			m_nRetryCount++;													// Increment retry counter
			m_nFt12SendState = FT12_SEND_STARTING;								// Go back to STARTING state
			KnxTm_SetSendTimer(3);												// Start timer for next try
			break;

		case WAIT_FOR_SENT_RCVD_ACK:
			// Internal-Timeout S
			m_nFt12SendState = WAIT_FOR_RCVD_ACK;								// We are still waiting to receive ACK
			KnxTm_SetSendTimer(50);												// Start timeout timer for ACK
			break;

		default:
			m_nBytesSent = 0;													// Reset byte counter
			m_nRetryCount++;													// Increment retry counter

			// Service for receive state machine
			if(m_nFt12RcvState == WAIT_TO_SEND_ACK)								// If we have to send ACK
			{
				KnxSer_TransmitChar(FT12_ACK);									// Send ACK
				m_nFt12RcvState	 = FT12_RCV_IDLE;								// Receiving is complete
				m_nFt12SendState = WAIT_FOR_SENT_ACK;							// We are still waiting to receive ACK
				KnxTm_SetSendTimer(3);											// Start timeout timer for ACK
			}
			else																// Else: No ACK pending
			{
				m_nFt12SendState = FT12_SEND_STARTING;							// Go back to STARTING state
				KnxTm_SetSendTimer(10);											// Start timer for next try
			}
			break;
	}
}

/// Called by interrupt function if a byte is received.
///
/// @param[in] nByte Received byte
///
void KnxFt12_OnByteRcvd(uint8_t nByte)
{
	switch(m_nFt12RcvState)														// Depend to state of the receive StateMachine
	{
		case FT12_RCV_STARTING:													// We are not ready, but we try it
		case FT12_RCV_IDLE:														// We are ready to receive
			switch(nByte)														// Depend to received byte
			{
				case FT12_START_FIX_FRAME:										// 0x10
					m_nFt12RcvState = WAIT_TO_RCV_CONTROL_F;					// Go to next state
					KnxTm_SetRcvTimer(3);										// Start timeout timer
					break;

				case FT12_START_VAR_FRAME:										// 0x68
					m_nBytesRcvd = 0;											// Reset byte counter
					m_nFt12RcvState = WAIT_TO_RCV_LENGTH1;						// Go to next state
					KnxTm_SetRcvTimer(3);										// Start timer for time out
					break;

				case FT12_ACK:													// 0xE5
					KnxFt12_OnReceivedAck();									// Call send state machine
					break;
			}
			break;

		case WAIT_TO_RCV_CONTROL_F:
			m_nRcvdControl	= nByte;											// Write byte into control buffer
			m_nRcvCheckSum	= nByte;											// The same value
			m_nFt12RcvState = WAIT_TO_RCV_CHECKSUM_F;							// We have to wait for the next byte
			KnxTm_SetRcvTimer(3);												// Start timer
			break;

		case WAIT_TO_RCV_CHECKSUM_F:
			if(nByte == m_nRcvCheckSum)											// If checksum is OK
			{
				m_nFt12RcvState = WAIT_TO_RCV_END_F;							// We have to wait for the next byte
				KnxTm_SetRcvTimer(3);											// Start timer
			}
			else
			{
				// Invalid checksum in fixed length telegram received
				m_nFt12RcvState = FT12_RCV_IDLE;								// Reset receive state machine
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
			}
			break;

		case WAIT_TO_RCV_END_F:
			if(nByte == FT12_END_CHAR)											// If end character is OK
			{
				KnxFt12_OnFixedRcvd(m_nRcvdControl);							// Interpret control byte
			}
			else																// Else: Ill. end character
			{
				// Invalid end char in fixed
			}

			m_nFt12RcvState = FT12_RCV_IDLE;									// Reset receive state machine
			KnxTm_SetRcvTimer(MAX_TIME);										// Set receive timer to max (~65s)
			break;

		case WAIT_TO_RCV_LENGTH1:
			m_nRcvdLength = nByte;												// Store length

			m_nFt12RcvState = WAIT_TO_RCV_LENGTH2;								// We have to wait for the next byte
			KnxTm_SetRcvTimer(3);												// Start Timer
			break;

		case WAIT_TO_RCV_LENGTH2:
			if(nByte == m_nRcvdLength)											// If length is correct
			{
				m_nFt12RcvState = WAIT_TO_RCV_START2;							// We have to wait for the next byte
				KnxTm_SetRcvTimer(3);											// Start Timer
			}
			else
			{
				// Invalid length in var
				m_nFt12RcvState = FT12_RCV_IDLE;								// Reset receive state machine
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
			}
			break;

		case WAIT_TO_RCV_START2:
			if(nByte == FT12_START_VAR_FRAME)									// 0x68
			{
				m_nFt12RcvState = WAIT_TO_RCV_CONTROL;							// We have to wait for the next byte
				KnxTm_SetRcvTimer(3);											// Start Timer
			}
			else
			{
				// Invalid start char in var
				m_nFt12RcvState = FT12_RCV_IDLE;								// Reset receive state machine
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
			}
			break;

		case WAIT_TO_RCV_CONTROL:
			if((nByte & FT12_CONTROL_RCV_MASK) == FT12_CONTROL_RCV)				// If control byte is OK
			{
				if(m_nRcvdLength > MAX_FRAME_LENGTH)							// If telegram is too long for our buffer
				{
					// We can not accept telegram because our buffer
					// is to small so we just count the bytes to
					// check the end of telegram
					m_nFt12RcvState = RECEIVING_TOO_LONG;						// We have to wait for the next byte
				}
				else if(!KnxBuf_HaveFreeRcvBuffer())							// If receive buffer is full
				{
					// We can not accept telegram while buffer
					// is full so we just count the bytes to
					// check the end of telegram
					m_nFt12RcvState = RECEIVING_BUSY;							// We have to wait for the next byte
				}
				else															// Else we accept the frame
				{
					m_nRcvdControl	= nByte;									// Write byte into control buffer
					m_nRcvCheckSum	= nByte;									// Start with checksum calculation
					m_nFt12RcvState = WAIT_TO_RCV_DATA;							// We have to wait for the next byte
				}
				m_nBytesRcvd = 1;												// Reserve first byte as length
				KnxTm_SetRcvTimer(3);											// Start Timer
			}
			else																// Else: Control byte failed
			{
				// Invalid control character in var length tel. rcvd
				m_nFt12RcvState = FT12_RCV_IDLE;								// Reset receive state machine
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
			}
			break;

		case WAIT_TO_RCV_DATA:
			GET_FREE_RCV_BUF[m_nBytesRcvd] = nByte;								// Store data byte
			m_nRcvCheckSum += nByte;											// Calculate checksum
			m_nBytesRcvd++;														// Increment data byte counter

			if(m_nBytesRcvd == m_nRcvdLength)									// If we have received all bytes
			{
				m_nFt12RcvState = WAIT_TO_RCV_CHECKSUM;							// We have to wait for the checksum
			}

			KnxTm_SetRcvTimer(3);												// Start Timer
			break;

		case WAIT_TO_RCV_CHECKSUM:
			if(nByte == m_nRcvCheckSum)											// If checksum is OK
			{
				m_nFt12RcvState = WAIT_TO_RCV_END;								// We have to wait for the next byte
				KnxTm_SetRcvTimer(3);											// Start Timer
			}
			else																// Checksum wrong
			{
				// Error: Invalid Checksum in variable length telegram received
				m_nFt12RcvState = FT12_RCV_IDLE;								// Reset receive state machine
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
			}
			break;

		case WAIT_TO_RCV_END:
			if(nByte == FT12_END_CHAR)											// If end character is OK
			{
				if((m_nRcvdControl & FT12_FCB_MASK) == FT12_LAST_RCV_FCB)		// If frame repetition
				{
					// Then: we do not use this telegram
					// Discard repetition
				}
				else															// Else: Frame is next one
				{
					FT12_NEXT_RCV_FCB;											// Toggle receive frame count bit
					GET_FREE_RCV_BUF[POS_LENGTH] = m_nRcvdLength-1;				// Write first byte as length (only bytes)
					KnxBuf_IncRcvQueue();										// Mark this buffer as used
				}

				if(KnxFt12_SendAck())											// If ACK could be sent
				{
					m_nFt12RcvState = FT12_RCV_IDLE;							// Receiving telegram is finished
					KnxTm_SetRcvTimer(MAX_TIME);								// Set receive timer to max (~65s)
				}
				else															// Else: Send state machine was busy
				{
					m_nFt12RcvState = WAIT_TO_SEND_ACK;							// We have to wait for the send state machine
					KnxTm_SetRcvTimer(25);										// to finish sending
				}
			}
			else																// Else: Invalid end character
			{
				// Invalid end char in var
				m_nFt12RcvState = FT12_RCV_IDLE;								// Reset receive state machine
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
			}
			break;

		case RECEIVING_BUSY:
			m_nBytesRcvd++;														// Just count the characters
			// Now we have received all data plus checksum and end char.
			if(m_nBytesRcvd == m_nRcvdLength+2)									// If telegram is complete
			{
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
				m_nFt12RcvState = FT12_RCV_IDLE;								// Go back to IDLE state
			}
			else
			{
				KnxTm_SetRcvTimer(3);											// Start Timer for next byte
			}
			break;

		case RECEIVING_TOO_LONG:
			m_nBytesRcvd++;														// Just count the characters
			// Now we have received all data plus checksum and end char.
			if(m_nBytesRcvd == m_nRcvdLength+2)									// If telegram is complete
			{
				if(nByte == FT12_END_CHAR)										// If it is an end char
				{
					// Nevertheless we send ACK to BAOS
					// to avoid service repetition
					if(KnxFt12_SendAck())										// If ACK could be sent
					{
						m_nFt12RcvState = FT12_RCV_IDLE;						// Receiving telegram is finished
						KnxTm_SetRcvTimer(MAX_TIME);							// Set receive timer to max (~65s)
					}
					else														// Else: Send state machine was busy
					{
						m_nFt12RcvState = WAIT_TO_SEND_ACK;						// We have to wait for the send state machine
						KnxTm_SetRcvTimer(25);									// to finish sending
					}
				}
				else															// Else end without end char
				{
					m_nFt12RcvState = FT12_RCV_IDLE;							// Receiving telegram is finished
					KnxTm_SetRcvTimer(MAX_TIME);								// Set receive timer to max (~65s)
				}
			}
			else																// Else: We are waiting for the next byte
			{
				KnxTm_SetRcvTimer(3);											// Restart Timer for next byte
			}
			break;

		default:																// On any other state
			// Invert state in Ft12 OnByteRcvd
			m_nFt12RcvState = FT12_RCV_IDLE;									// Reset receive state machine
			KnxTm_SetRcvTimer(MAX_TIME);										// Set receive timer to max (~65s)
			break;
	}
}

/// Called by KnxFt12_OnByteRcvd if a fixed
/// length telegram has been received.
///
/// @param[in] nControl Control byte of FT 1.2 telegram
///
void KnxFt12_OnFixedRcvd(uint8_t nControl)
{
	switch(nControl & FT12_CONTROL_RCV_MASK)									// Depend to control byte
	{
		case FT12_RESET_IND:													// Reset indication

			if(KnxFt12_SendAck())												// If ACK could be sent
			{
				m_nFt12RcvState = FT12_RCV_IDLE;								// Receiving telegram is finished
				KnxTm_SetRcvTimer(MAX_TIME);									// Set receive timer to max (~65s)
			}
			else																// Else: Send state machine was busy
			{
				m_nFt12RcvState = WAIT_TO_SEND_ACK;								// We have to wait for the send state machine
				KnxTm_SetRcvTimer(25);											// to finish sending
			}

			m_nLastSendFcb	= 0x00;												// Next Send Fcb will be TRUE
			m_nLastRcvFcb	= 0x00;												// Next Receive Fcb will be TRUE

			if(KnxBuf_HaveFreeRcvBuffer())										// If there are free receive buffer
			{
				GET_FREE_RCV_BUF[POS_LENGTH] = 0x01;							// Write length byte
				GET_FREE_RCV_BUF[1] = EMI2_L_RESET_IND;							// Send Reset indication to application
				KnxBuf_IncRcvQueue();											// Mark this buffer as used
			}
			else																// Else: No free in-buffer
			{
				// Reset indication lost
			}
			break;
	}
}

/// Called by interrupt timer function.
///
void KnxFt12_OnRcvTimer(void)
{
	switch(m_nFt12RcvState)														// Depend to state of the receive StateMachine
	{
		case FT12_RCV_STARTING:
			m_nFt12RcvState = FT12_RCV_IDLE;									// Now we are ready
			break;																// Nothing to do

		case FT12_RCV_STOPPED:													// Called via timer overflow every 65 s
		case FT12_RCV_IDLE:														// Called via timer overflow every 65 s
			break;																// Nothing to do

		default:
			// Timeout R
			m_nFt12RcvState = FT12_RCV_IDLE;									// Reset receive state machine
			m_nBytesRcvd	= 0;												// Reset byte counter
			break;
	}
}

// End of KnxFt12.c
