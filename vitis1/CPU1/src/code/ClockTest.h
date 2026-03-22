#ifndef CLOCK_TEST_H_
#define CLOCK_TEST_H_

#include "xil_types.h"
#include "cJSON.h"
#include "soft_timer.h" // 需要引用软时钟相关定义
#include "stdbool.h"
#include "ADDA.h"


// 最大存储的误差数量
#define MAX_CLOCK_TEST_ERRS 99

// 时钟误差测试结构体
typedef struct
{
    volatile bool isActive;     // 是否正在测试
    volatile bool isFirstPulse; // 是否是当前轮次的第一个脉冲

    uint32_t plusSeconds;  // 几秒一个脉冲 (标准间隔)
    uint32_t targetRounds; // 多少个脉冲算一轮 (Round)
    uint32_t targetTimes;  // 总共测几次 (TestTimes)

    volatile uint32_t currentPulseCount; // 当前轮次已接收的脉冲数
    volatile uint32_t currentTestNum;    // 当前已完成的测试次数 (TestedTimes)

    double errs[MAX_CLOCK_TEST_ERRS]; // 误差数组 (秒/天)

    In_CurrTime roundStartTime; // 当前轮次开始的时间戳
} ClockTest_t;

// 用于主循环上报的“信箱”结构体
typedef struct
{
    char result[16];              // "Doing" or "Success"
    volatile bool report_pending; // 是否有待发送的报告
    ClockTest_t snapshot;         // 数据快照
} ClockTestReport_t;

// 全局变量声明
extern volatile ClockTest_t g_ClockTest;
extern ClockTestReport_t g_ClockReportData;

// 函数声明
void ClockTest_Init(void);
bool ClockTest_Start(uint32_t plusSeconds, uint32_t round, uint32_t testTimes);
void ClockTest_Terminate(void);
void ClockTest_PPS_IntrHandler(void *CallbackRef); // 中断服务函数
void ClockTest_CheckAndReport(void);               // 主循环调用

#endif /* CLOCK_TEST_H_ */