/*
 * HAL_timer.c
 * 定时器0每1ms产生一次中断
 * Change Logs:
 * Date          Author     Notes
 * 2024-11-14    txp        first version
 */

#include "includes.h"

volatile bool isTimer0Zero = false;
volatile bool timer20ms_int = false;
volatile uint8_t timer1ms_int = 0;
volatile uint32_t timer1ms_count = 0;

void timer_init()
{
    // 清除定时器中断标志
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    // 使能定时器中断
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
}

volatile unsigned int delay_times = 0;
void delay_ms(unsigned int ms)
{
    delay_times = ms;
    while (delay_times != 0)
        __WFE();
}

// 定时器的中断服务函数
void TIMER_0_INST_IRQHandler(void)
{
    // 如果产生了定时器中断
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST))
    {
    case DL_TIMER_IIDX_ZERO: // 如果是0溢出中断
        if (delay_times != 0)
        {
            delay_times--;
        }
        timer1ms_int++;
        timer1ms_int %= 20;
        if (timer1ms_int == 0)
            timer20ms_int = true;
        isTimer0Zero = true;
        break;

    default: // 其他的定时器中断
        break;
    }
}
