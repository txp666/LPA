#ifndef __HAL_UART0_H__
#define __HAL_UART0_H__

void uart0_init(void);
void uart0_dma_write(const uint8_t *data, uint16_t size);
void sendControlParameters();
void uartSendData(uint16_t sp, uint16_t pv, int16_t mv, int16_t P, int16_t I, int16_t D);
void uart0_printf(const char *format, ...);

#endif // __HAL_UART0_H__
