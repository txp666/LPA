#ifndef __HAL_UART0_H__
#define __HAL_UART0_H__

void uart0_init(void);
void uart0_dma_write(const uint8_t *data, uint16_t size);

#endif // __HAL_UART0_H__
