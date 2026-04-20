#include "stm32f4xx.h"
#include "KnxTm_stm32.h"
#include "KnxFt12.h"


////////////////////////////////////////////////////////////////////////////////
// Global data
////////////////////////////////////////////////////////////////////////////////

static volatile uint32_t g_ms = 0;
static volatile uint32_t g_ticks = 0;

/* Timer-Deadlines in ms */
static volatile uint32_t send_deadline_ms = 0;
static volatile uint32_t rcv_deadline_ms  = 0;

void KnxTm_Init(void)
{
    /* SysTick = 1 kHz */
    SysTick->LOAD  = (SystemCoreClock/1000u) - 1u;
    SysTick->VAL   = 0;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

uint32_t KnxTm_GetTimeMs(void) { return g_ms; }
uint32_t KnxTm_GetDelayMs(uint32_t nOldTime) { return (uint32_t)(g_ms - nOldTime); }
bool_t   KnxTm_IsExpiredMs(uint32_t nTimeOut) { return (bool_t)((int32_t)(g_ms - nTimeOut) >= 0); }
void     KnxTm_SleepMs(uint32_t nDelay)
{
    uint32_t start = g_ms;
    while ((uint32_t)(g_ms - start) < nDelay) { __NOP(); }
}

/* „Ticks“ = hier ebenfalls 1 kHz; falls du willst, kannst du auf höher auflösen */
uint32_t KnxTm_GetTimeTicks(void) { return g_ticks; }
uint32_t KnxTm_GetDelayTicks(uint32_t nOldTime) { return (uint32_t)(g_ticks - nOldTime); }
bool_t   KnxTm_IsExpiredTicks(uint32_t nTimeOut) { return (bool_t)((int32_t)(g_ticks - nTimeOut) >= 0); }
void     KnxTm_SleepTicks(uint32_t nDelay)
{
    uint32_t start = g_ticks;
    while ((uint32_t)(g_ticks - start) < nDelay) { __NOP(); }
}

/* Ft1.2 erwartet: SetSendTimer/SetRcvTimer(ms) -> bei Ablauf KnxFt12_OnSendTimer/OnRcvTimer() */
void KnxTm_SetSendTimer(uint32_t nTimeOut)
{
    /* Timeout relativ (ms) ab jetzt */
    send_deadline_ms = g_ms + nTimeOut;
}

void KnxTm_SetRcvTimer(uint32_t nTimeOut)
{
    rcv_deadline_ms = g_ms + nTimeOut;
}

void SysTick_Handler(void)
{
    g_ms++;
    g_ticks++;   /* gleiche Auflösung */

    /* Sendetimer abgelaufen? */
    if (send_deadline_ms && (int32_t)(g_ms - send_deadline_ms) >= 0) {
        send_deadline_ms = 0;
        KnxFt12_OnSendTimer();
    }
    /* Empfangstimer abgelaufen? */
    if (rcv_deadline_ms && (int32_t)(g_ms - rcv_deadline_ms) >= 0) {
        rcv_deadline_ms = 0;
        KnxFt12_OnRcvTimer();
    }
}
