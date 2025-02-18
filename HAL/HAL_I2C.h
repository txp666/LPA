#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#include "includes.h"
// 实现us级延时
#define delay_us(X) delay_cycles((CPUCLK_FREQ / 1000000) * (X))
// #define delay_ms(X) delay_cycles((CPUCLK_FREQ / 1000) * (X))
// 设置SDA输出模式
#define SDA_OUT()                                   \
    {                                               \
        DL_GPIO_initDigitalOutput(IOMUX_PINCM1);    \
        DL_GPIO_setPins(GPIOA, DL_GPIO_PIN_0);      \
        DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_0); \
    }
// 设置SDA输入模式
#define SDA_IN()                                \
    {                                           \
        DL_GPIO_initDigitalInput(IOMUX_PINCM1); \
    }
#define SCL_OUT()                                   \
    {                                               \
        DL_GPIO_initDigitalOutput(IOMUX_PINCM2);    \
        DL_GPIO_setPins(GPIOA, DL_GPIO_PIN_1);      \
        DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_1); \
    }
// 获取SDA引脚的电平变化
#define SDA_GET() (((DL_GPIO_readPins(GPIOA, DL_GPIO_PIN_0) & DL_GPIO_PIN_0) > 0) ? 1 : 0)
// SDA与SCL输出
#define SDA(x) ((x) ? (DL_GPIO_setPins(GPIOA, DL_GPIO_PIN_0)) : (DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_0)))
#define SCL(x) ((x) ? (DL_GPIO_setPins(GPIOA, DL_GPIO_PIN_1)) : (DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_1)))

void IIC_Start(void);
void IIC_Stop(void);
void IIC_Send_Ack(uint8_t ack);
uint8_t IIC_Wait_Ack(void);
void IIC_Write(uint8_t data);
uint8_t IIC_Read(void);
uint8_t I2C_Master_Read(uint8_t dev_addr, uint8_t *reg_data, uint8_t count);
uint8_t I2C_Master_Write(uint8_t dev_addr, uint8_t *data, uint8_t count);
uint8_t I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
uint8_t I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
void IIC_init();

uint8_t atsh_I2C_Master_Write(uint8_t dev_addr, uint8_t *data, uint8_t count);
uint8_t atsh_I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
uint8_t atsh_I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
#endif // __HAL_I2C_H__
