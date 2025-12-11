#ifndef STATE_SEQUENCE_H
#define STATE_SEQUENCE_H

#include "Communications_Protocol.h"
#include "xaxicdma.h"
#include "xttcps.h"
#include "xparameters.h"
#include "cJSON.h"

// 状态序列相关结构体定义
#define MAX_SEQ_STEPS 50
#define MAX_SEQ_HARMS 32
// 最大状态序列回报数
#define MAX_SEQ_RESULTS 200

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

// ================= 结果记录结构体 =================
typedef struct
{
    int StateID;        // [新增] 步号 (从1开始计数)
    bool Triged;        // 是否提前跳转
    double Duration;    // 实际执行时长 (ms)
    u32 DI_State;       // 跳转时刻的DI状态 (32位位图)
    char TimeStamp[32]; // 该步结束的时刻 (可选，协议虽未明确要求每步时间，但调试有用)
} Seq_Step_Result_t;
// ================= 运行时状态结构体 =================
typedef struct
{
    int CurrentStepIndex;     // 当前步索引
    bool IsRunning;           // 运行标志
    bool IsHolding;           // [新增] 保持标志 (序列结束，保持最后一步输出，等待新指令)
    bool IsFinished;          // 标记是否刚刚完成（用于发送最后一次Success报告）
    u32 TotalSteps;           // 总步数
    u32 RepeatCountRemaining; // 剩余重复次数

    // --- 新增：上报相关 ---
    char StartTimeStr[32];                          // 整个序列的启动时间字符串
    int ExecutedCount;                              // 已执行的总步数 (作为写指针)
    int ReportedCount;                              // 已上报的步数 (作为读指针)
    Seq_Step_Result_t ExecResults[MAX_SEQ_RESULTS]; // 结果循环/线性缓冲区

    Step_Hw_Params_t StepParams[MAX_SEQ_STEPS]; // 预计算的参数数组 (动态分配或静态大数组)
    // 硬件参数缓存 (用于分离计算与执行) 在 Plan 阶段计算好，在 Run 阶段直接写入，不再耗时计算
    struct
    {
        u32 Range_Regs[4]; // r_din0 ~ r_din3 (量程)
        u32 Value_Regs[4]; // v_din0 ~ v_din3 (满幅值系数)
        u32 Init_Freq_Div; // 初始频率分频系数
    } Cached_Hw;
} StateSeq_Runtime_t;

extern StateSeq_Runtime_t g_StateSeqRuntime;
extern XTtcPs SeqTtcInstance;
extern XAxiCdma CdmaInstance;
// ================= 函数声明 =================
int StateSequence_Init(void);
void StateSequence_Plan(const char *startTimeStr); // 仅计算，不运行
void StateSequence_ApplyAndRun(void);              // 仅运行，不计算
void StateSequence_Stop(void);
void StateSequence_QuitMode(void);                       // [新增] 强制退出状态序列模式 (供 SetACS 等调用)
int StateSequence_EnableAlarm(const char *startTimeStr); // 解析时间并设置闹钟

// 中断处理
void StateSequence_AlarmHandler(void *CallBackRef); // 闹钟中断处理函数
void StateSequence_TTC_Handler(void *CallBackRef);
void StateSequence_DI_Check(uint32_t changed_bits, uint32_t current_val);
void check_and_report_state_sequence_status(void);

#endif // STATE_SEQUENCE_H