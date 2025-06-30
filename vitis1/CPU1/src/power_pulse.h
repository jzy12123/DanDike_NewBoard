// power_pulse.h (已更新)

#ifndef POWER_PULSE_H_
#define POWER_PULSE_H_

#include "xparameters.h"
#include "xil_io.h"
#include "stdint.h"

// IP核基地址
#define POWER_PULSE_BASE XPAR_POWER_PULSE_V1_AXI_0_BASEADDR

// 寄存器偏移
#define PP_REG_READ_ENABLE 0x00
#define PP_REG_WRITE_ENABLE 0x04
#define PP_REG_P_DATA 0x08
#define PP_REG_Q_DATA 0x0C

// 中断ID (这些值需要从 xparameters.h 中查找并替换)
#define INTR_ID_PULSE_P XPS_FPGA12_INT_ID
#define INTR_ID_PULSE_Q XPS_FPGA13_INT_ID

// 电能脉冲相关的配置和状态
typedef struct
{
    uint32_t pulseConstantP; // 有功电能脉冲常数 (imp/kWh)
    uint32_t pulseConstantQ; // 无功电能脉冲常数 (imp/kvarh)
    double measuredPowerP;   // 中断测量的有功功率 (kW)
    double measuredPowerQ;   // 中断测量的无功功率 (kvar)
} PowerPulse_t;

extern volatile PowerPulse_t g_PowerPulse;

// 函数声明
void PowerPulse_Init(void);
void PowerPulse_UpdateOutput(double total_p_watts, double total_q_var);
void PowerPulse_P_IntrHandler(void *CallbackRef);
void PowerPulse_Q_IntrHandler(void *CallbackRef);
void PowerPulse_PollInput(void); // <-- 新增轮询函数声明

#endif /* POWER_PULSE_H_ */