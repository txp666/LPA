#include "includes.h"

/* I2C 最大传输和接收缓冲区大小 */
#define I2C_TX_MAX_PACKET_SIZE (16)
#define I2C_RX_MAX_PACKET_SIZE (16)

/* 全局状态和缓冲区封装结构 */
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

/* 全局变量 */
static I2C_Controller gI2CController;

/* 初始化函数 */
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

#define IIC_delay 1
/******************************************************************
 * 函 数 名 称：IIC_Start
 * 函 数 说 明：IIC起始信号
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
 ******************************************************************/
void IIC_Start(void)
{
    SDA_OUT();

    SCL(0);
    SDA(1);
    SCL(1);

    delay_us(IIC_delay);

    SDA(0);
    delay_us(IIC_delay);
    SCL(0);
    delay_us(IIC_delay);
}

/******************************************************************
 * 函 数 名 称：IIC_Stop
 * 函 数 说 明：IIC停止信号
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 备       注：无
 ******************************************************************/
void IIC_Stop(void)
{
    SDA_OUT();

    SCL(0);
    SDA(0);

    SCL(1);
    delay_us(IIC_delay);
    SDA(1);
    delay_us(IIC_delay);
}

/******************************************************************
 * 函 数 名 称：IIC_Send_Ack
 * 函 数 说 明：主机发送应答
 * 函 数 形 参：0应答  1非应答
 * 函 数 返 回：无
 * 备       注：无
 ******************************************************************/
void IIC_Send_Ack(uint8_t ack)
{
    SDA_OUT();
    SCL(0);
    SDA(0);
    delay_us(IIC_delay);
    if (!ack)
        SDA(0);
    else
        SDA(1);
    SCL(1);
    delay_us(IIC_delay);
    SCL(0);
    SDA(1);
}

/******************************************************************
 * 函 数 名 称：IIC_Wait_Ack
 * 函 数 说 明：等待从机应答
 * 函 数 形 参：无
 * 函 数 返 回：1=无应答   0=有应答
 * 备       注：无
 ******************************************************************/
uint8_t IIC_Wait_Ack(void)
{
    char ack = 0;
    unsigned char ack_flag = 10;
    SDA_IN();
    SDA(1);
    delay_us(IIC_delay);
    SCL(1);
    delay_us(IIC_delay);
    while ((SDA_GET() == 1) && (ack_flag))
    {
        ack_flag--;
        delay_us(IIC_delay);
    }

    if (ack_flag <= 0)
    {
        IIC_Stop();
        return 1;
    }
    else
    {
        SCL(0);
        SDA_OUT();
    }
    return ack;
}

/******************************************************************
 * 函 数 名 称：IIC_Write
 * 函 数 说 明：IIC写一个字节
 * 函 数 形 参：data - 写入的数据
 * 函 数 返 回：无
 * 备       注：无
 ******************************************************************/
void IIC_Write(uint8_t data)
{
    int i = 0;
    SDA_OUT();
    SCL(0); // 拉低时钟开始数据传输

    for (i = 0; i < 8; i++)
    {
        SDA((data & 0x80) >> 7);
        delay_us(2);
        data <<= 1;
        delay_us(6);
        SCL(1);
        delay_us(4);
        SCL(0);
        delay_us(4);
    }
}

/******************************************************************
 * 函 数 名 称：IIC_Read
 * 函 数 说 明：IIC读1个字节
 * 函 数 形 参：无
 * 函 数 返 回：读出的1个字节数据
 * 备       注：无
 ******************************************************************/
uint8_t IIC_Read(void)
{
    unsigned char i, receive = 0;
    SDA_IN(); // SDA设置为输入
    for (i = 0; i < 8; i++)
    {
        SCL(0);
        delay_us(IIC_delay);
        SCL(1);
        delay_us(IIC_delay);
        receive <<= 1;
        if (SDA_GET())
        {
            receive |= 1;
        }
        delay_us(IIC_delay);
    }
    return receive;
}

/******************************************************************
 * 函 数 名 称：I2C_Master_ReadReg
 * 函 数 说 明：I2C主机从指定设备的指定寄存器读取多个字节数据
 * 函 数 形 参：
 *            dev_addr  - 设备地址
 *            reg_addr  - 寄存器地址
 *            reg_data  - 存储读取数据的缓冲区
 *            count     - 读取的字节数
 * 函 数 返 回：0 - 成功，1 - 失败
 * 备       注：无
 ******************************************************************/
uint8_t atsh_I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count)
{
    uint8_t i = 0;
    IIC_Start();
    IIC_Write(dev_addr << 1 | 0); // 发送设备地址和写信号
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1; // 无应答，返回失败
    }
    IIC_Write(reg_addr); // 发送寄存器地址
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1; // 无应答，返回失败
    }
    IIC_Start();                  // 重新启动信号
    IIC_Write(dev_addr << 1 | 1); // 发送设备地址和读信号
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1; // 无应答，返回失败
    }
    for (i = 0; i < count; i++)
    {
        reg_data[i] = IIC_Read(); // 读取数据
        if (i != (count - 1))
            IIC_Send_Ack(0); // 发送应答信号
        else
            IIC_Send_Ack(1); // 最后一个字节发送非应答信号
    }
    IIC_Stop(); // 发送停止信号
    return 0;   // 成功
}

/******************************************************************
 * 函 数 名 称：I2C_Master_WriteReg
 * 函 数 说 明：I2C主机向指定设备的指定寄存器写入多个字节数据
 * 函 数 形 参：
 *            dev_addr  - 设备地址
 *            reg_addr  - 寄存器地址
 *            reg_data  - 要写入的数据缓冲区
 *            count     - 写入的字节数
 * 函 数 返 回：0 - 成功，1 - 失败
 * 备       注：无
 ******************************************************************/
uint8_t atsh_I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count)
{
    uint8_t i = 0;
    IIC_Start();
    IIC_Write(dev_addr << 1 | 0); // 发送设备地址和写信号
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1; // 无应答，返回失败
    }
    IIC_Write(reg_addr); // 发送寄存器地址
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1; // 无应答，返回失败
    }
    for (i = 0; i < count; i++)
    {
        IIC_Write(reg_data[i]); // 写入数据
        if (IIC_Wait_Ack())
        {
            IIC_Stop();
            return 1; // 无应答，返回失败
        }
    }
    IIC_Stop(); // 发送停止信号
    return 0;   // 成功
}

/******************************************************************
 * 函 数 名 称：I2C_Master_Write
 * 函 数 说 明：I2C主机向指定设备写入多个字节数据
 * 函 数 形 参：
 *            dev_addr  - 设备地址
 *            data      - 要写入的数据缓冲区
 *            count     - 写入的字节数
 * 函 数 返 回：0 - 成功，1 - 失败
 * 备       注：无
 ******************************************************************/
uint8_t atsh_I2C_Master_Write(uint8_t dev_addr, uint8_t *data, uint8_t count)
{
    uint8_t i = 0;
    IIC_Start();
    IIC_Write(dev_addr << 1 | 0); // 发送设备地址和写信号
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1; // 无应答，返回失败
    }
    for (i = 0; i < count; i++)
    {
        IIC_Write(data[i]); // 写入数据
        if (IIC_Wait_Ack())
        {
            IIC_Stop();
            return 1; // 无应答，返回失败
        }
    }
    IIC_Stop(); // 发送停止信号
    return 0;   // 成功
}
