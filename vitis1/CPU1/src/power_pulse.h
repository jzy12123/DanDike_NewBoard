// power_pulse.h (已更新)

#ifndef POWER_PULSE_H_
#define POWER_PULSE_H_

#include "xparameters.h"
#include "xil_io.h"
#include "stdint.h"
#include "stdbool.h"
#include "soft_timer.h"
#include "Communications_Protocol.h" // 为了使用 cJSON
#include <string.h>
// IP核基地址
#define POWER_PULSE_BASE XPAR_POWER_PULSE_V1_AXI_0_BASEADDR

// 寄存器偏移
#define PP_REG_READ_ENABLE 0x00
#define PP_REG_WRITE_ENABLE 0x04
#define PP_REG_P_DATA 0x08
#define PP_REG_Q_DATA 0x0C

// 电能脉冲相关的配置和状态
typedef struct
{
    uint32_t pulseConstantP; // 有功电能脉冲常数 (imp/kWh)
    uint32_t pulseConstantQ; // 无功电能脉冲常数 (imp/kvarh)
    double measuredPowerP;   // 中断测量的有功功率 (kW)
    double measuredPowerQ;   // 中断测量的无功功率 (kvar)
} PowerPulse_t;
extern volatile PowerPulse_t g_PowerPulse;

// 电能脉冲误差测试结构体
typedef struct
{
    volatile bool isActive; // 测试是否正在进行
    volatile char testMode; // 'P' 或 'Q'
    uint32_t pulseConstant; // 被测表的脉冲常数
    uint32_t freqDivFactor; // 分频系数
    uint32_t targetRounds;  // 目标圈数 (每轮的脉冲数)
    uint32_t targetTimes;   // 目标测试次数

    volatile uint32_t currentPulseCount; // 当前累计的脉冲数
    volatile uint32_t currentTestNum;    // 当前已完成的测试次数
    double errs[99];                     // 存储每次测试的误差结果，最多99次

    volatile In_CurrTime roundStartTime; // 当前一轮测试的起始时间戳
} EnergyTest_t;
extern EnergyTest_t g_EnergyTest_P; // P通道测试状态
extern EnergyTest_t g_EnergyTest_Q; // Q通道测试状态

// --- 新增代码: 定义用于从ISR传递报告数据到主循环的 "信箱" 结构体 ---
typedef struct
{
    char result[16];              // 中文注释: 存储 "Doing" 或 "Success" 结果字符串
    volatile bool report_pending; // 中文注释: 报告标志位，true 表示有新报告需要发送
    EnergyTest_t p_test_snapshot; // 中文注释: P通道测试数据的快照，供主循环安全读取
    EnergyTest_t q_test_snapshot; // 中文注释: Q通道测试数据的快照
} EnergyTestReportData_t;
extern EnergyTestReportData_t g_energy_report_data;

// 函数声明
void PowerPulse_Init(void);
void PowerPulse_UpdateOutput(double total_p_watts, double total_q_var);
void PowerPulse_P_IntrHandler(void *CallbackRef);
void PowerPulse_Q_IntrHandler(void *CallbackRef);
void PowerPulse_PollInput(void); // 新增轮询函数声明

void process_energy_pulse(volatile EnergyTest_t *test_state, double measured_power_kw);
void init_EnergyTest(void);          // 电能初始化函数
void PowerPulse_TerminateTest(void); // 终止电能测试函数
#endif