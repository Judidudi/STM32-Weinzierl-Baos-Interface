////////////////////////////////////////////////////////////////////////////////
//
// File: KnxFt12.h
//
// Interface to BAOS over FT1.2 protocol
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////

#ifndef KNXFT12__H___INCLUDED
#define KNXFT12__H___INCLUDED


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "config.h"																// Global configuration
#include "StdDefs.h"															// General defines and project settings
#include "stdint.h"

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

// Service code for KNX EMI2
// Reset used for fixed frame telegrams
// transmitted in packets of length 1
#define EMI2_L_RESET_IND		0xA0


// Define for FT1.2 internal buffer length
// Internal buffer usage: len, data, data, ..
// len is the count of following BAOS data bytes
// In the buffer there is no FT1.2 control info
// Max len = MAX_FRAME_LENGTH - 1
// MAX_FRAME_LENGTH	set to 251 can hold a BAOS frame with 250 bytes
// This is the standard buffer length of the KNX BAOS 83x implementation
#define MAX_FRAME_LENGTH		251												// Max. length of KNX data frame
#define SND_BUF_LENGTH			MAX_FRAME_LENGTH								// Size of buffer (in bytes) for one KNX frame

// Byte positions in Object Server Protocol (length only)
#define POS_LENGTH				0												// Length of service

// Defines for FT1.2 protocol
#define FT12_START_FIX_FRAME	0x10											// Start byte for frames with fixed length
#define FT12_START_VAR_FRAME	0x68											// Start byte for frames with variable length
#define FT12_CONTROL_SEND		0x53											// Control field for sending udata to module
#define FT12_CONTROL_RCV_MASK	0xDF											// Mask to check Control field for receiving
#define FT12_CONTROL_RCV		0xD3											// Control field for receiving udata to module
#define FT12_END_CHAR			0x16											// The end character for FT1.2 protocol
#define FT12_FCB_MASK			0x20											// mask to get fcb byte in control field
#define FT12_LAST_SEND_FCB		m_nLastSendFcb									// The last frame count bit for sending
#define FT12_NEXT_SEND_FCB		(m_nLastSendFcb ^= 0x20)						// The next frame count bit for sending
#define FT12_LAST_RCV_FCB		m_nLastRcvFcb									// The last frame count bit for receiving
#define FT12_NEXT_RCV_FCB		(m_nLastRcvFcb ^= 0x20)							// The next frame count bit for receiving
#define FT12_ACK				0xE5											// Acknowledge byte for FT1.2 protocol


// Control bytes for fixed length telegrams
#define FT12_RESET_IND			0xC0											// Reset indication
#define FT12_RESET_REQ			0x40											// Control field for sending a reset request to BAU
#define FT12_STATUS_REQ			0x49											// Status request
#define FT12_STATUS_RES			0x8B											// Respond status
#define FT12_CONFIRM_ACK		0x80											// Confirm acknowledge
#define FT12_CONFIRM_NACK		0x81											// Confirm not acknowledge


////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

// States for Ft12 protocol send state machine
enum FT12_SEND_STATE
{
	// States with no traffic
	FT12_SEND_STOPPED,															// State machine stopped
	FT12_SEND_STARTING,															// Starting state machine, not yet ready
	FT12_SEND_IDLE,																// Ready for sending or receiving
	FT12_SEND_PAUSE,															// We wait 3ms before sending next telegram

	// States to send frames with fixed length
	WAIT_TO_SEND_CONTROL_F,														// We have sent the start byte (0x10), now we have to send the control field
	WAIT_TO_SEND_CHECKSUM_F,													// We have sent the control byte, now we have to send the checksum byte
	WAIT_TO_SEND_END_F,															// We have sent the checksum byte, now we have to send the end byte (0x16)

	// States to send frames with variable length
	WAIT_TO_SEND_LENGTH1,														// We have sent the first start byte (0x68), now we have to send the first length byte
	WAIT_TO_SEND_LENGTH2,														// We have sent the first length byte, now we have to send the second length byte
	WAIT_TO_SEND_START2,														// We have sent the second length byte, now we have to send the second start byte (0x68)
	WAIT_TO_SEND_CONTROL,														// We have sent the second start byte, now we have to send the control field
	WAIT_TO_SEND_DATA,															// We have sent at least the control byte, now we have to send the next data byte
	WAIT_TO_SEND_CHECKSUM,														// We have sent all data bytes, now we have to send the checksum byte
	WAIT_TO_SEND_END,															// We have sent the checksum byte, now we have to send the end byte (0x16)
	WAIT_FOR_SENT_END,															// We have sent the end byte, now we have to wait for the THE interrupt
	WAIT_FOR_RCVD_ACK,															// We have sent the complete frame, now we wait for the ACK character (0xE5)
	WAIT_FOR_SENT_ACK,															// We have sent an ACK character (0xE5) and wait for the THE interrupt
	WAIT_FOR_SENT_RCVD_ACK														// We have sent a telegram AND after that an ACK character for a received frame
	// Now we wait for send and receive interrupt
};


// States for Ft12 protocol receive state machine
enum FT12_RCV_STATE
{
	// States with no traffic
	FT12_RCV_STOPPED,															// State machine stopped
	FT12_RCV_STARTING,															// Starting state machine, not yet ready
	FT12_RCV_IDLE,																// Ready for sending or receiving

	// States to receive frames with fixed length
	WAIT_TO_RCV_CONTROL_F,														// We have received the start byte (0x10), now we have to RCV the control field
	WAIT_TO_RCV_CHECKSUM_F,														// We have received the control byte, now we have to send the checksum byte
	WAIT_TO_RCV_END_F,															// We have received the checksum byte, now we have to send the end byte (0x16)

	// States to receive frames with variable length
	WAIT_TO_RCV_LENGTH1,														// We have received the first start byte (0x68), now we wait for the first length byte
	WAIT_TO_RCV_LENGTH2,														// We have received the first length byte, now we wait for the second length byte
	WAIT_TO_RCV_START2,															// We have received the second length byte, now we wait for the second start byte (0x68)
	WAIT_TO_RCV_CONTROL,														// We have received the second start byte, now we wait for the control field
	WAIT_TO_RCV_DATA,															// We have received at least the control byte, now we wait for the next data byte
	WAIT_TO_RCV_CHECKSUM,														// We have received all data bytes, now we wait for the checksum byte
	WAIT_TO_RCV_END,															// We have received the checksum byte, now we wait for the end byte (0x16)

	// States depending on send state machine
	WAIT_TO_SEND_ACK,															// We have received the complete telegram and now we are waiting for send state machine

	RECEIVING_BUSY,																// We are receiving a telegram without having a free buffer
	RECEIVING_TOO_LONG															// We are receiving a telegram, but our buffer is too small
};


////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

extern void	  KnxFt12_Init(void);												// Initialize Ft1.2 communication
extern void	  KnxFt12_Close(void);												// Stops communication
extern bool_t KnxFt12_Read(uint8_t* pBuffer);									// Read telegram from receive buffer to pBuffer
extern bool_t KnxFt12_Write(uint8_t* pBuffer);									// Write telegram from pBuffer to send buffer
extern bool_t KnxFt12_GetConnectionStatus();									// Return whether is connected
extern bool_t	KnxFt12_ResetRequest(void);

// Declaration of event functions
// only called by system functions
extern void	 KnxFt12_OnByteSent(void);											// Called when next byte can be sent
extern void	 KnxFt12_OnSendTimer(void);											// Called on send timer event
extern void	 KnxFt12_OnPreamble(void);											// Called when preamble frame received
extern void	 KnxFt12_OnByteRcvd(uint8_t nByte);									// Called when data byte has been received
extern void	 KnxFt12_OnRcvTimer(void);											// Called on receive timer event


#endif /* KNXFT12__H___INCLUDED */

// End of KnxFt12.h
