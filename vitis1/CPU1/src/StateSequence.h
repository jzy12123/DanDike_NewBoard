#ifndef STATE_SEQUENCE_H
#define STATE_SEQUENCE_H

#include "Communications_Protocol.h"
#include "xaxicdma.h"
#include "xttcps.h"
#include "xparameters.h"

// 状态序列相关结构体定义
#define MAX_SEQ_STEPS 50
#define MAX_SEQ_HARMS 32

// ================= 配置宏定义 =================
#define STATE_SEQ_DDR_BUFFER_BASE 0x30000000    // 预留DDR地址存放预计算波形
#define WAVE_STEP_SIZE_BYTES (DATA_LEN * 4 * 4) // 16KB (1024点 * 4字节 * 4组)

// 定义 BRAM 的物理地址
#define STATE_SEQ_BRAM_BASEADDR 0xC0000000

// 硬件设备ID
#define CDMA_DEVICE_ID XPAR_AXICDMA_0_DEVICE_ID
#define SEQ_TTC_DEVICE_ID XPAR_XTTCPS_2_DEVICE_ID
#define SEQ_TTC_INTR_ID XPAR_XTTCPS_2_INTR

// ================= 预计算参数结构体 =================
// 用于存储每一步计算好的硬件寄存器值，避免在中断中进行浮点运算
typedef struct
{
    u32 Din0_Value; // UB + UA
    u32 Din1_Value; // UX + UC
    u32 Din2_Value; // IB + IA
    u32 Din3_Value; // IX + IC
    u32 Freq_Divisor;
    u32 DO_State;     // 开出量状态
    u32 Ttc_Interval; // 定时器计数值
    u8 Ttc_Prescaler; // 定时器分频
} Step_Hw_Params_t;

// ================= 运行时状态结构体 =================
typedef struct
{
    int CurrentStepIndex;     // 当前步索引
    bool IsRunning;           // 运行标志
    u32 TotalSteps;           // 总步数
    u32 RepeatCountRemaining; // 剩余重复次数

    // 预计算的参数数组 (动态分配或静态大数组)
    Step_Hw_Params_t StepParams[MAX_SEQ_STEPS];
} StateSeq_Runtime_t;

extern StateSeq_Runtime_t g_StateSeqRuntime;
extern XTtcPs SeqTtcInstance;
extern XAxiCdma CdmaInstance;
// ================= 函数声明 =================
int StateSequence_Init(void);
void StateSequence_PrepareAndStart(void);
void StateSequence_Stop(void);
void StateSequence_OnAlarmTrigger(void);

// 中断处理
void StateSequence_TTC_Handler(void *CallBackRef);
void StateSequence_DI_Check(uint32_t changed_bits, uint32_t current_val);

void Test_StateSequence_Scenario(void);
#endif // STATE_SEQUENCE_H