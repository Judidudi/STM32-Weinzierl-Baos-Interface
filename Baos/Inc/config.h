////////////////////////////////////////////////////////////////////////////////
//
// File: config.h
//
// Global configuration for Demo Application
//
// (c) 2002-2016 WEINZIERL ENGINEERING GmbH
//
////////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG__H___INCLUDED
#define CONFIG__H___INCLUDED


////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

// The following define is for debug purposes,
// it could be defined either here or in
// the settings of the IDE.
// It only does effectively activate some
// stop points at dedicated positions. See below

//#define _DEBUG 1


// The Development board has an Atmel SAMD20 MCU.

#define FR_CLOCK			(7372800ul)											// Base Time: 1 s


// If we are in debugging mode >= 1,
// stop at the dedicated points DBG_SUSPEND
// in a never ending loop.

#if _DEBUG >= 1
#define DBG_SUSPEND for(;;)
#else
#define DBG_SUSPEND
#endif

#endif /* CONFIG__H___INCLUDED */

// End of config.h
