#pragma once
#include <stdint.h>
#include <stdarg.h>

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

#define BTN_PORT        GPIOA
#define BTN_GPIO_ENR    RCC_AHB1ENR_GPIOAEN
#define DEBOUNCE_MS     (30u)
#define EXTI_IRQ_PRIO   (6u)

////////////////////////////////////////////////////////////////////////////////
// Public function prototypes
////////////////////////////////////////////////////////////////////////////////

/* Wrapper */
void App_Init();



/* Reset / Fehler */
void App_HandleResetIndication(void);
void App_HandleError(uint8_t code);

/* Server Items */
void App_HandleGetServerItemRes(uint16_t id, uint8_t len, const uint8_t* data);
void App_HandleSetServerItemRes(void);

/* Datapoint Description
   erster Param ist sicher die DP-ID (uint16_t), alles weitere variiert → varargs */
void App_HandleGetDatapointDescriptionRes(uint16_t id, ...);

/* Description String */
void App_HandleGetDescriptionStringRes(const uint8_t* str, ...);

/* Datapoint Values */
void App_HandleGetDatapointValueRes(uint16_t id, uint8_t state, uint8_t length, const uint8_t* data);
void App_HandleDatapointValueInd(uint16_t id, uint8_t state, uint8_t length, const uint8_t* data);
void App_HandleSetDatapointValueRes(void);

/* Server Item Indication */
void App_HandleServerItemInd(uint16_t id, uint8_t length, const uint8_t* data);

/* Parameter Bytes */
void App_HandleGetParameterByteRes(uint16_t index, uint8_t value);






void appdbg_putc(char c);
void appdbg_write(const char* s);
void appdbg_nl(void);
void App_HandleKey(char key);

void Usart2Init();
void Time_Init();
void Buttons_Init();
