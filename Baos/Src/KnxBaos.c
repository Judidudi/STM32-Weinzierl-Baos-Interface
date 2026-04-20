//////////////////////////////////////////////////////////////////////////////////
//
// File: KnxBaos.c
//
// ObjectServer protocol to communicate with BAOS
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include <string.h>																// For memmove()

#include "config.h"																// Global configuration
#include "StdDefs.h"															// General defines and project settings
#include "KnxSer_stm32.h"																// RS232 Communication
#include "KnxTm_stm32.h"																// Header for the timer functions
#include "KnxBaos.h"															// Object server protocol
#include "KnxBaosHandler.h"														// Call back handler


////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

// Defines for transmission state
#define TR_STATE_IDLE		0													// Transmission State can send to BAOS
#define TR_STATE_WF_RES		1													// Transmission State wait for response

#define TR_TIMEOUT			500													// The time wait for response in ms

#define MAX_SERVER_ITEM_LEN 10													// The max length of server item data


////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

// Type of call back function
typedef void (*t_Rcv_callback)(uint8_t* pBuf);									// Function type for receive call back handler


////////////////////////////////////////////////////////////////////////////////
// Local members
////////////////////////////////////////////////////////////////////////////////

static uint8_t		  m_pBaosRcvBuf[OBJ_SERV_BUF_SIZE];							// Buffer to Receive Telegrams, Note not smaller than 128 byte
static uint8_t		  m_pBaosSndBuf[OBJ_SERV_BUF_SIZE];							// This Buffer is used to send with KnxFt12_Write
static uint8_t		  m_nBaosState;												// State of the transceiver system
static uint32_t		  m_nBaosTimeStamp;											// Is used to handle the timeout
static t_Rcv_callback m_pCallbackOnRcv;											// Call back pointer for receive


////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

static bool_t KnxBaos_SendToDriver(uint8_t* pBuffer);


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

/// This function initializes the BAOS communication system.
///
void KnxBaos_Init(void)
{
	KnxTm_Init();																// Set timer-configurations

	m_pCallbackOnRcv = &KnxBaos_RcvCallBackFunc;								// Store call back pointer
	m_nBaosState = TR_STATE_IDLE;												// Initialize transmission state

	m_pBaosSndBuf[POS_LENGTH] = 0;												// Initialize the Buffer

	KnxFt12_Init();																// Initialize BAOS communication

}

/// Cyclic processing of communication.
/// This function tries to read new telegrams from
/// FT1.2 driver and checks whether a pending DP
/// service can be sent to BAOS HW.
///
void KnxBaos_Process(void)
{
	bool_t bAccept;																// Accept the current telegram?

	// First proceed the receive job
	if(KnxFt12_Read(m_pBaosRcvBuf))												// Poll telegrams from BAOS
	{
		bAccept = FALSE;														// Initialize telegram to be ignored

		if((m_pBaosRcvBuf[POS_LENGTH] == 1) &&									// Check length
		   ((m_pBaosRcvBuf[POS_MAIN_SERV] == BAOS_RESET_SRV) ||					// and reset indication from FT 1.2
			(m_pBaosRcvBuf[POS_MAIN_SERV] == M_RESET_IND)))						// or reset indication from cEMI
		{
			m_pBaosRcvBuf[POS_MAIN_SERV] = BAOS_RESET_SRV;						// Change always to BAOS_RESET_SRV
			bAccept = TRUE;														// Telegram is a reset indication, so we accept it
		}

		if((m_pBaosRcvBuf[POS_LENGTH] >= 3) &&
		   (m_pBaosRcvBuf[POS_MAIN_SERV] == BAOS_MAIN_SRV))						// Check length and main service code for BAOS
		{
			bAccept = TRUE;														// Telegram is a main service, so we accept it
		}

		if(bAccept == TRUE)														// Do we accept the telegram?
		{
			// Check if it is a response
			if((m_pBaosRcvBuf[POS_SUB_SERV] & BAOS_SUB_TYPE_MASK) ==			// If received service
			   BAOS_SUB_TYPE_RES)												// is a response
			{
				if(m_nBaosState == TR_STATE_WF_RES)								// The last Job is completed
				{
					m_nBaosState = TR_STATE_IDLE;								// Go back to idle
				}
				// else: unexpected response
			}

			// Pass any received data to application
			if(m_pCallbackOnRcv != NULL)										// Check call-back pointer
			{
				m_pCallbackOnRcv(m_pBaosRcvBuf);								// Call the call back function
			}
		}
		else
		{
			return;																// no, telegram not accepted
		}
	}

	if(m_nBaosState == TR_STATE_IDLE)											// If we are not waiting
	{
		if(m_pBaosSndBuf[POS_LENGTH] != 0)										// If buffer is not empty
		{
			KnxBaos_SendToDriver(m_pBaosSndBuf);								// Send the service in buffer
			m_pBaosSndBuf[POS_LENGTH] = 0;										// Set the buffer empty
		}
	}
	else																		// Else: we are waiting
	{
		if(KnxTm_GetDelayMs(m_nBaosTimeStamp) > TR_TIMEOUT)						// Wait too long for response
		{
			// This would be an error case, go back to idle
			m_nBaosState = TR_STATE_IDLE;										// Timeout ready for next
		}
	}
}

/// This function returns the transmission state of the
/// BAOS communication. It can be called by the application
/// to check whether a non buffered service is available.
///
/// @return TRUE if sending is possible,
///			FALSE if we are waiting for a response
///
bool_t KnxBaos_IsIdle(void)
{
	return m_nBaosState == TR_STATE_IDLE;										// Return state
}

/// This function send GetServerItem request to BAOS.
///
/// @param[in] nStartItem First ServerItem ID
/// @param[in] nNumberOfItems Number of ServerItems to request
/// @return TRUE if successful,
///			FALSE on error
///
bool_t KnxBaos_GetServerItem(
	uint16_t nStartItem, uint16_t nNumberOfItems)
{
	uint8_t	 pBuffer[8];														// Local buffer for service

	pBuffer[POS_LENGTH]						= 6;								// Length of service
	pBuffer[POS_MAIN_SERV]					= BAOS_MAIN_SRV;					// Main service code
	pBuffer[POS_SUB_SERV]					= BAOS_GET_SRV_ITEM_REQ;			// Sub service code
	pBuffer[GET_SRV_ITEM_POS_START_HI]		= HIBYTE(nStartItem);				// ID of first item
	pBuffer[GET_SRV_ITEM_POS_START_LO]		= LOBYTE(nStartItem);				// ID of first item
	pBuffer[GET_SRV_ITEM_POS_NR_HI]			= HIBYTE(nNumberOfItems);			// Number of items to return
	pBuffer[GET_SRV_ITEM_POS_NR_LO]			= LOBYTE(nNumberOfItems);			// Number of items to return

	return KnxBaos_SendToDriver(pBuffer);										// Pass request to driver
}

/// This function sends a SetServerItem request to BAOS.
/// This function can send only one item per call.
///
/// @param[in] nItId ServerItem ID
/// @param[in] nItDataLen Length of ServerItem (bytes)
/// @param[in] pItData Pointer to data of ServerItem
/// @return TRUE if successful,
///			FALSE on error
///
bool_t KnxBaos_SetServerItem(
	uint16_t nItId, uint8_t nItDataLen, uint8_t* pItData)
{
	uint8_t	 pBuffer[11+MAX_SERVER_ITEM_LEN];									// Local buffer for service
	uint16_t one = 1;

	if(nItDataLen > MAX_SERVER_ITEM_LEN)										// If item too long
	{
		return FALSE;															// Return error
	}

	pBuffer[POS_LENGTH]					= 9 + nItDataLen;						// Set length of service
	pBuffer[POS_MAIN_SERV]				= BAOS_MAIN_SRV;						// Main service code
	pBuffer[POS_SUB_SERV]				= BAOS_SET_SRV_ITEM_REQ;				// Sub service code
	pBuffer[SET_SRV_ITEM_POS_START_HI]	= HIBYTE(nItId);						// ID of first item
	pBuffer[SET_SRV_ITEM_POS_START_LO]	= LOBYTE(nItId);						// ID of first item
	pBuffer[SET_SRV_ITEM_POS_NR_HI]		= HIBYTE(one);							// Number of items
	pBuffer[SET_SRV_ITEM_POS_NR_LO]		= LOBYTE(one);							// Number of items

	pBuffer[SET_SRV_ITEM_POS_ID_HI]		= HIBYTE(nItId);						// Write the ID of item in the Buffer
	pBuffer[SET_SRV_ITEM_POS_ID_LO]		= LOBYTE(nItId);						// Write the ID of item in the Buffer
	pBuffer[SET_SRV_ITEM_POS_DATA_LEN]	= nItDataLen;							// Write the length of item in the Buffer

	memmove(&pBuffer[SET_SRV_ITEM_POS_DATA], pItData, nItDataLen);				// Copy the Value to Buffer

	return KnxBaos_SendToDriver(pBuffer);										// Pass request to driver
}

/// This function sends a GetDatapointDescription request to BAOS.
///
/// @param[in] nStartDatapoint First data point ID
/// @param[in] nNumberOfDatapoints Number of data points to request
/// @return TRUE if successful,
///			FALSE on error
///
bool_t KnxBaos_GetDpDescription(
	uint16_t nStartDatapoint, uint16_t nNumberOfDatapoints)
{
	uint8_t	 pBuffer[7];														// Local buffer for service

	pBuffer[POS_LENGTH]					= 6;									// Length of service
	pBuffer[POS_MAIN_SERV]				= BAOS_MAIN_SRV;						// Main service code
	pBuffer[POS_SUB_SERV]				= BAOS_GET_DP_DESCR_REQ;				// Sub service code
	pBuffer[GET_DP_DES_POS_START_HI]	= HIBYTE(nStartDatapoint);				// ID of first Data point
	pBuffer[GET_DP_DES_POS_START_LO]	= LOBYTE(nStartDatapoint);				// ID of first Data point
	pBuffer[GET_DP_DES_POS_NR_HI]		= HIBYTE(nNumberOfDatapoints);			// Maximum number of Descriptions to return
	pBuffer[GET_DP_DES_POS_NR_LO]		= LOBYTE(nNumberOfDatapoints);			// Maximum number of Descriptions to return

	return KnxBaos_SendToDriver(pBuffer);										// Pass request to driver
}

/// This function sends a GetDescriptionString request to BAOS.
///
/// @param[in] nStartString First description string ID
/// @param[in] nNumberOfStrings Number of description strings to request
/// @return TRUE if successful,
///			FALSE on error
///
bool_t KnxBaos_GetDescriptionString(
	uint16_t nStartString, uint16_t nNumberOfStrings)
{
	uint8_t	 pBuffer[7];														// Local buffer for service

	pBuffer[POS_LENGTH]					= 6;									// Length of service
	pBuffer[POS_MAIN_SERV]				= BAOS_MAIN_SRV;						// Main service code
	pBuffer[POS_SUB_SERV]				= BAOS_GET_DESCR_STR_REQ;				// Sub service code
	pBuffer[GET_DES_STR_POS_START_HI]	= HIBYTE(nStartString);					// ID of first String
	pBuffer[GET_DES_STR_POS_START_LO]	= LOBYTE(nStartString);					// ID of first String
	pBuffer[GET_DES_STR_POS_NR_HI]		= HIBYTE(nNumberOfStrings);				// Maximum number of Strings to return
	pBuffer[GET_DES_STR_POS_NR_LO]		= LOBYTE(nNumberOfStrings);				// Maximum number of Strings to return

	return KnxBaos_SendToDriver(pBuffer);										// Pass request to driver
}

/// This function sends a GetDescriptionValue request to BAOS.
///
/// @param[in] nStartDatapoint First data point ID
/// @param[in] nNumberOfDatapoints Number of data points to request
/// @return TRUE if successful,
///			FALSE on error
///
bool_t KnxBaos_GetDpValue(
	uint16_t nStartDatapoint, uint16_t nNumberOfDatapoints)
{
	uint8_t	 pBuffer[10];														// Local buffer for service

	pBuffer[POS_LENGTH]					= 7;									// Length of service
	pBuffer[POS_MAIN_SERV]				= BAOS_MAIN_SRV;						// Main service code
	pBuffer[POS_SUB_SERV]				= BAOS_GET_DP_VALUE_REQ;				// Sub service code
	pBuffer[GET_DP_VAL_POS_START_HI]	= HIBYTE(nStartDatapoint);				// ID of first Data point
	pBuffer[GET_DP_VAL_POS_START_LO]	= LOBYTE(nStartDatapoint);				// ID of first Data point
	pBuffer[GET_DP_VAL_POS_NR_HI]		= HIBYTE(nNumberOfDatapoints);			// Maximum number of Data points to return
	pBuffer[GET_DP_VAL_POS_NR_LO]		= LOBYTE(nNumberOfDatapoints);			// Maximum number of Data points to return
	pBuffer[GET_DP_VAL_POS_FILTER]		= 0;									// Filter: Get all datapoint values

	return KnxBaos_SendToDriver(pBuffer);										// Pass request to driver
}

/// This function adds a data point command to a buffer. So the
/// application can use this function even if the communication
/// is busy. The complete request will be sent automatically as
/// soon as the state is idle.
///
/// @param[in] nDpId Data point ID
/// @param[in] nDpCmd Command (eg. DP_CMD_SET_VAL, DP_CMD_SET_SEND_VAL)
/// @param[in] nDpValLen Length of data point values
/// @param[in] pDpVal Pointer to data point value
/// @return TRUE if successful,
///			FALSE on error
///
bool_t KnxBaos_SendValue(
	uint16_t nDpId, uint8_t nDpCmd,
	uint8_t nDpValLen, uint8_t* pDpVal)
{
	uint8_t* pBaosSndBufPos;													// Position to write in the buffer
	uint16_t nLowestDpId;														// Data point ID received from BAOS
	uint16_t nDpCount;															// Number of data points in frame

	if(nDpValLen > 14)															// If value length is out of range
	{
		return FALSE;															// It's not a good run
	}

	if(m_pBaosSndBuf[POS_LENGTH] == 0)											// If buffer is empty
	{
		// Case: No data pending: Prepare an empty
		// request here to fill the data below
		pBaosSndBufPos = m_pBaosSndBuf;											// Initialize the Pointer
		*pBaosSndBufPos++ = 6;													// Set service length
		*pBaosSndBufPos++ = BAOS_MAIN_SRV;										// Set main service code
		*pBaosSndBufPos++ = BAOS_SET_DP_VALUE_REQ;								// Set sub service code
		*pBaosSndBufPos++ = HIBYTE(nDpId);										// ID of first Data point
		*pBaosSndBufPos++ = LOBYTE(nDpId);										// ID of first Data point
		*pBaosSndBufPos++ = 0;													// Number of data points
		*pBaosSndBufPos++ = 0;
	}
	else if (m_pBaosSndBuf[POS_LENGTH] + 4 + nDpValLen < MAX_FRAME_LENGTH)		// Else: Add to pending service
	{
		// Case: Data pending: Set the pointer
		// to service to the next empty position.
		pBaosSndBufPos = m_pBaosSndBuf + m_pBaosSndBuf[POS_LENGTH] + 1;			// Calculate the Pointer

		// Set lowest data point ID in frame
		nLowestDpId = BUILD_WORD(m_pBaosSndBuf[POS_STR_DP_HI], m_pBaosSndBuf[POS_STR_DP_LO]);

		if(nLowestDpId > nDpId)													// Check lowest DpId in buffer
		{
			m_pBaosSndBuf[POS_STR_DP_HI] = HIBYTE(nDpId);						// Fix lowest DpId
			m_pBaosSndBuf[POS_STR_DP_LO] = LOBYTE(nDpId);						// Fix lowest DpId
		}
	}
	else																		// Else: Overflow
	{
		// Case: Data pending and there is not enough
		// space left to add this data: Error
		return FALSE;															// Return error
	}

	// Add the data point data to the buffer
	*pBaosSndBufPos++ = HIBYTE(nDpId);											// Set data point Id
	*pBaosSndBufPos++ = LOBYTE(nDpId);											// Set data point Id
	*pBaosSndBufPos++ = nDpCmd;
	*pBaosSndBufPos++ = nDpValLen;												// Set command and value length
	memmove(pBaosSndBufPos, pDpVal, nDpValLen);									// Copy the value to buffer

	m_pBaosSndBuf[POS_LENGTH] += 4 + nDpValLen;									// Calculate the new length of service

	// Increment number of data points
	nDpCount =																	// Get the number of data points
		BUILD_WORD(m_pBaosSndBuf[POS_NR_DP_HI], m_pBaosSndBuf[POS_NR_DP_LO]);
	nDpCount++;																	// Inc the number of data points

	m_pBaosSndBuf[POS_NR_DP_HI] = HIBYTE(nDpCount);								// Store number of data points (high)
	m_pBaosSndBuf[POS_NR_DP_LO] = LOBYTE(nDpCount);								// Store number of data points (low)

	return TRUE;
}

/// This function send GetParameterByte request to BAOS.
///
/// @param[in] nStartByte First parameter byte ID
/// @param[in] nNumberOfBytes Number of parameter bytes to request
/// @return TRUE if successful,
///			FALSE on error
///
bool_t KnxBaos_GetParameterByte(
	uint16_t nStartByte, uint16_t nNumberOfBytes)
{
	uint8_t	 pBuffer[7];														// Local buffer for service

	pBuffer[POS_LENGTH]					= 6;									// Length of service
	pBuffer[POS_MAIN_SERV]				= BAOS_MAIN_SRV;						// Main service code
	pBuffer[POS_SUB_SERV]				= BAOS_GET_PARAM_BYTE_REQ;				// Sub service code
	pBuffer[GET_PAR_BYTE_POS_START_HI]	= HIBYTE(nStartByte);					// ID of first Byte
	pBuffer[GET_PAR_BYTE_POS_START_LO]	= LOBYTE(nStartByte);					// ID of first Byte
	pBuffer[GET_PAR_BYTE_POS_NR_HI]		= HIBYTE(nNumberOfBytes);				// Number of bytes to return
	pBuffer[GET_PAR_BYTE_POS_NR_LO]		= LOBYTE(nNumberOfBytes);				// Number of bytes to return

	return KnxBaos_SendToDriver(pBuffer);										// Pass request to driver
}

/// This function is used internally to pass a request
/// to the corresponding communication driver, here
/// we pass it to FT1.2.
///
/// @param[in] pBuffer Pointer to BAOS frame
/// @return TRUE if successful,
///			FALSE on error
///
static bool_t KnxBaos_SendToDriver(uint8_t* pBuffer)
{
	if(m_nBaosState != TR_STATE_IDLE)											// If communication busy
	{
		return FALSE;															// Return error
	}

	if(KnxFt12_Write(pBuffer))													// Pass request to FT1.2 driver
	{
		m_nBaosTimeStamp = KnxTm_GetTimeMs();									// Get a time stamp for timeout
		m_nBaosState = TR_STATE_WF_RES;											// Set state to waiting for response
		return TRUE;															// Return success
	}
	else																		// Else: Driver failed
	{
		return FALSE;															// Return error
	}
}

// End of KnxBaos.c
