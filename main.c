#include "includes.h"

uint16_t Cnt;

int main(void)
{
    SYSCFG_DL_init();
    uart0_init();
    timer_init();
    IIC_init();
    INA228_init();
    uart0_printf("start\n");
    while (1)
    {
        while (isTimer0Zero == false)
        {
            __WFI();
        }
        isTimer0Zero = false;
        // while (timer20ms_int == false)
        // {
        //     __WFI();
        // }
        // timer20ms_int = false;
        uint16_t vbus = (uint16_t)INA228_Get_VBUS();
        uint16_t current = (uint16_t)INA228_Get_CURRENT();
        uartSendData(vbus, current, 0, 0, 0, 0);
    }
}
