#include "includes.h"

#define RX_BUFF_SIZE 62
unsigned char rx_buf[RX_BUFF_SIZE];
unsigned char rx_buf_in_index = 0;
unsigned char rxHeadFlag = 0;
unsigned char rxData[RX_BUFF_SIZE];

volatile bool gConsoleTxTransmitted, gConsoleTxDMATransmitted;

void uart0_dma_write(const uint8_t *data, uint16_t size)
{
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)(data));
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)(&UART_0_INST->TXDATA));
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, size);

    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    while (false == gConsoleTxDMATransmitted)
    {
        __WFE();
    }
    gConsoleTxDMATransmitted = false;
}

void uart0_init()
{
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    gConsoleTxTransmitted = false;
    gConsoleTxDMATransmitted = false;
}

void UART_0_INST_IRQHandler(void)
{
    unsigned char hd = 0;
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST))
    {
    case DL_UART_MAIN_IIDX_EOT_DONE:
        gConsoleTxTransmitted = true;
        break;
    case DL_UART_MAIN_IIDX_DMA_DONE_TX:
        gConsoleTxDMATransmitted = true;
        break;
    case DL_UART_MAIN_IIDX_RX:
        rx_buf[rx_buf_in_index] = DL_UART_Main_receiveData(UART_0_INST);
        break;
    default:
        break;
    }
}

// 重定向fputc函数
int fputc(int ch, FILE *stream)
{
    uart0_dma_write((uint8_t *)&ch, 1);
    return ch;
}

// 重定向fputs函数
int fputs(const char *restrict s, FILE *restrict stream)
{

    uint16_t char_len = 0;
    uart0_dma_write((uint8_t *)s, strlen(s));
    char_len = strlen(s);

    return char_len;
}

int puts(const char *_ptr)
{
    return 0;
}
void uart0_printf(const char *format, ...)
{
    va_list args;
    char buffer[128];
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    uart0_dma_write((uint8_t *)buffer, strlen(buffer));
}
