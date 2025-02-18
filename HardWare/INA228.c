#include "includes.h"

INA228_CONFIGTypeDef INA228_MainConfig;
INA228_ADCTypeDef INA228_ADC_MainConfig;

float CURRENT_LSB;

// 读取函数
uint8_t INA228_Write_Mem(uint8_t addr, uint8_t LENGTH, uint8_t *buff)
{
	return I2C_Master_WriteReg(INA228_ADDR, addr, buff, LENGTH);
}

uint8_t INA228_Read_Mem(uint8_t addr, uint8_t LENGTH, uint8_t *buff)
{
	return I2C_Master_ReadReg(INA228_ADDR, addr, buff, LENGTH);
}

// 分流电阻校准，单位：毫欧,ppm/℃
void INA228_SET_SHUNT_CAL(float RSHUNT, uint16_t ppm)
{
	uint16_t SHUNT_CAL;
	if (INA228_MainConfig.ADCRANGE == 1)
		SHUNT_CAL = 131072 * CURRENT_LSB * 100000 * (RSHUNT / 1000.0) * 4;
	else
		SHUNT_CAL = 131072 * CURRENT_LSB * 100000 * (RSHUNT / 1000.0);

	uint8_t data1[2] = {SHUNT_CAL >> 8, SHUNT_CAL};
	INA228_Write_Mem(INA228_SHUNT_CAL, 2, data1);
	uint8_t data2[2] = {ppm >> 8, ppm};
	INA228_Write_Mem(INA228_SHUNT_TEMPCO, 2, data2);
}

// 获取分流电阻电压
float INA228_Get_VSHUNT(void)
{
	float VSHUNT;
	uint8_t data1[3];
	INA228_Read_Mem(INA228_VSHUNT, 3, data1);
	int32_t data = (data1[0] << 16 | data1[1] << 8 | data1[2]) >> 4;
	if (data & 0x80000)
	{
		data |= 0xFFF00000;
	}
	if (INA228_MainConfig.ADCRANGE == 1)
		VSHUNT = 0.0000003125 * data;
	else
		VSHUNT = 0.000000078125 * data;
	return VSHUNT;
}

// 获取VBUS电压
float INA228_Get_VBUS(void)
{
	float VBUS;
	uint8_t data1[3];
	INA228_Read_Mem(INA228_VBUS, 3, data1);
	uint32_t data = (data1[0] << 16 | data1[1] << 8 | data1[2]) >> 4;
	VBUS = 0.0001953125 * data;
	return VBUS * 10000;
}

// 获取芯片温度
float INA228_Get_DIETEMP(void)
{
	float TEMP;
	int16_t data;
	uint8_t data1[2];
	INA228_Read_Mem(INA228_DIETEMP, 2, data1);
	data = data1[0] << 8 | data1[1];

	TEMP = 0.0078125 * data;
	return TEMP;
}

// 获取电流
float INA228_Get_CURRENT(void)
{
	float CURRENT;
	uint8_t data1[3];
	INA228_Read_Mem(INA228_CURRENT, 3, data1);
	int32_t data = (data1[0] << 16 | data1[1] << 8 | data1[2]) >> 4;
	if (data & 0x80000)
	{
		data |= 0xFFF00000;
	}

	CURRENT = data * CURRENT_LSB;
	return CURRENT * 1000 * 1000 * 10;
}

// 获取功率
float INA228_Get_POWER(void)
{
	float POWER;
	uint8_t data1[3];
	INA228_Read_Mem(INA228_POWER, 3, data1);
	uint32_t data = data1[0] << 16 | data1[1] << 8 | data1[2];
	POWER = data * CURRENT_LSB * 3.2;
	return POWER;
}

// 获取能量
float INA228_Get_ENERGY(void)
{
	float ENERGY;
	uint64_t data;
	uint8_t data1[5];
	INA228_Read_Mem(INA228_ENERGY, 5, data1);
	data = data1[0];
	data = data << 32;
	data = data1[1] << 24 | data1[2] << 16 | data1[1] << 8 | data1[0];
	ENERGY = 16.0 * 3.2 * CURRENT_LSB * data;
	return ENERGY;
}

// 获取电荷量
float INA228_Get_CHARGE(void)
{
	float CHARGE;
	int64_t data;
	uint8_t data1[5];
	INA228_Read_Mem(INA228_CHARGE, 5, data1);
	data = data1[0];
	data = data << 32;
	data = data1[1] << 24 | data1[2] << 16 | data1[1] << 8 | data1[0];
	CHARGE = CURRENT_LSB * data;
	return CHARGE;
}

// 将累积寄存器 ENERGY 和 CHARGE 的内容复位为 0
void INA228_RESET_ACC(void)
{
	uint16_t data = INA228_MainConfig.RST << 15 | INA228_MainConfig.RSTACC << 14 | INA228_MainConfig.CONVDLY << 6 | INA228_MainConfig.TEMPCOMP << 5 | INA228_MainConfig.ADCRANGE << 4;
	data |= 0x4000;
	uint8_t data1[2] = {data >> 8, data};
	INA228_Write_Mem(INA228_CONFIG, 2, data1);
}

void INA228_Set_SOVL() {
};

void INA228_init(void)
{
	// 初始化INA228配置
	INA228_MainConfig.ADCRANGE = 1;
	INA228_MainConfig.MEC = 0.2;
	INA228_MainConfig.TEMPCOMP = 1;
	INA228_MainConfig.RST = 0;
	INA228_MainConfig.RSTACC = 1;

	// 初始化INA228 ADC配置
	INA228_ADC_MainConfig.MODE = 0x0B;
	INA228_ADC_MainConfig.VBUSCT = 0x06;
	INA228_ADC_MainConfig.VSHCT = 0x06;
	INA228_ADC_MainConfig.VTCT = 0x06;
	INA228_ADC_MainConfig.AVG = 0x02;

	// 写入配置
	uint16_t data;
	uint8_t data0[2];

	// 初始化INA228主配置
	data = 1 << 15;
	data0[0] = data >> 8;
	data0[1] = data;
	INA228_Write_Mem(INA228_CONFIG, 2, data0);

	CURRENT_LSB = INA228_MainConfig.MEC / 524288.0;
	data = INA228_MainConfig.RST << 15 | INA228_MainConfig.RSTACC << 14 | INA228_MainConfig.CONVDLY << 6 | INA228_MainConfig.TEMPCOMP << 5 | INA228_MainConfig.ADCRANGE << 4;
	data0[0] = data >> 8;
	data0[1] = data;
	INA228_Write_Mem(INA228_CONFIG, 2, data0);

	// 初始化INA228 ADC配置
	uint16_t adc_data = INA228_ADC_MainConfig.MODE << 12 | INA228_ADC_MainConfig.VBUSCT << 9 | INA228_ADC_MainConfig.VSHCT << 6 | INA228_ADC_MainConfig.VTCT << 3 | INA228_ADC_MainConfig.AVG;
	uint8_t adc_data1[2] = {adc_data >> 8, adc_data};
	INA228_Write_Mem(INA228_ADC_CONFIG, 2, adc_data1);

	// 设置Rshunt阻值和温漂
	INA228_SET_SHUNT_CAL(200, 50);
}
