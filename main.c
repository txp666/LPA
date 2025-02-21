#include "includes.h"

#define PRINT(window, fmt, args...) printf("{" #window "}" fmt "\n", ##args)
int main(void)
{
    SYSCFG_DL_init();
    uart0_init();
    timer_init();
    IIC_init();
    INA228_init();
    while (1)
    {
        while (isTimer0Zero == false)
        {
            __WFI();
        }
        isTimer0Zero = false;
        float v = INA228_Get_VBUS(), i = INA228_Get_CURRENT();
        PRINT(plotter, "%f,%f", v, i);
    }
}
