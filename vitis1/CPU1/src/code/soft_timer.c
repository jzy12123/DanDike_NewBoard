#include "soft_timer.h"
#include "xil_printf.h"
#include "sleep.h"                   // 包含 usleep (如果是裸机环境) 或 <unistd.h> (如果是Linux用户空间)
#include "StateSequence.h"           // g_StateSeqRuntime
#include "Communications_Protocol.h" // g_WaveRecordTask
#include "WaveRecord.h"              // WaveRecord_OnAlarmIRQ
#include "ReplayWave.h"              // ReplayWave_OnAlarmIRQ

// --- 全局影子寄存器定义 ---
uint32_t g_SoftTimer_Reg5_Shadow = 0;  // bm_encode_en
uint32_t g_SoftTimer_Reg6_Shadow = 0;  // bm_decode_en
uint32_t g_SoftTimer_Reg7_Shadow = 0;  // pps_clr_en
uint32_t g_SoftTimer_Reg15_Shadow = 0; // rdserial	enable和定时功能
/**
 * @brief 将一个0-99的整数转换为BCD码 (存储在一个字节中)
 */
uint8_t int_to_bcd_byte(uint8_t value)
{
    if (value > 99)
        value = 99;
    return ((value / 10) << 4) | (value % 10);
}

/**
 * @brief 将一个字节的BCD码转换为整数
 */
uint8_t bcd_to_int_byte(uint8_t bcd_value)
{
    return (((bcd_value >> 4) & 0x0F) * 10) + (bcd_value & 0x0F);
}

/**
 * @brief 计算给定日期是一年中的第几天
 */
int calculate_year_day(int month, int day, int year)
{
    int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12 || day < 1)
        return 0;
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        days_in_month[2] = 29;
    }
    if (day > days_in_month[month])
        return 0;
    int year_day_val = 0;
    for (int i = 1; i < month; i++)
    {
        year_day_val += days_in_month[i];
    }
    year_day_val += day;
    return year_day_val;
}

/**
 * @brief 计算给定日期的星期几 (ISO 8601标准: 1 = 周一, ..., 7 = 周日)
 */
int calculate_weekday_iso(int y, int m, int d)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    int day_of_week = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; // 0 for Sunday
    if (day_of_week == 0)
        day_of_week = 7; // Convert Sunday from 0 to 7
    return day_of_week;
}

/**
 * @brief 写入时间数据到软时钟IP核
 */
void write_soft_timer(Out_RealTime *time_data)
{
    uint32_t reg_val_0, reg_val_1, reg_val_2;
    int full_year = time_data->year; // 假设传入的已经是四位年份

    int year_day_val = calculate_year_day(time_data->month, time_data->day, full_year);
    if (year_day_val == 0)
    {
        // xil_printf("SoftTimer Write Error: Invalid date for year_day (%d-%d-%d)\r\n", full_year, time_data->month, time_data->day);
        return;
    }

    uint8_t bcd_yy = int_to_bcd_byte(full_year % 100);
    uint8_t yd_d0 = year_day_val % 10;
    uint8_t yd_d1 = (year_day_val / 10) % 10;
    uint8_t yd_d2 = (year_day_val / 100) % 10;
    uint16_t final_bcd_year_day = (uint16_t)(yd_d2 << 8) | (yd_d1 << 4) | yd_d0;

    reg_val_0 = (uint32_t)(bcd_yy << 10) | (final_bcd_year_day & 0x03FF);

    reg_val_1 = (uint32_t)(int_to_bcd_byte(time_data->month) << 13) |
                (uint32_t)(int_to_bcd_byte(time_data->day) << 7) |
                (time_data->week & 0x7F);

    reg_val_2 = (uint32_t)(int_to_bcd_byte(time_data->hour) << 14) |
                (uint32_t)(int_to_bcd_byte(time_data->min) << 7) |
                (uint32_t)(int_to_bcd_byte(time_data->sec));

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG0, reg_val_0);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG1, reg_val_1);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG2, reg_val_2);

    // 更新影子变量，并写入硬件
    g_SoftTimer_Reg5_Shadow = (time_data->bm_encode_en ? 1 : 0);
    g_SoftTimer_Reg6_Shadow = (time_data->bm_decode_en ? 1 : 0);
    g_SoftTimer_Reg7_Shadow = (time_data->pps_clr_en ? 1 : 0);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG5, g_SoftTimer_Reg5_Shadow);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG6, g_SoftTimer_Reg6_Shadow);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG7, g_SoftTimer_Reg7_Shadow);

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG3, 1); // wr_date = 1
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG4, 1); // wr_time = 1

    usleep(10); // 稍作延时确保IP核捕获，1us可能不够，10-100us更稳妥

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG3, 0); // wr_date = 0
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG4, 0); // wr_time = 0
}

/**
 * @brief 读取软时钟IP核的当前时间
 */
/**
 * @brief 读取软时钟IP核的当前时间 (增加防翻转回读校验)
 */
void read_current_time(In_CurrTime *Curr_Time)
{
    uint32_t reg_data3, reg_data4, reg_data5, reg_data6, reg_data7;
    uint32_t reg_data6_verify; // 用于校验的一致性变量

    // -------------------------------------------------------------------------
    // 关键修正: 循环读取直到 DaySec (REG6) 在读取 SubSec (REG7) 前后保持一致
    // 这能防止在读取间隙发生进位导致的时间错乱
    // -------------------------------------------------------------------------
    do
    {
        reg_data6 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG6);        // 读取 DaySec
        reg_data7 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG7);        // 读取 SubSec
        reg_data6_verify = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG6); // 再次读取 DaySec
    } while (reg_data6 != reg_data6_verify);

    // 其他寄存器变化较慢，通常直接读取即可 (为了极致严谨，也可对 reg3/4/5 加类似校验)
    reg_data3 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG3);
    reg_data4 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG4);
    reg_data5 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG5);

    // 赋值逻辑保持不变
    Curr_Time->curr_year = 2000 + bcd_to_int_byte((reg_data3 >> 10) & 0xFF);
    uint16_t bcd_yd_read = reg_data3 & 0x03FF;
    uint8_t yd_r_d0 = bcd_yd_read & 0xF;
    uint8_t yd_r_d1 = (bcd_yd_read >> 4) & 0xF;
    uint8_t yd_r_d2 = (bcd_yd_read >> 8) & 0x3;
    Curr_Time->curr_yearday = (uint16_t)yd_r_d2 * 100 + (uint16_t)yd_r_d1 * 10 + yd_r_d0;

    Curr_Time->curr_month = bcd_to_int_byte((reg_data4 >> 13) & 0x1F);
    Curr_Time->curr_day = bcd_to_int_byte((reg_data4 >> 7) & 0x3F);
    Curr_Time->curr_week = reg_data4 & 0x7F;

    Curr_Time->curr_hour = bcd_to_int_byte((reg_data5 >> 14) & 0x3F);
    Curr_Time->curr_minute = bcd_to_int_byte((reg_data5 >> 7) & 0x7F);
    Curr_Time->curr_second = bcd_to_int_byte(reg_data5 & 0x7F);

    Curr_Time->curr_daysec = reg_data6 & 0x1FFFF;
    Curr_Time->curr_subsec = reg_data7 & 0xFFFFFF;
}

/**
 * @brief 读取B码时间
 */
void read_bm_time(In_BmTime *Bm_Time)
{
    uint32_t reg0 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG0);
    uint32_t reg1 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG1);
    uint32_t reg2 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG2);

    Bm_Time->bm_year = 2000 + bcd_to_int_byte((reg0 >> 10) & 0xFF);
    uint16_t bcd_bm_yd = reg0 & 0x03FF;
    uint8_t bm_yd_r_d0 = bcd_bm_yd & 0xF;
    uint8_t bm_yd_r_d1 = (bcd_bm_yd >> 4) & 0xF;
    uint8_t bm_yd_r_d2 = (bcd_bm_yd >> 8) & 0x3;
    Bm_Time->bm_yearday = (uint16_t)bm_yd_r_d2 * 100 + (uint16_t)bm_yd_r_d1 * 10 + bm_yd_r_d0;

    Bm_Time->bm_hour = bcd_to_int_byte((reg1 >> 14) & 0x3F);
    Bm_Time->bm_minute = bcd_to_int_byte((reg1 >> 7) & 0x7F);
    Bm_Time->bm_second = bcd_to_int_byte(reg1 & 0x7F);
    Bm_Time->bm_daysec = reg2 & 0x1FFFF; // Assuming binary
}

/**
 * @brief 计算两个 In_CurrTime 结构体之间的时间差（单位：秒）
 * @param end_time 结束时间戳
 * @param start_time 开始时间戳
 * @return double类型的时间差（秒）
 */
double time_diff_seconds(const In_CurrTime *end_time, const In_CurrTime *start_time)
{
    // 亚秒部分基于10MHz时钟 (1个单位 = 100ns = 0.1us)
    const double subsec_per_second = 10000000.0;

    // 计算总的日内秒（整数部分）
    double total_seconds_end = (double)end_time->curr_daysec;
    double total_seconds_start = (double)start_time->curr_daysec;

    // 计算总的亚秒（小数部分）
    double total_subsec_end = (double)end_time->curr_subsec / subsec_per_second;
    double total_subsec_start = (double)start_time->curr_subsec / subsec_per_second;

    double diff = (total_seconds_end + total_subsec_end) - (total_seconds_start + total_subsec_start);

    // 处理跨天情况 (假设测试时间不会超过一天)
    if (diff < 0)
    {
        diff += 86400.0; // 一天的秒数
    }

    return diff;
}

/**
 * @brief 软时钟闹钟中断处理函数 (统一分发入口)
 * @details 硬件产生高电平中断 -> 进入此函数 -> 关闭使能(清除中断源) -> 分发任务
 */
void SoftTimer_AlarmHandler(void *CallBackRef)
{
    // 1. 清除硬件中断源 (关闭 Alarm Enable)
    g_SoftTimer_Reg15_Shadow &= ~STIMER_ALARM_EN_MASK;
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG15, g_SoftTimer_Reg15_Shadow);

    // xil_printf("CPU1: [IRQ] SoftTimer Alarm Triggered!\r\n");

    // 2. 任务分发逻辑

    // 优先级 1: 检查是否有 独立录波任务 (SetTaskWaveRecord) 在等待定时
    if (g_WaveRecordTask.State == 1)
    {
        WaveRecord_OnAlarmIRQ();
        return; // 响应完毕，退出
    }

    // 优先级 2: 波形回放任务 (SetTaskWaveReplayStart) 在等待定时
    if (g_ReplayRuntime.isWaiting)
    {
        ReplayWave_OnAlarmIRQ();
        return;
    }

    // 优先级 3: 状态序列任务 (SetTaskStateSequence)
    if (!g_StateSeqRuntime.IsRunning)
    {
        StateSequence_ApplyAndRun(); // 应用配置并运行
    }
    else
    {
        // 如果状态序列已经在运行，通常忽略此次定时启动，或者打个日志
        // xil_printf("CPU1: [IRQ] Alarm ignored because StateSequence is already running.\r\n");
    }
}

/**
 * @brief 单独控制 PPS 清零使能 (不影响时间写入)
 */
void SoftTimer_SetPPS_Clr_En(bool enable)
{
    // 只修改影子寄存器的对应位 (目前REG7只有bit0，直接赋值即可，若有多位需位操作)
    g_SoftTimer_Reg7_Shadow = enable ? 1 : 0;
    // 将影子值写入硬件
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG7, g_SoftTimer_Reg7_Shadow);
}

/**
 * @brief 单独控制 B码输出使能
 */
void SoftTimer_SetBmEncode_En(bool enable)
{
    g_SoftTimer_Reg5_Shadow = enable ? 1 : 0;
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG5, g_SoftTimer_Reg5_Shadow);
}

/**
 * @brief 单独控制 B码解码使能
 */
void SoftTimer_SetBmDecode_En(bool enable)
{
    g_SoftTimer_Reg6_Shadow = enable ? 1 : 0;
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG6, g_SoftTimer_Reg6_Shadow);
}