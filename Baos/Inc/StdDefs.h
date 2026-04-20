////////////////////////////////////////////////////////////////////////////////
//
// File: StdDefs.h
//
// Global types for Demo Application
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////

#ifndef STDDEFS__H___INCLUDED
#define STDDEFS__H___INCLUDED


////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

//#include "samd20G18.h"															// Include chip definition


////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////


// General defines

#ifndef NULL
#define NULL	0																// Used for pointer
#endif

#ifndef FALSE
#define FALSE	0																// Used for boolean
#endif

#ifndef TRUE
#define TRUE	1																// Used for boolean
#endif

// Define Bits
#define BIT00						(1 << 0)									// Bit 0
#define BIT01						(1 << 1)									// Bit 1
#define BIT02						(1 << 2)									// Bit 2
#define BIT03						(1 << 3)									// Bit 3
#define BIT04						(1 << 4)									// Bit 4
#define BIT05						(1 << 5)									// Bit 5
#define BIT06						(1 << 6)									// Bit 6
#define BIT07						(1 << 7)									// Bit 7
#define BIT08						(1 << 8)									// Bit 8
#define BIT09						(1 << 9)									// Bit 9
#define BIT10						(1 << 10)									// Bit 10
#define BIT11						(1 << 11)									// Bit 11
#define BIT12						(1 << 12)									// Bit 12
#define BIT13						(1 << 13)									// Bit 13
#define BIT14						(1 << 14)									// Bit 14
#define BIT15						(1 << 15)									// Bit 15
#define BIT16						(1 << 16)									// Bit 16
#define BIT17						(1 << 17)									// Bit 17
#define BIT18						(1 << 18)									// Bit 18
#define BIT19						(1 << 19)									// Bit 19
#define BIT20						(1 << 20)									// Bit 20
#define BIT21						(1 << 21)									// Bit 21
#define BIT22						(1 << 22)									// Bit 22
#define BIT23						(1 << 23)									// Bit 23
#define BIT24						(1 << 24)									// Bit 24
#define BIT25						(1 << 25)									// Bit 25
#define BIT26						(1 << 26)									// Bit 26
#define BIT27						(1 << 27)									// Bit 27
#define BIT28						(1 << 28)									// Bit 28
#define BIT29						(1 << 29)									// Bit 29
#define BIT30						(1 << 30)									// Bit 30
#define BIT31						(1 << 31)									// Bit 31

// General macros
#define HIBYTE(nWord)				((uint8_t)((nWord >> 8) & 0x00FF))			// Retrieves the high byte of a word
#define LOBYTE(nWord)				((uint8_t)( nWord		& 0x00FF))			// Retrieves the low byte of a word
#define HILO(nWord)					HIBYTE(nWord), LOBYTE(nWord)				// Divide word in two bytes
#define BUILD_WORD(nHigh, nLow)		(((uint16_t)nHigh << 8) + nLow)				// Retrieves a 16 bit word
#define UNREFERENCED_PARAMETER(P)	(P=P)										// Prevents compiler warning


////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

// General type defines. These are normally defined in "stdint.h", which comes
// from the compiler. But if some types are missing, we can define them here.

#ifndef ___bool_t_defined
typedef unsigned char		bool_t;												//	1 Bit variable
#define __bool_t_defined 1
#endif


/*
#ifndef ___int8_t_defined
typedef char				int8_t;												//	8 Bit variable
typedef unsigned char		uint8_t;											//	8 Bit variable (unsigned)
#define __int8_t_defined 1
#endif

#ifndef ___int16_t_defined
typedef int					int16_t;											// 16 Bit variable
typedef unsigned int		uint16_t;											// 16 Bit variable (unsigned)
#define __int16_t_defined 1
#endif

#ifndef ___int32_t_defined
typedef long				int32_t;											// 32 Bit variable
typedef unsigned long		uint32_t;											// 32 Bit variable (unsigned)
#define __int32_t_defined 1
#endif
*/

#endif /* STDDEFS__H___INCLUDED */

// End of StdDefs.h
