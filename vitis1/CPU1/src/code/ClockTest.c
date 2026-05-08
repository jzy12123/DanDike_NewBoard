#include "ClockTest.h"
#include "xscugic.h"
#include "xil_printf.h"
#include "Msg_Que.h"
#include <string.h>
#include <stdio.h>

// 全局变量定义
volatile ClockTest_t g_ClockTest;
ClockTestReport_t g_ClockReportData;

/**
 * @brief 初始化时钟测试模块
 */
void ClockTest_Init(void)
{
    memset((void *)&g_ClockTest, 0, sizeof(ClockTest_t));
    g_ClockTest.isActive = false;

    memset(&g_ClockReportData, 0, sizeof(ClockTestReport_t));
    g_ClockReportData.report_pending = false;

    printf("CPU1: ClockTest module initialized.\r\n");
}

/**
 * @brief 启动时钟误差测试
 */
bool ClockTest_Start(uint32_t plusSeconds, uint32_t round, uint32_t testTimes)
{
    if (g_ClockTest.isActive)
    {
        printf("CPU1: ClockTest Warning: Test already running, restarting.\r\n");
        ClockTest_Terminate(); // 先停止旧的
    }

    // 参数校验
    if (plusSeconds == 0 || round == 0 || testTimes == 0 || testTimes > MAX_CLOCK_TEST_ERRS)
    {
        printf("CPU1: ClockTest Error: Invalid parameters.\r\n");
        return false;
    }

    // 初始化状态
    memset((void *)&g_ClockTest, 0, sizeof(ClockTest_t));
    g_ClockTest.plusSeconds = plusSeconds;
    g_ClockTest.targetRounds = round;
    g_ClockTest.targetTimes = testTimes;
    g_ClockTest.isFirstPulse = true;
    g_ClockTest.isActive = true;

    // 使能 PPS 中断
    // XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_PPS_IN2CPU_INTR);
    printf("CPU1: ClockTest Started. Pulse=%us, Round=%u, Times=%u\r\n",
           (unsigned int)plusSeconds, (unsigned int)round, (unsigned int)testTimes);

    return true;
}

/**
 * @brief 终止时钟误差测试
 */
void ClockTest_Terminate(void)
{
    g_ClockTest.isActive = false;
    // 禁用 PPS 中断以节省资源
    // XIntc_Disable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_PPS_IN2CPU_INTR);
    printf("CPU1: ClockTest Terminated.\r\n");
}

/**
 * @brief PPS 中断处理函数 (需要注册到 ID 9)
 */
void ClockTest_PPS_IntrHandler(void *CallbackRef)
{
    // 1. 只有在测试激活时才处理
    if (!g_ClockTest.isActive)
    {
        return;
    }

    // 2. 立即读取当前软时钟时间
    In_CurrTime now;
    read_current_time(&now);

    // 3. 状态机逻辑
    if (g_ClockTest.isFirstPulse)
    {
        // 第一轮的第一个脉冲，仅记录其实时间，不计数
        // 因为我们需要测量的是两个脉冲之间的 *间隔*
        memcpy((void *)&g_ClockTest.roundStartTime, &now, sizeof(In_CurrTime));
        g_ClockTest.isFirstPulse = false;
        g_ClockTest.currentPulseCount = 0;
        // printf("CPU1: ClockTest First Pulse Captured.\r\n");
    }
    else
    {
        g_ClockTest.currentPulseCount++;

        // 4. 判断当前一轮 (Round) 是否完成
        if (g_ClockTest.currentPulseCount >= g_ClockTest.targetRounds)
        {

            // 计算时间差 (实测时间)
            double time_elapsed = time_diff_seconds(&now, (const In_CurrTime *)&g_ClockTest.roundStartTime);

            // 计算标准时间
            double time_standard = (double)g_ClockTest.targetRounds * (double)g_ClockTest.plusSeconds;

            // 计算误差 (秒/天)
            // 公式: ( (实测 - 标准) / 标准 ) * 86400
            double error_sec_per_day = 0.0;
            if (time_standard > 0.000001)
            {
                error_sec_per_day = ((time_elapsed - time_standard) / time_standard) * 86400.0;
            }

            // 记录误差
            if (g_ClockTest.currentTestNum < MAX_CLOCK_TEST_ERRS)
            {
                g_ClockTest.errs[g_ClockTest.currentTestNum] = error_sec_per_day;
            }

            g_ClockTest.currentTestNum++;

            // 打印调试
            // printf("CPU1: Round Done. Elapsed=%.6fs, Std=%.6fs, Err=%.4f s/d\r\n",
            //       time_elapsed, time_standard, error_sec_per_day);

            // 5. 判断是否所有测试 (TestTimes) 都完成
            if (g_ClockTest.currentTestNum >= g_ClockTest.targetTimes)
            {
                // 全部完成
                g_ClockTest.isActive = false;
                ClockTest_Terminate(); // 关中断
                strcpy(g_ClockReportData.result, "Success");
            }
            else
            {
                // 仅本轮完成，继续下一轮
                strcpy(g_ClockReportData.result, "Doing");
                // 重置下一轮的起始时间为当前时间
                memcpy((void *)&g_ClockTest.roundStartTime, &now, sizeof(In_CurrTime));
                g_ClockTest.currentPulseCount = 0;
            }

            // 6. 触发上报 (复制快照)
            memcpy(&g_ClockReportData.snapshot, (void *)&g_ClockTest, sizeof(ClockTest_t));
            g_ClockReportData.report_pending = true;
        }
    }
}

/**
 * @brief 检查并上报测试状态 (在 Main Loop 中调用)
 */
void ClockTest_CheckAndReport(void)
{
    if (!g_ClockReportData.report_pending)
    {
        return;
    }

    // 构建 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "FunType", "TaskEvent");
    cJSON_AddStringToObject(root, "FunCode", "SetTaskClockTest");
    cJSON_AddStringToObject(root, "Result", g_ClockReportData.result);

    cJSON *data = cJSON_CreateObject();
    cJSON *chns = cJSON_CreateArray();
    cJSON *chn1 = cJSON_CreateObject();

    cJSON_AddNumberToObject(chn1, "Chn", 1);
    cJSON_AddNumberToObject(chn1, "Round", g_ClockReportData.snapshot.currentPulseCount); // 协议要求当前正在测试圈数
    cJSON_AddNumberToObject(chn1, "TestedTimes", g_ClockReportData.snapshot.currentTestNum);

    // Completed 标志: 如果 Result 是 Success，则 Completed 为 true
    bool completed = (strcmp(g_ClockReportData.result, "Success") == 0);
    cJSON_AddBoolToObject(chn1, "Completed", completed);

    // 填充误差数组
    cJSON *errs = cJSON_CreateArray();
    for (int i = 0; i < g_ClockReportData.snapshot.currentTestNum; i++)
    {
        cJSON_AddItemToArray(errs, cJSON_CreateNumber(g_ClockReportData.snapshot.errs[i]));
    }
    cJSON_AddItemToObject(chn1, "Errs", errs);

    cJSON_AddItemToArray(chns, chn1);
    cJSON_AddItemToObject(data, "Chns", chns);
    cJSON_AddItemToObject(root, "Data", data);

    // 发送
    char *str = cJSON_PrintUnformatted(root);
    if (str)
    {
        size_t len = strlen(str);
        char *finalStr = malloc(len + 3);
        if (finalStr)
        {
            snprintf(finalStr, len + 3, "|%s|", str);
            MsgQue_write(finalStr, strlen(finalStr));
            free(finalStr);
        }
        free(str);
    }
    cJSON_Delete(root);

    // 清除标志
    g_ClockReportData.report_pending = false;
}
