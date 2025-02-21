#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#include "includes.h"
// 实现us级延时
#define delay_us(X) delay_cycles((CPUCLK_FREQ / 1000000) * (X))

uint8_t I2C_Master_Read(uint8_t dev_addr, uint8_t *reg_data, uint8_t count);
uint8_t I2C_Master_Write(uint8_t dev_addr, uint8_t *data, uint8_t count);
uint8_t I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
uint8_t I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
void IIC_init();

#endif // __HAL_I2C_H__
