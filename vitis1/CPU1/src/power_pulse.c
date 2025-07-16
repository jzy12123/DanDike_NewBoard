// power_pulse.c

#include "power_pulse.h"
#include "xscugic.h" // 用于中断清除
#include "xil_io.h"

EnergyTest_t g_EnergyTest_P; // P通道测试状态
EnergyTest_t g_EnergyTest_Q; // Q通道测试状态

// 全局变量，用于存储电能脉冲的配置和状态
volatile PowerPulse_t g_PowerPulse = {
    .pulseConstantP = 7200, // 默认值
    .pulseConstantQ = 7200, // 默认值
    .measuredPowerP = 0.0,
    .measuredPowerQ = 0.0};

/**
 * @brief 初始化电能脉冲IP核
 * @details 使能IP核的读和写功能。
 */
void PowerPulse_Init(void)
{
    // 中文注释: 打开读有功(P)和无功(Q)脉冲的使能
    Xil_Out32(POWER_PULSE_BASE + PP_REG_READ_ENABLE, 0x3);

    // 中文注释: 打开写有功(P)和无功(Q)脉冲的使能
    Xil_Out32(POWER_PULSE_BASE + PP_REG_WRITE_ENABLE, 0x3);

    printf("Power pulse module initialized.\n");
}

/**
 * @brief 根据功率计算并更新输出脉冲
 * @param total_p_watts 总有功功率 (单位: W)
 * @param total_q_var   总无功功率 (单位: var)
 * @details 核心公式: 脉冲周期(秒) = 3600 / 电表常数(imp/kWh) / 负载功率(kW)
 * 脉宽计数值 = 脉冲周期 / 2 / 10ns
 */
void PowerPulse_UpdateOutput(double total_p_watts, double total_q_var)
{
    double power_p_kw = total_p_watts / 1000.0; // 从 W 转换为 kW
    double power_q_kvar = total_q_var / 1000.0; // 从 var 转换为 kvar

    uint32_t pulse_width_p = 0;
    uint32_t pulse_width_q = 0;

    // --- 计算有功脉冲宽度 ---
    // 中文注释: 确保功率为正且脉冲常数不为零，以避免除零错误
    if (power_p_kw > 0.000001 && g_PowerPulse.pulseConstantP > 0)
    {
        // 中文注释: 计算脉冲周期 (单位: 秒)
        double period_p_s = 3600.0 / (double)g_PowerPulse.pulseConstantP / power_p_kw;

        // 中文注释: 计算高/低电平时间 (单位: 秒)，即周期的一半
        double half_period_p_s = period_p_s / 2.0;

        // 中文注释: 将高/低电平时间转换为10ns为单位的计数值 (1秒 = 100,000,000个10ns)
        pulse_width_p = (uint32_t)(half_period_p_s * 100000000.0);
    }

    // --- 计算无功脉冲宽度 ---
    if (power_q_kvar > 0.000001 && g_PowerPulse.pulseConstantQ > 0)
    {
        double period_q_s = 3600.0 / (double)g_PowerPulse.pulseConstantQ / power_q_kvar;
        double half_period_q_s = period_q_s / 2.0;
        pulse_width_q = (uint32_t)(half_period_q_s * 100000000.0);
    }

    // 中文注释: 将计算出的脉宽值写入硬件寄存器
    Xil_Out32(POWER_PULSE_BASE + PP_REG_P_DATA, pulse_width_p);
    Xil_Out32(POWER_PULSE_BASE + PP_REG_Q_DATA, pulse_width_q);
}

// 中文注释: 用于存储上一次读取到的脉冲宽度，以便检测变化
static uint32_t last_pulse_width_p = 0;
static uint32_t last_pulse_width_q = 0;

/**
 * @brief 轮询方式读取电能脉冲输入
 * @details 此函数应在主循环中被周期性调用。
 * 它通过比较当前和上一次的脉宽计数值来检测新脉冲的到来。
 */
void PowerPulse_PollInput(void)
{
    // --- 轮询有功(P)通道 ---
    uint32_t current_pulse_width_p = Xil_In32(POWER_PULSE_BASE + PP_REG_P_DATA);

    // 中文注释: 如果当前读取的脉宽与上次不同，说明一个新脉冲已被硬件测量
    if (current_pulse_width_p != last_pulse_width_p)
    {
        // 中文注释: 更新上一次的脉宽值
        last_pulse_width_p = current_pulse_width_p;

        // 中文注释: 根据新的脉宽值计算功率
        if (current_pulse_width_p > 0 && g_PowerPulse.pulseConstantP > 0)
        {
            double half_period_s = (double)current_pulse_width_p / 100000000.0;
            double period_s = half_period_s * 2.0;
            g_PowerPulse.measuredPowerP = 3600.0 / (double)g_PowerPulse.pulseConstantP / period_s;
        }
        else
        {
            g_PowerPulse.measuredPowerP = 0.0;
        }
        printf("CPU1: Polling detected new P pulse. Width_count=%u, Measured Power: %.6f kW\n",
               (unsigned int)current_pulse_width_p, g_PowerPulse.measuredPowerP);
    }

    // --- 轮询无功(Q)通道 ---
    uint32_t current_pulse_width_q = Xil_In32(POWER_PULSE_BASE + PP_REG_Q_DATA);

    if (current_pulse_width_q != last_pulse_width_q)
    {
        last_pulse_width_q = current_pulse_width_q;

        if (current_pulse_width_q > 0 && g_PowerPulse.pulseConstantQ > 0)
        {
            double half_period_s = (double)current_pulse_width_q / 100000000.0;
            double period_s = half_period_s * 2.0;
            g_PowerPulse.measuredPowerQ = 3600.0 / (double)g_PowerPulse.pulseConstantQ / period_s;
        }
        else
        {
            g_PowerPulse.measuredPowerQ = 0.0;
        }
        printf("CPU1: Polling detected new Q pulse. Width_count=%u, Measured Power: %.6f kvar\n",
               (unsigned int)current_pulse_width_q, g_PowerPulse.measuredPowerQ);
    }
}

/**
 * @brief 有功脉冲输入中断处理函数
 * @param CallbackRef 回调引用 (未使用)
 */
void PowerPulse_P_IntrHandler(void *CallbackRef)
{
    // 中文注释: 从硬件寄存器读取测得的脉宽计数值
    uint32_t pulse_width_p = Xil_In32(POWER_PULSE_BASE + PP_REG_P_DATA);

    // 中文注释: 根据脉宽值计算功率
    if (pulse_width_p > 0 && g_PowerPulse.pulseConstantP > 0)
    {
        // 中文注释: 从计数值反推高/低电平时间 (秒)
        double half_period_s = (double)pulse_width_p / 100000000.0;
        // 中文注释: 计算完整周期
        double period_s = half_period_s * 2.0;
        // 中文注释: 根据公式反推功率 (kW)
        g_PowerPulse.measuredPowerP = 3600.0 / (double)g_PowerPulse.pulseConstantP / period_s;
    }
    else
    {
        g_PowerPulse.measuredPowerP = 0.0;
    }

    // printf("CPU1: P pulse received. Measured Power: %.4f kW\r\n", g_PowerPulse.measuredPowerP);

    // --- 新增：电能误差测试逻辑 ---
    if (g_EnergyTest_P.isActive && g_EnergyTest_P.testMode == 'P')
    {
        process_energy_pulse(&g_EnergyTest_P, lineAC.totalP / 1000.0);
    }
}

/**
 * @brief 无功脉冲输入中断处理函数
 * @param CallbackRef 回调引用 (未使用)
 */
void PowerPulse_Q_IntrHandler(void *CallbackRef)
{
    uint32_t pulse_width_q = Xil_In32(POWER_PULSE_BASE + PP_REG_Q_DATA);

    if (pulse_width_q > 0 && g_PowerPulse.pulseConstantQ > 0)
    {
        double half_period_s = (double)pulse_width_q / 100000000.0;
        double period_s = half_period_s * 2.0;
        g_PowerPulse.measuredPowerQ = 3600.0 / (double)g_PowerPulse.pulseConstantQ / period_s;
    }
    else
    {
        g_PowerPulse.measuredPowerQ = 0.0;
    }

    // printf("CPU1: Q pulse received. Measured Power: %.4f kvar\r\n", g_PowerPulse.measuredPowerQ);

    // --- 新增：电能误差测试逻辑 ---
    if (g_EnergyTest_Q.isActive && g_EnergyTest_Q.testMode == 'Q')
    {
        process_energy_pulse(&g_EnergyTest_Q, lineAC.totalQ / 1000.0);
    }
}

/**
 * @brief 处理电能脉冲，用于误差测试
 * @param test_state 指向当前测试（P或Q）的状态结构体
 * @param measured_power_kw Zynq系统当前测量的功率（kW或kvar）
 */
void process_energy_pulse(volatile EnergyTest_t *test_state, double measured_power_kw)
{
    test_state->currentPulseCount++;

    // 如果是本轮的第一个脉冲，记录开始时间
    if (test_state->currentPulseCount == 1)
    {
        read_current_time((In_CurrTime *)&test_state->roundStartTime);
    }

    uint32_t total_pulses_for_one_round = test_state->targetRounds * test_state->freqDivFactor;

    // 如果累计脉冲数达到一轮的目标值
    if (test_state->currentPulseCount >= total_pulses_for_one_round)
    {
        In_CurrTime round_end_time;
        read_current_time(&round_end_time);

        // 1. 计算时间差
        double time_elapsed = time_diff_seconds(&round_end_time, (const In_CurrTime *)&test_state->roundStartTime);
        if (time_elapsed <= 0)
        {                                      // 防止无效时间差
            test_state->currentPulseCount = 0; // 重启本轮计数
            return;
        }

        // 2. 计算测量能量 (E_meas)
        double e_meas_kwh = measured_power_kw * (time_elapsed / 3600.0);

        // 3. 计算标准能量 (E_std)
        double e_std_kwh = (double)total_pulses_for_one_round / (double)test_state->pulseConstant;

        // 4. 计算误差
        if (e_std_kwh > 1e-12)
        { // 避免除以零
            double error = ((e_meas_kwh - e_std_kwh) / e_std_kwh) * 100.0;
            if (test_state->currentTestNum < 99)
            {
                test_state->errs[test_state->currentTestNum] = error;
            }
        }

        test_state->currentTestNum++;

        // 5. 检查测试是否全部完成
        const char *result_str;
        if (test_state->currentTestNum >= test_state->targetTimes)
        {
            result_str = "Success";
            test_state->isActive = false; // 结束测试
        }
        else
        {
            result_str = "Doing";
        }

        // 6. 上报 TaskEvent
        extern void report_energy_test_event(const volatile EnergyTest_t *test_state, const char *result);
        report_energy_test_event(test_state, result_str);

        // 7. 重置脉冲计数器，准备下一轮
        test_state->currentPulseCount = 0;
    }
}
/**
 * @brief 新增：初始化电能误差测试状态
 */
void init_EnergyTest(void)
{
    memset((void *)&g_EnergyTest_P, 0, sizeof(EnergyTest_t));
    memset((void *)&g_EnergyTest_Q, 0, sizeof(EnergyTest_t));
    g_EnergyTest_P.isActive = false;
    g_EnergyTest_Q.isActive = false;
}