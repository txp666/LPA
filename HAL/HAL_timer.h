#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

extern volatile bool isTimer0Zero;
extern volatile bool timer20ms_int;

void delay_ms(unsigned int ms);
void timer_init();

#endif // __HAL_TIMER_H__
