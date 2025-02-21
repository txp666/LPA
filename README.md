<p align="center">
  <img width="40%" align="center" src="./images/LPA.png" alt="logo">
</p>
  <h1 align="center">
  低功耗分析仪
</h1>

使用纸飞机串口助手观察曲线（电压/V,电流/mA）：

![images](./images/纸飞机.png)
根据自己要测的最大电流，修改分流电阻提高分辨率：

![images](./images/分流电阻.png)

改完电阻修改程序 INA228 设置：

![images](./images/ina228.png)

# 目录树:

```
LPA
├─ .ccsproject
├─ .clangd
├─ .cproject
├─ .project
├─ HAL
│  ├─ HAL_I2C.c
│  ├─ HAL_I2C.h
│  ├─ HAL_timer.c
│  ├─ HAL_timer.h
│  ├─ HAL_uart0.c
│  └─ HAL_uart0.h
├─ HardWare
│  ├─ INA228.c
│  └─ INA228.h
├─ includes.h
├─ main.c
├─ main.syscfg
├─ README.md
└─ targetConfigs
   ├─ MSPM0L1306.ccxml
   └─ readme.txt

```
