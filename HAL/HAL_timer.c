#include "includes.h"

volatile bool isTimer0Zero = false;

void timer_init()
{
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
}

volatile unsigned int delay_times = 0;
void delay_ms(unsigned int ms)
{
    delay_times = ms;
    while (delay_times != 0)
        __WFE();
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST))
    {
    case DL_TIMER_IIDX_ZERO:
        if (delay_times != 0)
        {
            delay_times--;
        }
        isTimer0Zero = true;
        break;

    default:
        break;
    }
}
