////////////////////////////////////////////////////////////////////////////////
//
// File   : App.c
// Purpose: Application for laboratory setup
// Date   : 22.08.2025
//
////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "stm32f4xx.h"
#include "KnxBaos.h"
#include "KnxSer_stm32.h"
#include "App.h"


/* --- local function prototypes ----------------------------------------- */
static void HandleButtonChar(char ch);
static void appdbg_hex8(uint8_t v);

static inline bool App_CanSend(void);
bool App_SendDpBool(uint16_t dp_id, bool value);
bool App_SendDpU8(uint16_t dp_id, uint8_t value);
bool App_SendDpFloat16(uint16_t dp_id, float value);
bool App_SetServerItem(uint16_t item_id, const uint8_t* data, uint8_t len);

static uint16_t App_Encode_DPT9_Float(float v);



////////////////////////////////////////////////////////////////////////////////
// Wrapper
////////////////////////////////////////////////////////////////////////////////
void App_Init(void){

    Usart2Init();        // Debug-Ausgabe (ST-Link VCP an PA2/PA3 bleibt frei nutzbar)
    Time_Init();         // DWT-Zeitbasis
    Buttons_Init();      // Taster aktivieren

    appdbg_write("[BAOS] & [APP] Init Done\n");
}



////////////////////////////////////////////////////////////////////////////////
// UART Port-initialisation
////////////////////////////////////////////////////////////////////////////////

void Usart2Init(void){
    GPIO_TypeDef  *port  = GPIOA;
    USART_TypeDef *usart = USART2;

    // Konfiguration von GPIOA Pin2 und Pin3
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;        // GPIOA: Bustakt aktivieren
    port->MODER  &= ~GPIO_MODER_MODER2_Msk;     // PA2  : Reset auf Default-Wert
    port->MODER  |= GPIO_MODER_MODER2_1;        // PA2  : Alt. Funk.
    port->AFR[0] &= ~GPIO_AFRL_AFRL2;           // PA2  : AF zuruecksetzen
    port->AFR[0] |= GPIO_AFRL_AFRL2_2 |         // PA2  : AF7 waehlen
                    GPIO_AFRL_AFRL2_1 |
                    GPIO_AFRL_AFRL2_0;

    port->MODER  &= ~GPIO_MODER_MODER3_Msk;     // PA3  : Reset auf Default-Wert
    port->MODER  |= GPIO_MODER_MODER3_1;        // PA3  : Alt. Funk.
    port->AFR[0] &= ~GPIO_AFRL_AFRL3;           // PA3  : AF zuruecksetzen
    port->AFR[0] |= GPIO_AFRL_AFRL3_2 |         // PA3  : AF7 waehlen
                    GPIO_AFRL_AFRL3_1 |
                    GPIO_AFRL_AFRL3_0;

    // GPIOA Pin 5 als Ausgang konfigurieren (Onboard-LED)
    port->MODER  &= ~GPIO_MODER_MODER5_Msk;     // PA5  : Reset auf Default-Wert
    port->MODER  |= GPIO_MODER_MODER5_0;        // PA5  : Output

    // Konfiguration von USART2
    // Hier wird nur die Baudrate eingestellt. Wortlaenge, Paritaet und Anzahl
    // der Stoppbits entsprechen den Default-Einstellungen nach einem Reset.
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;        // USART2: Bustakt aktivieren
    usart->BRR  = get_pclk() / 115200u;          // USART3: Baudrate = 115200 bps
    usart->CR1 |= USART_CR1_UE;                  // USART2: Starte USART2
    usart->CR1 |= (USART_CR1_RE | USART_CR1_TE); // USART2: Aktiviere Receiver + Transmitter
    usart->CR1 |= USART_CR1_RXNEIE;              // USART2: Aktiviere Receiver-Interrupt
}



/* ==== Zeitbasis über DWT (kein SysTick nötig) ==== */
inline void Time_Init(void){
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Trace freischalten
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;           // Cycle Counter an
}
static inline uint32_t millis(void){
    return DWT->CYCCNT / (SystemCoreClock / 1000u);
}

/* ==== Entprellung + Dispatch ==== */
static uint32_t s_last_ms[5] = {0};



////////////////////////////////////////////////////////////////////////////////
// Button Port-initialisation
////////////////////////////////////////////////////////////////////////////////

void Buttons_Init(void){
    // Clocks
    RCC->AHB1ENR |= BTN_GPIO_ENR;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // PA5..PA9: MODER=00 (Input)
    BTN_PORT->MODER &=
        ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7 |
          GPIO_MODER_MODER8 | GPIO_MODER_MODER9);

    // Pull-Konfiguration lösche
    BTN_PORT->PUPDR &=
        ~(GPIO_PUPDR_PUPDR5 | GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7 |
          GPIO_PUPDR_PUPDR8 | GPIO_PUPDR_PUPDR9);

    // Pull-Down
    BTN_PORT->PUPDR |=
        (GPIO_PUPDR_PUPDR5_1 | GPIO_PUPDR_PUPDR6_1 | GPIO_PUPDR_PUPDR7_1 |
         GPIO_PUPDR_PUPDR8_1 | GPIO_PUPDR_PUPDR9_1);

    // SYSCFG: Port A auf EXTI5..9 routen (PA = 0)
    SYSCFG->EXTICR[1] &= ~(SYSCFG_EXTICR2_EXTI5 | SYSCFG_EXTICR2_EXTI6 | SYSCFG_EXTICR2_EXTI7);
    SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI8 | SYSCFG_EXTICR3_EXTI9);

    // evtl. alte Pending-Flags löschen
    EXTI->PR = (EXTI_PR_PR5 | EXTI_PR_PR6 | EXTI_PR_PR7 | EXTI_PR_PR8 | EXTI_PR_PR9);

    // Mask/Trigger: fallende Flanke (Taster gegen GND)
    EXTI->IMR  |= (EXTI_IMR_MR5 | EXTI_IMR_MR6 | EXTI_IMR_MR7 | EXTI_IMR_MR8 | EXTI_IMR_MR9);
    EXTI->FTSR |= (EXTI_FTSR_TR5 | EXTI_FTSR_TR6 | EXTI_FTSR_TR7 | EXTI_FTSR_TR8 | EXTI_FTSR_TR9);
    EXTI->RTSR &= ~(EXTI_RTSR_TR5 | EXTI_RTSR_TR6 | EXTI_RTSR_TR7 | EXTI_RTSR_TR8 | EXTI_RTSR_TR9);

    // NVIC
    NVIC_SetPriority(EXTI9_5_IRQn, EXTI_IRQ_PRIO);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}




////////////////////////////////////////////////////////////////////////////////
// Interrupt Request Handler
////////////////////////////////////////////////////////////////////////////////

static void HandleLine(uint32_t line){
    // line: 5..9 -> Index 0..4 -> 'a'..'e'
    uint32_t idx = line - 5u;
    uint32_t now = millis();
    if ((now - s_last_ms[idx]) < DEBOUNCE_MS) return;
    s_last_ms[idx] = now;
    HandleButtonChar((char)('a' + idx));              // 5→'a', 6→'b', ..., 9→'e'
}

/* ==== EINZIGE ISR: EXTI9_5 ==== */
void EXTI9_5_IRQHandler(void){
    uint32_t pending = EXTI->PR & (EXTI_PR_PR5 | EXTI_PR_PR6 | EXTI_PR_PR7 |
                                   EXTI_PR_PR8 | EXTI_PR_PR9);
    if (!pending) return;

    if (pending & EXTI_PR_PR5){ EXTI->PR = EXTI_PR_PR5; HandleLine(5); }
    if (pending & EXTI_PR_PR6){ EXTI->PR = EXTI_PR_PR6; HandleLine(6); }
    if (pending & EXTI_PR_PR7){ EXTI->PR = EXTI_PR_PR7; HandleLine(7); }
    if (pending & EXTI_PR_PR8){ EXTI->PR = EXTI_PR_PR8; HandleLine(8); }
    if (pending & EXTI_PR_PR9){ EXTI->PR = EXTI_PR_PR9; HandleLine(9); }
}





////////////////////////////////////////////////////////////////////////////////
// Button key Handle
////////////////////////////////////////////////////////////////////////////////
void App_HandleKey(char key){
    switch (key){
        case 'e':
            if (App_CanSend()){
            	appdbg_write("[CMD] a -> GetDP 1\n");
            	KnxBaos_GetDpValue(1, 1);
            }
            else                          appdbg_write("[CMD] busy (GetDP)\n");
            break;

        case 'b': {
            static bool s; s = !s;
            if (App_SendDpBool(1, s))
            { 	appdbg_write("[CMD] b -> Set DP1 bool=");
            	appdbg_write(s?"1\n":"0\n"); }
            else
				appdbg_write("[CMD] busy (Set DP1 bool)\n");
        } break;

        case 'c': {
            uint16_t v = 0x1c;	//set temp
            if (App_SendDpFloat16(3, v)) { appdbg_write("[CMD] c -> Set DP3 u16=0x"); appdbg_hex8(v); appdbg_nl(); }
            else                      appdbg_write("[CMD] busy (Set DP2 u8)\n");
        } break;

        /*case 'd':
        	if (KnxBaos_GetDpValue(2, 1)) appdbg_write("test: get ist_temp\n");
            else                              appdbg_write("[CMD] busy (Set DP3 float16)\n");
            break;*/

        case 'd':
            if (KnxBaos_GetDpValue(2, 1))
                appdbg_write("[CMD] d -> Get DP2 (Temp)\n");
            else
                appdbg_write("[CMD] busy (Get DP2)\n");
            break;


        case 'a': {
            uint8_t mode = 0x01;
            if (App_SetServerItem(0x0102, &mode, 1)) appdbg_write("[CMD] e -> Set ServerItem 0x0102=01\n");
            else                                      appdbg_write("[CMD] busy (Set ServerItem)\n");
        } break;

        default: break;
    }
}

//Writes pushed Button to the Terminal
static void HandleButtonChar(char ch){
    appdbg_write("[BTN] -> "); appdbg_putc(ch); appdbg_nl();
    App_HandleKey(ch);
}


////////////////////////////////////////////////////////////////////////////////
// UART Transmitfunctions
////////////////////////////////////////////////////////////////////////////////
void appdbg_putc(char c){
    while (!(USART2->SR & USART_SR_TXE)) {}
    USART2->DR = (uint8_t)c;
}

void appdbg_write(const char* s){ while (*s) appdbg_putc(*s++); }

void appdbg_nl(void){ appdbg_putc('\n'); }

static void appdbg_hex8(uint8_t v){
    static const char H[]="0123456789ABCDEF";
    appdbg_putc(H[(v>>4)&0xF]); appdbg_putc(H[v&0xF]);
}
static void appdbg_hex16(uint16_t v){ appdbg_hex8((uint8_t)(v>>8)); appdbg_hex8((uint8_t)v); }
static void appdbg_dump_bytes(const uint8_t* p, uint8_t n){
    for (uint8_t i=0;i<n;i++){ appdbg_hex8(p[i]); if (i+1<n) appdbg_putc(' '); }
}






////////////////////////////////////////////////////////////////////////////////
// Send-Helper MCU -> BAOS
////////////////////////////////////////////////////////////////////////////////

static inline bool App_CanSend(void){
	return KnxBaos_IsIdle();
}

bool App_RequestDpValues(uint16_t start_dp, uint16_t count){
    if (!App_CanSend()) return false;
    return KnxBaos_GetDpValue(start_dp, count);
}

bool App_SendDpBool(uint16_t dp_id, bool value){
    if (!App_CanSend()) return false;
    uint8_t b = value ? 0x01u : 0x00u;
    return KnxBaos_SendValue(dp_id, DP_CMD_SET_SEND_VAL, 1u, &b);
}

bool App_SendDpU8(uint16_t dp_id, uint8_t value){
    if (!App_CanSend()) return false;
    return KnxBaos_SendValue(dp_id, DP_CMD_SET_SEND_VAL, 1u, &value);
}

bool App_SetServerItem(uint16_t item_id, const uint8_t* data, uint8_t len){
    if (!App_CanSend()) return false;
    return KnxBaos_SetServerItem(item_id, len, (uint8_t*)data); // API expect non-const
}

bool App_SendDpFloat16(uint16_t dp_id, float value){
    if (!App_CanSend()) return false;
    uint16_t raw = App_Encode_DPT9_Float(value);
    uint8_t buf[2] = { (uint8_t)(raw>>8), (uint8_t)raw };
    return KnxBaos_SendValue(dp_id, DP_CMD_SET_SEND_VAL, 2u, buf);
}

static uint16_t App_Encode_DPT9_Float(float v){
    int32_t mant = (int32_t)(v * 100.0f + (v>=0 ? 0.5f : -0.5f));
    int8_t  exp  = 0;
    while (mant < -2048 || mant > 2047) { mant >>= 1; exp++; }
    return (uint16_t)(((exp & 0x0F) << 11) | (mant & 0x07FF));
}






////////////////////////////////////////////////////////////////////////////////
// BAOS / FT1.2 Callbacks	BAOS -> MCU
////////////////////////////////////////////////////////////////////////////////
void App_HandleResetIndication(void){
    appdbg_write("[BAOS] Reset.Ind\n");
}
void App_HandleError(uint8_t code){
    appdbg_write("[BAOS] Error 0x");
    appdbg_hex8(code);
    appdbg_nl();
}
void App_HandleGetServerItemRes(uint16_t id, uint8_t len, const uint8_t* data){
    appdbg_write("[BAOS] GetServerItem.Res id="); appdbg_hex16(id);
    appdbg_write(" len="); appdbg_hex8(len);
    appdbg_write(" data="); if (data && len) appdbg_dump_bytes(data, len);
    appdbg_nl();
}
void App_HandleSetServerItemRes(void){
    appdbg_write("[BAOS] SetServerItem.Res OK\n");
}
void App_HandleGetDatapointDescriptionRes(uint16_t id, ...){
    appdbg_write("[BAOS] GetDP.Description.Res id="); appdbg_hex16(id); appdbg_nl();
}
void App_HandleGetDescriptionStringRes(const uint8_t* str, ...){
    unsigned int len = 0u;
    va_list ap; va_start(ap, str);
    if (str) len = va_arg(ap, unsigned int);
    va_end(ap);

    appdbg_write("[BAOS] GetDescriptionString.Res len=");
    appdbg_hex8((uint8_t)len); appdbg_write(" \"");
    for (unsigned int i=0; i<len && str; ++i){
        char c = (char)str[i];
        appdbg_putc((c>=32 && c<127)? c: '.');
    }
    appdbg_write("\"\n");
}
void App_HandleGetDatapointValueRes(uint16_t id, uint8_t state, uint8_t length, const uint8_t* data){
    appdbg_write("[BAOS] GetDP.Value.Res id="); appdbg_hex16(id);
    appdbg_write(" state=0x"); appdbg_hex8(state);
    appdbg_write(" len="); appdbg_hex8(length);
    appdbg_write(" data="); if (data && length) appdbg_dump_bytes(data, length);
    appdbg_nl();
}

void App_HandleDatapointValueInd(uint16_t id, uint8_t state, uint8_t length, const uint8_t* data){
    appdbg_write("[BAOS] DP.Ind id="); appdbg_hex16(id);
    appdbg_write(" state=0x"); appdbg_hex8(state);
    appdbg_write(" len="); appdbg_hex8(length);
    appdbg_write(" data="); if (data && length) appdbg_dump_bytes(data, length);
    appdbg_nl();
}
void App_HandleSetDatapointValueRes(void){
    appdbg_write("[BAOS] SetDP.Value.Res OK\n");
}
void App_HandleServerItemInd(uint16_t id, uint8_t length, const uint8_t* data){
    appdbg_write("[BAOS] ServerItem.Ind id="); appdbg_hex16(id);
    appdbg_write(" len="); appdbg_hex8(length);
    appdbg_write(" data="); if (data && length) appdbg_dump_bytes(data, length);
    appdbg_nl();
}
void App_HandleGetParameterByteRes(uint16_t index, uint8_t value){
    appdbg_write("[BAOS] ParamByte["); appdbg_hex16(index);
    appdbg_write("] = 0x"); appdbg_hex8(value); appdbg_nl();
}
