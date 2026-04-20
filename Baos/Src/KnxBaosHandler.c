////////////////////////////////////////////////////////////////////////////////
//
// File: KnxBaosHandler.c
//
// BAOS: Handler for BAOS call backs
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include "config.h"																// Global configuration
#include "StdDefs.h"															// General defines and project settings
#include "KnxBaos.h"															// Object server protocol
#include "stdint.h"
#include "App.h"



////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

static void KnxBaos_OnGetServerItemRes(uint8_t* pBuf);
static void KnxBaos_OnSetServerItemRes(uint8_t* pBuf);
static void KnxBaos_OnGetDatapointDescriptionRes(uint8_t* pBuf);
static void KnxBaos_OnGetDescriptionStringRes(uint8_t* pBuf);
static void KnxBaos_OnGetDatapointValueRes(uint8_t* pBuf);
static void KnxBaos_OnDatapointValueInd(uint8_t* pBuf);
static void KnxBaos_OnServerItemInd(uint8_t* pBuf);
static void KnxBaos_OnSetDatapointValueRes(uint8_t* pBuf);
static void KnxBaos_OnGetParameterByteRes(uint8_t* pBuf);


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////


/// This function is called by KnxBaos_Process()
/// if data has been received from the BAOS hardware.
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
void KnxBaos_RcvCallBackFunc(uint8_t* pBuf)
{

	switch(pBuf[POS_MAIN_SERV])													// Check main service
	{
		case BAOS_RESET_SRV:													// If reset service code
			App_HandleResetIndication();										// Call reset indication handler
			return;

		case BAOS_MAIN_SRV:														// Main BAOS service

			switch(pBuf[POS_SUB_SERV])
			{
				case BAOS_GET_SRV_ITEM_RES:										// GetServerItem.Res
					KnxBaos_OnGetServerItemRes(pBuf);
					break;

				case BAOS_SET_SRV_ITEM_RES:										// SetServerItem.Res
					KnxBaos_OnSetServerItemRes(pBuf);
					break;

				case BAOS_GET_DP_DESCR_RES:										// GetDatapointDescription.Res
					KnxBaos_OnGetDatapointDescriptionRes(pBuf);
					break;

				case BAOS_GET_DESCR_STR_RES:									// GetDescriptionString.Res
					KnxBaos_OnGetDescriptionStringRes(pBuf);
					break;

				case BAOS_GET_DP_VALUE_RES:										// GetDatapointValue.Res
					KnxBaos_OnGetDatapointValueRes(pBuf);
					break;

				case BAOS_DP_VALUE_IND:											// DatapointValue.Ind
					KnxBaos_OnDatapointValueInd(pBuf);
					break;

				case BAOS_SRV_ITEM_IND:											// ServerItem.Ind
					KnxBaos_OnServerItemInd(pBuf);
					break;

				case BAOS_SET_DP_VALUE_RES:										// SetDatapointValue.Res
					KnxBaos_OnSetDatapointValueRes(pBuf);
					break;

				case BAOS_GET_PARAM_BYTE_RES:									// GetParameterByte.Res
					KnxBaos_OnGetParameterByteRes(pBuf);
					break;

				default:
					DBG_SUSPEND;												// Should never happen
					break;
			}
	}
}

/// Handle the GetServerItem.Res telegram.
///
/// This response is sent by the server as reaction to the GetServerItem
/// request. If an error is detected during the request processing server
/// send a negative response (number of items == 0).
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnGetServerItemRes(uint8_t* pBuf)
{
	uint16_t nNumberOfItems;
	uint8_t	 nErrorCode;
	uint16_t i;																	// Current item number
	uint16_t nItemId;
	uint8_t	 nItemDataLength;
	uint8_t* pData;																// Byte pointer to array

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < GET_SRV_ITEM_MIN_LEN)									// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nNumberOfItems =
		BUILD_WORD(pBuf[GET_SRV_ITEM_POS_NR_HI], pBuf[GET_SRV_ITEM_POS_NR_LO]);	// Get number of items

	if(nNumberOfItems == 0)														// Error, if no items
	{
		nErrorCode = pBuf[GET_SRV_ITEM_POS_ERROR];								// Get error code
		App_HandleError(nErrorCode);
		return;
	}

	pData = &pBuf[GET_SRV_ITEM_POS_ARRAY];										// Set to first Item

	for(i = 1; i <= nNumberOfItems; i++)											// For all items in service
	{
		nItemId = BUILD_WORD(*pData, *(pData + 1));								// Extract ID of item
		pData += 2;
		nItemDataLength = *pData++;												// Extract length of item
		App_HandleGetServerItemRes(nItemId, nItemDataLength, pData);			// Call our handler
		pData += nItemDataLength;												// Set pointer to next item
	}
}

/// Handle the SetServerItem.Res telegram.
///
/// This response is sent by the server as reaction to the SetServerItem
/// request. If an error is detected during the request processing server
/// send a negative response (error code != 0).
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnSetServerItemRes(uint8_t* pBuf)
{
	uint8_t	 nErrorCode;

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < SET_SRV_ITEM_MIN_LEN)									// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nErrorCode = pBuf[SET_SRV_ITEM_POS_ERROR];									// Get error code

	if(nErrorCode != 0)															// Error
	{
		App_HandleError(nErrorCode);
		return;
	}

	App_HandleSetServerItemRes();												// Call our handler
}

/// Handle the GetDatapointDescription.Res telegram.
///
/// This response is sent by the server as reaction to the
/// GetDatapointDescription request. If an error is detected during the request
/// processing, the server sends a negative response
/// (number of data points == 0).
///
/// @param[in] pBuf pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnGetDatapointDescriptionRes(uint8_t* pBuf)
{
	uint16_t nCurrentDatapoint;
	uint16_t nNumberOfDatapoints;
	uint8_t	 nErrorCode;
	uint16_t i;																	// Current data point number
	uint8_t	 nDpValueLength;
	uint8_t	 nDpConfigFlags;
	uint8_t* pData;																// Byte pointer to array

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < GET_DP_DES_MIN_LEN)									// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nNumberOfDatapoints =
		BUILD_WORD(pBuf[GET_DP_DES_POS_NR_HI], pBuf[GET_DP_DES_POS_NR_LO]);		// Get number of data points

	if(nNumberOfDatapoints == 0)												// Error, if no data points
	{
		nErrorCode = pBuf[GET_DP_DES_POS_ERROR];								// Get error code
		App_HandleError(nErrorCode);
		return;
	}

	pData = &pBuf[GET_DP_DES_POS_ARRAY];										// Set to first data point

	for(i = 1; i <= nNumberOfDatapoints; i++)									// For all data points in service
	{
		nCurrentDatapoint = BUILD_WORD(*pData, *(pData + 1));					// Extract ID of data point
		pData += 2;
		nDpValueLength = *pData++;												// Extract length of data point
		nDpConfigFlags = *pData++;												// Extract flags of data point
		pData++;

		App_HandleGetDatapointDescriptionRes(
			nCurrentDatapoint, nDpValueLength, nDpConfigFlags);					// Call our handler

	}
}

/// Handle the GetDescriptionString.Res telegram.
///
/// This response is sent by the server as reaction to the GetDescriptionString
/// request. If an error is detected during the processing of the request, the
/// server sends a negative response (number of strings == 0).
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnGetDescriptionStringRes(uint8_t* pBuf)
{
	uint16_t nNumberOfStrings;
	uint8_t	 nErrorCode;
	uint16_t i;																	// Current string number
	uint8_t* strDpDescription;
	uint16_t nDpDescriptionLength;
	uint8_t* pData;																// Byte pointer to array

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < GET_DES_STR_MIN_LEN)									// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nNumberOfStrings =
		BUILD_WORD(pBuf[GET_DES_STR_POS_NR_HI], pBuf[GET_DES_STR_POS_NR_LO]);	// Get number of strings

	if(nNumberOfStrings == 0)													// Error, if no strings
	{
		nErrorCode = pBuf[GET_DES_STR_POS_ERROR];								// Get error code
		App_HandleError(nErrorCode);
		return;
	}

	pData = &pBuf[GET_DES_STR_POS_ARRAY];										// Set to first string

	for(i = 1; i <= nNumberOfStrings; i++)										// For all strings in service
	{
		nDpDescriptionLength = BUILD_WORD(*pData, *(pData + 1));				// Extract description length of data point
		pData += 2;
		strDpDescription = pData;

		App_HandleGetDescriptionStringRes(
			strDpDescription, nDpDescriptionLength);							// Call our handler

		pData += nDpDescriptionLength;
	}
}

/// Handle the GetDatapointValue.Res telegram.
///
/// This response is sent by the server as reaction to the GetDatapointValue
/// request. If an error is detected during the processing of the request, the
/// server sends a negative response (number of data points == 0).
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnGetDatapointValueRes(uint8_t* pBuf)
{
	uint16_t nNumberOfDatapoints;
	uint8_t	 nErrorCode;
	uint16_t i;																	// Current item number
	uint16_t nDpId;
	uint8_t	 nDpState;
	uint8_t	 nDpLength;
	uint8_t* pData;																// Byte pointer to array

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < GET_DP_VAL_MIN_LEN)									// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nNumberOfDatapoints =
		BUILD_WORD(pBuf[GET_DP_VAL_POS_NR_HI], pBuf[GET_DP_VAL_POS_NR_LO]);		// Get number of data points

	if(nNumberOfDatapoints == 0)												// Error, if no data points
	{
		nErrorCode = pBuf[GET_DP_VAL_POS_ERROR];								// Get error code
		App_HandleError(nErrorCode);
		return;
	}

	pData = &pBuf[GET_DP_VAL_POS_ARRAY];										// Set to first data point

	for(i = 1; i <= nNumberOfDatapoints; i++)									// For all data points in service
	{
		nDpId = BUILD_WORD(*pData, *(pData + 1));								// Extract ID of data point
		pData += 2;
		nDpState  = *pData++;													// Extract state of data point
		nDpLength = *pData++;													// Extract length of data point
		App_HandleGetDatapointValueRes(nDpId, nDpState, nDpLength, pData);		// Call our handler
		pData += nDpLength;														// Set pointer to next item
	}
}

/// Handle the DatapointValue.Ind telegram.
///
/// This indication is sent asynchronously by the server if the data point(s)
/// value is changed.
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnDatapointValueInd(uint8_t* pBuf)
{
	uint16_t nNumberOfDatapoints;
	uint16_t i;																	// Current item number
	uint16_t nDpId;
	uint8_t	 nDpState;
	uint8_t	 nDpLength;
	uint8_t* pData;																// Byte pointer to array

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < DP_VAL_MIN_LEN)										// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nNumberOfDatapoints =
		BUILD_WORD(pBuf[DP_VAL_POS_NR_HI], pBuf[DP_VAL_POS_NR_LO]);				// Get number of data points

	pData = &pBuf[DP_VAL_POS_ARRAY];											// Set to first data point

	for(i = 1; i <= nNumberOfDatapoints; i++)									// For all data points in service
	{
		nDpId = BUILD_WORD(*pData, *(pData + 1));								// Extract ID of data point
		pData += 2;
		nDpState  = *pData++;													// Extract state of data point
		nDpLength = *pData++;													// Extract length of data point
		App_HandleDatapointValueInd(nDpId, nDpState, nDpLength, pData);			// Call our handler
		pData += nDpLength;														// Set pointer to next item
	}
}

/// Handle the ServerItem.Ind telegram.
///
/// This indication is sent asynchronously by the server if the server item(s)
/// has changed.
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnServerItemInd(uint8_t* pBuf)
{
	uint16_t nNumberOfItems;
	uint16_t i;																	// Current item number
	uint16_t nSiId;
	uint8_t	 nSiLength;
	uint8_t* pData;																// Byte pointer to array

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < SI_VAL_MIN_LEN)										// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nNumberOfItems =
		BUILD_WORD(pBuf[SI_VAL_POS_NR_HI], pBuf[SI_VAL_POS_NR_LO]);				// Get number of items

	pData = &pBuf[SI_VAL_POS_ARRAY];											// Set to first item

	for(i = 1; i <= nNumberOfItems; i++)											// For all items in service
	{
		nSiId = BUILD_WORD(*pData, *(pData + 1));								// Extract ID of server item
		pData += 2;
		nSiLength  = *pData++;													// Extract length of item
		App_HandleServerItemInd(nSiId, nSiLength, pData);						// Call our handler
		pData += nSiLength;														// Set pointer to next item
	}
}

/// Handle the SetDatapointValue.Res telegram.
///
/// This response is sent by the server as reaction to the SetDatapointValue
/// request. If an error is detected during the processing of the request, the
/// server sends a negative response (error code != 0).
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnSetDatapointValueRes(uint8_t* pBuf)
{
	uint8_t	 nErrorCode;

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < SET_DP_VAL_MIN_LEN)									// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nErrorCode = pBuf[SET_DP_VAL_POS_ERROR];									// Get error code

	if(nErrorCode != 0)															// Error
	{
		App_HandleError(nErrorCode);
		return;
	}

	App_HandleSetDatapointValueRes();											// Call our handler
}

/// Handle the GetParameterByte.Res telegram.
///
/// This response is sent by the server as reaction to the GetParameterByte
/// request. If an error is detected during the request processing server send a
/// negative response (number of bytes == 0).
///
/// @param[in] pBuf Pointer to the byte array from KNX BAOS ObjectServer
/// telegram (first entry is length then the actual telegram,
/// see "KNX BAOS ObjectServer Protocol specification")
///
static void KnxBaos_OnGetParameterByteRes(uint8_t* pBuf)
{
	uint16_t nNumberOfBytes;
	uint8_t	 nErrorCode;
	uint16_t i;																	// Current byte number
	uint8_t nByte;
	uint8_t* pData;																// Byte pointer to array

	// Check the length of telegram
	if(pBuf[POS_LENGTH] < DP_VAL_MIN_LEN)										// If telegram is too short
	{
		return;																	// Error, exit here
	}

	nNumberOfBytes =
		BUILD_WORD(pBuf[GET_PAR_BYTE_POS_NR_HI], pBuf[GET_PAR_BYTE_POS_NR_LO]);	// Get number of bytes

	if(nNumberOfBytes == 0)														// Error, if no bytes
	{
		nErrorCode = pBuf[GET_PAR_BYTE_POS_ERROR];								// Get error code
		App_HandleError(nErrorCode);
		return;
	}

	pData = &pBuf[GET_PAR_BYTE_POS_ARRAY];										// Set to first byte

	for(i = 1; i <= nNumberOfBytes; i++)											// For all bytes in service
	{
		nByte = *pData++;														// Get parameter value
		App_HandleGetParameterByteRes(i, nByte);								// Call our handler
	}
}

// End of KnxBaosHandler.c
