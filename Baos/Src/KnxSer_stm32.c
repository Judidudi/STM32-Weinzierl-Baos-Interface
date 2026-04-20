#include "KnxSer_stm32.h"
#include "KnxFt12.h"


////////////////////////////////////////////////////////////////////////////////
// Frequency detection
////////////////////////////////////////////////////////////////////////////////
uint32_t get_pclk(void) {
    uint32_t cfgr = RCC->CFGR;
    static const uint16_t ahb_presc_tbl[16]  = {1,1,1,1,1,1,1,1, 2,4,8,16,64,128,256,512};
    static const uint8_t  apb_presc_tbl[8]   = {1,1,1,1,2,4,8,16};
    uint32_t hclk = SystemCoreClock / ahb_presc_tbl[(cfgr >> RCC_CFGR_HPRE_Pos) & 0xF];
    uint32_t ppre1= apb_presc_tbl[(cfgr >> RCC_CFGR_PPRE1_Pos) & 0x7];
    return hclk / ppre1;
}


////////////////////////////////////////////////////////////////////////////////
// USART3 initialization
////////////////////////////////////////////////////////////////////////////////
void KnxSer_Init(void)
{
    GPIO_TypeDef  *port = GPIOC;
    USART_TypeDef *usart = USART3;

    // Configuration GPIOC pin10 and pin11
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;        // GPIOC: Bustakt aktivieren
    port->MODER  &= ~GPIO_MODER_MODER10_Msk;    // PC10  : Reset auf Default-Wert
    port->MODER  |= GPIO_MODER_MODER10_1;       // PC10  : Alt. Funk.
    port->AFR[1] &= ~GPIO_AFRH_AFRH2;           // PC10  : AF zuruecksetzen
    port->AFR[1] |= GPIO_AFRH_AFRH2_2 |         // PC10  : AF7 waehlen
                    GPIO_AFRH_AFRH2_1 |
                    GPIO_AFRH_AFRH2_0;

    port->MODER  &= ~GPIO_MODER_MODER11_Msk;    // PC11  : Reset auf Default-Wert
    port->MODER  |= GPIO_MODER_MODER11_1;       // PC11  : Alt. Funk.
    port->AFR[1] &= ~GPIO_AFRH_AFRH3;           // PC11  : AF zuruecksetzen
    port->AFR[1] |= GPIO_AFRH_AFRH3_2 |         // PC11  : AF7 waehlen
                    GPIO_AFRH_AFRH3_1 |
                    GPIO_AFRH_AFRH3_0;

    // Configuration USART3
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;        // USART3: activate bus-clock
    usart->BRR  = get_pclk() / 19200u;         	 // USART3: Baudrate = 19600 bps
    usart->CR1 |= USART_CR1_UE;                  // USART3: start USART3
    usart->CR1 |= (USART_CR1_RE | USART_CR1_TE); // USART3: activate Receiver + Transmitter
    usart->CR1 |= USART_CR1_RXNEIE;              // USART3: activate Receiver-Interrupt
    usart->CR1 |= USART_CR1_PCE | USART_CR1_M;	 // USART3: enable parity | 9-bit (8 data + parity)
    usart->CR1 &= ~USART_CR1_PS;				 // USART3: even parity
    usart->CR2 &= ~USART_CR2_STOP_Msk; 			 // USART3: 1 stopbit

    NVIC_SetPriority(USART3_IRQn, 5);
    NVIC_EnableIRQ(USART3_IRQn);
}


////////////////////////////////////////////////////////////////////////////////
// USART3 initialization
////////////////////////////////////////////////////////////////////////////////
void KnxSer_TransmitChar(uint8_t nChar)
{
    /* Wenn DR leer ist, Byte sofort schreiben */
    if (USART3->SR & USART_SR_TXE) {
        USART3->DR = nChar;
    }
    /* TXE-Interrupt einschalten, damit weitere Bytes per IRQ nachgefordert werden */
    USART3->CR1 |= USART_CR1_TXEIE;
}


////////////////////////////////////////////////////////////////////////////////
// USART3 initialization
////////////////////////////////////////////////////////////////////////////////
void USART3_IRQHandler(void)
{
    uint32_t sr = USART3->SR;

    /* --- Fehlerquellen (Parity, Framing, Overrun) löschen --- */
    if (sr & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE)) {
        volatile uint8_t dump = (uint8_t)USART3->DR; // SR+DR lesen => Fehler quittiert
        (void)dump;
    }

    /* --- Empfang (RXNE) --- */
    if (sr & USART_SR_RXNE) {
        uint8_t b = USART3->DR;   // Datenbyte holen
        KnxFt12_OnByteRcvd(b);             // an FT1.2-Stack weitergeben
    }

    /* --- Sende-Puffer leer (TXE) --- */
    if (sr & USART_SR_TXE) {
        uint32_t before = USART3->SR;
        KnxFt12_OnByteSent();              // Stack nach neuem Byte fragen
        uint32_t after = USART3->SR;

        // Wenn der Stack nichts geschrieben hat -> TXE-Interrupt deaktivieren
        if ((before & USART_SR_TXE) && (after & USART_SR_TXE)) {
            USART3->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}




