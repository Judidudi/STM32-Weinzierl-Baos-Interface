#include <stm32f4xx.h>
#include <stdint.h>
#include "KnxSer_stm32.h"
#include "KnxTm_stm32.h"
#include "KnxFt12.h"
#include "KnxBaos.h"
#include "App.h"


////////////////////////////////////////////////////////////////////////////////
// Main function
////////////////////////////////////////////////////////////////////////////////

int main(void)
{
    SystemCoreClockUpdate();

    KnxBaos_Init();
    App_Init();

    while (1) {
        KnxBaos_Process();
        __NOP();
    }
}
