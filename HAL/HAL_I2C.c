#include "includes.h"

#define I2C_TX_MAX_PACKET_SIZE (16)
#define I2C_RX_MAX_PACKET_SIZE (16)
typedef struct
{
    uint8_t txBuffer[I2C_TX_MAX_PACKET_SIZE]; // 发送缓冲区
    uint8_t rxBuffer[I2C_RX_MAX_PACKET_SIZE]; // 接收缓冲区
    uint32_t txLen;                           // 发送数据长度
    uint32_t txCount;                         // 已发送字节数
    uint32_t rxLen;                           // 接收数据长度
    uint32_t rxCount;                         // 已接收字节数
    volatile enum {
        I2C_STATUS_IDLE = 0,
        I2C_STATUS_TX_STARTED,
        I2C_STATUS_TX_INPROGRESS,
        I2C_STATUS_TX_COMPLETE,
        I2C_STATUS_RX_STARTED,
        I2C_STATUS_RX_INPROGRESS,
        I2C_STATUS_RX_COMPLETE,
        I2C_STATUS_ERROR,
    } status;
} I2C_Controller;

static I2C_Controller gI2CController;

void IIC_init(void)
{
    NVIC_EnableIRQ(I2C_INST_INT_IRQN);       // 启用中断
    gI2CController.status = I2C_STATUS_IDLE; // 初始化状态
}

/* 写数据函数 */
uint8_t I2C_Master_Write(uint8_t dev_addr, uint8_t *data, uint8_t count)
{
    if (count > I2C_TX_MAX_PACKET_SIZE)
    {
        return 1; // 错误: 数据超出缓冲区
    }

    gI2CController.txLen = count;
    gI2CController.txCount = DL_I2C_fillControllerTXFIFO(I2C_INST, data, count);

    if (gI2CController.txCount < gI2CController.txLen)
    {
        DL_I2C_enableInterrupt(I2C_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    }
    else
    {
        DL_I2C_disableInterrupt(I2C_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    }

    gI2CController.status = I2C_STATUS_TX_STARTED;
    DL_I2C_startControllerTransfer(I2C_INST, dev_addr, DL_I2C_CONTROLLER_DIRECTION_TX, gI2CController.txLen);

    while (gI2CController.status != I2C_STATUS_TX_COMPLETE &&
           gI2CController.status != I2C_STATUS_ERROR)
        __WFE();

    if (gI2CController.status == I2C_STATUS_ERROR)
    {
        return 2; // 错误: 传输失败
    }

    return 0; // 成功
}

/* 读数据函数 */
uint8_t I2C_Master_Read(uint8_t dev_addr, uint8_t *reg_data, uint8_t count)
{
    if (count > I2C_RX_MAX_PACKET_SIZE)
    {
        return 1; // 错误: 数据超出缓冲区
    }

    gI2CController.rxLen = count;
    gI2CController.rxCount = 0;
    gI2CController.status = I2C_STATUS_RX_STARTED;

    DL_I2C_startControllerTransfer(I2C_INST, dev_addr, DL_I2C_CONTROLLER_DIRECTION_RX, gI2CController.rxLen);

    while (gI2CController.status != I2C_STATUS_RX_COMPLETE &&
           gI2CController.status != I2C_STATUS_ERROR)
        __WFE();

    if (gI2CController.status == I2C_STATUS_ERROR)
    {
        return 2; // 错误: 传输失败
    }

    memcpy(reg_data, gI2CController.rxBuffer, count);
    return 0; // 成功
}

/* 寄存器写函数 */
uint8_t I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count)
{
    uint8_t buffer[I2C_TX_MAX_PACKET_SIZE];

    if (count + 1 > I2C_TX_MAX_PACKET_SIZE)
    {
        return 1; // 错误: 数据超出缓冲区
    }

    buffer[0] = reg_addr;
    memcpy(&buffer[1], reg_data, count);

    return I2C_Master_Write(dev_addr, buffer, count + 1);
}

/* 寄存器读函数 */
uint8_t I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count)
{
    if (I2C_Master_Write(dev_addr, &reg_addr, 1) != 0)
    {
        return 1; // 错误: 寄存器地址写入失败
    }

    return I2C_Master_Read(dev_addr, reg_data, count);
}

/* 中断处理函数 */
void I2C_INST_IRQHandler(void)
{
    switch (DL_I2C_getPendingInterrupt(I2C_INST))
    {
    case DL_I2C_IIDX_CONTROLLER_RX_DONE:
        gI2CController.status = I2C_STATUS_RX_COMPLETE;
        break;
    case DL_I2C_IIDX_CONTROLLER_TX_DONE:
        DL_I2C_disableInterrupt(I2C_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
        gI2CController.status = I2C_STATUS_TX_COMPLETE;
        break;
    case DL_I2C_IIDX_CONTROLLER_RXFIFO_TRIGGER:
        while (!DL_I2C_isControllerRXFIFOEmpty(I2C_INST))
        {
            if (gI2CController.rxCount < gI2CController.rxLen)
                gI2CController.rxBuffer[gI2CController.rxCount++] = DL_I2C_receiveControllerData(I2C_INST);
            else
                DL_I2C_receiveControllerData(I2C_INST);
        }
        break;
    case DL_I2C_IIDX_CONTROLLER_TXFIFO_TRIGGER:
        if (gI2CController.txCount < gI2CController.txLen)
        {
            gI2CController.txCount += DL_I2C_fillControllerTXFIFO(I2C_INST,
                                                                  &gI2CController.txBuffer[gI2CController.txCount],
                                                                  gI2CController.txLen - gI2CController.txCount);
        }
        break;
    case DL_I2C_IIDX_CONTROLLER_ARBITRATION_LOST:
    case DL_I2C_IIDX_CONTROLLER_NACK:
        gI2CController.status = I2C_STATUS_ERROR;
        break;
    default:
        break;
    }
}
