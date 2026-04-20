////////////////////////////////////////////////////////////////////////////////
//
// File: KnxBaos.h
//
// Object server protocol to communicate with BAOS
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////

#ifndef KNXBAOS__H___INCLUDED
#define KNXBAOS__H___INCLUDED


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

// Byte positions in Object Server Protocol
// Common part
#define POS_LENGTH					0											// Length of service
#define POS_MAIN_SERV				1											// Main Service Code
#define POS_SUB_SERV				2											// Sub Service Code
#define POS_STR_DP_HI				3											// Start Data point
#define POS_STR_DP_LO				4											// Start Data point
#define POS_NR_DP_HI				5											// Number of Data points
#define POS_NR_DP_LO				6											// Number of Data points
#define POS_FIRST_DP_ID				7											// First Data point


// GetServerItem.Req/Res part
#define GET_SRV_ITEM_POS_START_HI	3											// StartItem position
#define GET_SRV_ITEM_POS_START_LO	4											// StartItem position
#define GET_SRV_ITEM_POS_NR_HI		5											// NumberOfItems position
#define GET_SRV_ITEM_POS_NR_LO		6											// NumberOfItems position
#define GET_SRV_ITEM_POS_ERROR		7											// ErrorCode position
#define GET_SRV_ITEM_POS_ARRAY		7											// Item array position

#define GET_SRV_ITEM_MIN_LEN		7											// Minimum length of telegram

// SetServerItem.Req/Res part
#define SET_SRV_ITEM_POS_START_HI	3											// StartItem position
#define SET_SRV_ITEM_POS_START_LO	4											// StartItem position
#define SET_SRV_ITEM_POS_NR_HI		5											// NumberOfItems position
#define SET_SRV_ITEM_POS_NR_LO		6											// NumberOfItems position
#define SET_SRV_ITEM_POS_ERROR		7											// ErrorCode position
#define SET_SRV_ITEM_POS_ID_HI		7											// Item ID position
#define SET_SRV_ITEM_POS_ID_LO		8											// Item ID position
#define SET_SRV_ITEM_POS_DATA_LEN	9											// Length of data
#define SET_SRV_ITEM_POS_DATA		10											// Start of data

#define SET_SRV_ITEM_MIN_LEN		7											// Minimum length of telegram

// GetDatapointDescription.Req/Res part
#define GET_DP_DES_POS_START_HI		3											// StartDatapoint position
#define GET_DP_DES_POS_START_LO		4											// StartDatapoint position
#define GET_DP_DES_POS_NR_HI		5											// NumberOfDatapoints position
#define GET_DP_DES_POS_NR_LO		6											// NumberOfDatapoints position
#define GET_DP_DES_POS_ERROR		7											// ErrorCode position
#define GET_DP_DES_POS_ARRAY		7											// DP array position

#define GET_DP_DES_MIN_LEN			7											// Minimum length of telegram

// GetDescriptionString.Req/Res part
#define GET_DES_STR_POS_START_HI	3											// StartString position
#define GET_DES_STR_POS_START_LO	4											// StartString position
#define GET_DES_STR_POS_NR_HI		5											// NumberOfStrings position
#define GET_DES_STR_POS_NR_LO		6											// NumberOfStrings position
#define GET_DES_STR_POS_ERROR		7											// ErrorCode position
#define GET_DES_STR_POS_ARRAY		7											// DP array position

#define GET_DES_STR_MIN_LEN			7											// Minimum length of telegram

// GetDatapointValue.Req/Res part
#define GET_DP_VAL_POS_START_HI		3											// StartDatapoint position
#define GET_DP_VAL_POS_START_LO		4											// StartDatapoint position
#define GET_DP_VAL_POS_NR_HI		5											// NumberOfDatapoints position
#define GET_DP_VAL_POS_NR_LO		6											// NumberOfDatapoints position
#define GET_DP_VAL_POS_ERROR		7											// ErrorCode position
#define GET_DP_VAL_POS_ARRAY		7											// DP array position
#define GET_DP_VAL_POS_FILTER		7											// Filter

#define GET_DP_VAL_MIN_LEN			7											// Minimum length of telegram

// DatapointValue.Ind part
#define DP_VAL_POS_START_HI			3											// StartDatapoint position
#define DP_VAL_POS_START_LO			4											// StartDatapoint position
#define DP_VAL_POS_NR_HI			5											// NumberOfDatapoints position
#define DP_VAL_POS_NR_LO			6											// NumberOfDatapoints position
#define DP_VAL_POS_ARRAY			7											// DP array position

#define DP_VAL_MIN_LEN				7											// Minimum length of telegram

// ServerItem.Ind part
#define SI_VAL_POS_START_HI			3											// StartItem position
#define SI_VAL_POS_START_LO			4											// StartItem position
#define SI_VAL_POS_NR_HI			5											// NumberOfItems position
#define SI_VAL_POS_NR_LO			6											// NumberOfItems position
#define SI_VAL_POS_ARRAY			7											// ServerItem array position

#define SI_VAL_MIN_LEN				7											// Minimum length of telegram

// SetDatapointValue.Req/Res part
#define SET_DP_VAL_POS_START_HI		3											// StartDatapoint position
#define SET_DP_VAL_POS_START_LO		4											// StartDatapoint position
#define SET_DP_VAL_POS_NR_HI		5											// NumberOfDatapoints position
#define SET_DP_VAL_POS_NR_LO		6											// NumberOfDatapoints position
#define SET_DP_VAL_POS_ERROR		7											// ErrorCode position
#define SET_DP_VAL_POS_ARRAY		7											// DP array position

#define SET_DP_VAL_MIN_LEN			7											// Minimum length of telegram

// GetParameterByte.Req/Res part
#define GET_PAR_BYTE_POS_START_HI	3											// StartByte position
#define GET_PAR_BYTE_POS_START_LO	4											// StartByte position
#define GET_PAR_BYTE_POS_NR_HI		5											// NumberOfBytes position
#define GET_PAR_BYTE_POS_NR_LO		6											// NumberOfBytes position
#define GET_PAR_BYTE_POS_ERROR		7											// ErrorCode position
#define GET_PAR_BYTE_POS_ARRAY		7											// Bytes array position

#define GET_PAR_BYTE_MIN_LEN		7											// Minimum length of telegram

// Defines for cEMI protocol
#define M_RESET_IND							0xF0								// Reset indication after a restart or download

// Defines for object server protocol
#define BAOS_MAIN_SRV						0xF0								// Main service code for all BAOS services
#define BAOS_RESET_SRV						0xA0								// Reset/Reboot service code

#define BAOS_SUB_TYPE_MASK					0xC0								// Mask for sub service type
#define BAOS_SUB_TYPE_REQ					0x00								// Sub service type request
#define BAOS_SUB_TYPE_RES					0x80								// Sub service type response
#define BAOS_SUB_TYPE_IND					0xC0								// Sub service type indication


#define BAOS_GET_SRV_ITEM_REQ				0x01								// GetServerItem.Req
#define BAOS_GET_SRV_ITEM_RES				0x81								// GetServerItem.Res

#define BAOS_SET_SRV_ITEM_REQ				0x02								// SetServerItem.Req
#define BAOS_SET_SRV_ITEM_RES				0x82								// SetServerItem.Res

#define BAOS_GET_DP_DESCR_REQ				0x03								// GetDatapointDescription.Req
#define BAOS_GET_DP_DESCR_RES				0x83								// GetDatapointDescription.Res

#define BAOS_GET_DESCR_STR_REQ				0x04								// GetDescriptionString.Req
#define BAOS_GET_DESCR_STR_RES				0x84								// GetDescriptionString.Res

#define BAOS_GET_DP_VALUE_REQ				0x05								// GetDatapointValue.Req
#define BAOS_GET_DP_VALUE_RES				0x85								// GetDatapointValue.Res

#define BAOS_DP_VALUE_IND					0xC1								// DatapointValue.Ind
#define BAOS_SRV_ITEM_IND					0xC2								// ServerItem.Ind

#define BAOS_SET_DP_VALUE_REQ				0x06								// SetDatapointValue.Req
#define BAOS_SET_DP_VALUE_RES				0x86								// SetDatapointValue.Res

#define BAOS_GET_PARAM_BYTE_REQ				0x07								// GetParameterByte.Req
#define BAOS_GET_PARAM_BYTE_RES				0x87								// GetParameterByte.Res


// Defines for commands used by data point services
#define DP_CMD_NONE							0x00								// Do nothing
#define DP_CMD_SET_VAL						0x01								// Change value in datapoint
#define DP_CMD_SEND_VAL						0x02								// Send the current value on the bus
#define DP_CMD_SET_SEND_VAL					0x03								// Change value and send it on the bus
#define DP_CMD_SEND_READ_VAL				0x04								// Send a value read to the bus
#define DP_CMD_CLEAR_STATE					0x05								// Clear the state of a datapoint

// Defines for DatapointDescription configuration flags
#define DP_DES_FLAG_PRIO					0x03								// Transmit priority
#define DP_DES_FLAG_COM						0x04								// Datapoint communication enabled
#define DP_DES_FLAG_READ					0x08								// Read from bus enabled
#define DP_DES_FLAG_WRITE					0x10								// Write from bus enabled
#define DP_DES_FLAG_reserved				0x20								// Reserved
#define DP_DES_FLAG_CTR						0x40								// Clients transmit request processed
#define DP_DES_FLAG_UOR						0x80								// Update on response enabled

// Item ID's used in Get-/SetDeviceItem services
#define ID_NONE									0								// ServerItem: None
#define ID_HARDWARE_TYPE						1								// ServerItem: Hardware type
#define ID_HARDWARE_VER							2								// ServerItem: Hardware version
#define ID_FIRMWARE_VER							3								// ServerItem: Firmware version
#define ID_MANUFACT_SYS							4								// ServerItem: Manufacturer of BAU
#define ID_MANUFACT_APP							5								// ServerItem: Manufacturer of application
#define ID_APP_ID								6								// ServerItem: Application ID
#define ID_APP_VER								7								// ServerItem: Application version
#define ID_SERIAL_NUM							8								// ServerItem: Serial number
#define ID_TIME_RESET							9								// ServerItem: Time since reset
#define ID_BUS_STATE							10								// ServerItem: Bus connection state
#define ID_MAX_BUFFER							11								// ServerItem: Max. buffer size
#define ID_STRG_LEN								12								// ServerItem: Length of description string
#define ID_BAUDRATE								13								// ServerItem: Baudrate
#define ID_BUFFER_SIZE							14								// ServerItem: Current buffer size
#define ID_PROG_MODE							15								// ServerItem: Programming mode
#define ID_PROTOCOL_BIN_VER						16								// ServerItem: Binary protocol version
#define ID_IND_SENDING							17								// ServerItem: Sending of indications
#define ID_PROTOCOL_WS_VER						18								// ServerItem: Protocol version of web services
#define ID_DOWNLOAD_COUNTER						41								// ServerItem: Download counter

#define OBJ_SERV_BUF_SIZE				MAX_FRAME_LENGTH						// Buffer size to handle server objects with the FT1.2 driver
// MAX_FRAME_LENGTH is defined in KnxFt12.h


////////////////////////////////////////////////////////////////////////////////
// Public function prototypes
////////////////////////////////////////////////////////////////////////////////

extern void KnxBaos_Init(void);
extern void KnxBaos_Process(void);

extern bool_t KnxBaos_IsIdle(void);
extern bool_t KnxBaos_GetServerItem(uint16_t nStartItem, uint16_t nNumberOfItems);
extern bool_t KnxBaos_SetServerItem(uint16_t nItId, uint8_t nItDataLen, uint8_t* pItData);
extern bool_t KnxBaos_GetDpDescription(uint16_t nStartDatapoint, uint16_t nNumberOfDatapoints);
extern bool_t KnxBaos_GetDescriptionString(uint16_t nStartString, uint16_t nNumberOfStrings);
extern bool_t KnxBaos_GetDpValue(uint16_t nStartDatapoint, uint16_t nNumberOfDatapoints);
extern bool_t KnxBaos_SendValue(uint16_t nDpId, uint8_t nDpCmd, uint8_t nDpValLen, uint8_t* pDpVal);
extern bool_t KnxBaos_GetParameterByte(uint16_t nStartByte, uint16_t nNumberOfBytes);


#endif /* KNXBAOS__H___INCLUDED */

// End of KnxBaos.h
