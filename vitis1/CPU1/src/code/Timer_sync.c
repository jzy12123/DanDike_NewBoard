#include "Timer_sync.h"
#include "gps.h"
#include "ADDA.h"
#include "xil_printf.h"
#include "Msg_Que.h"
#include "soft_timer.h"
#include "8025IIC.h"
#include <stdio.h>
#include "xintc.h"

// --- 全局变量定义 ---
volatile TimeSyncManager_t g_TimeSyncManager;

// --- 静态辅助函数声明 ---
static void send_task_event(const char *result_str);
static int parse_and_set_manual_time(const char *time_str);

// Timeout definitions (in 0.5s ticks)
#define GPS_SYNC_TIMEOUT_TICKS 10   // 5秒
#define IRIGB_SYNC_TIMEOUT_TICKS 10 // 5秒
#define REPORT_INTERVAL_TICKS 2     // 1秒

/**
 * @brief 初始化对时管理器
 */
void TimeSync_Init(void)
{
    g_TimeSyncManager.current_mode = SYNC_MODE_NONE;
    g_TimeSyncManager.status = TIME_SYNC_IDLE;
    g_TimeSyncManager.timeout_ticks = 0;
    g_TimeSyncManager.report_ticks = 0;
}

/**
 * @brief 启动一个对时任务
 */
int StartSystemSync(SyncModeType mode, cJSON *data)
{
    if (g_TimeSyncManager.status == TIME_SYNC_IN_PROGRESS)
    {
        xil_printf("CPU1: System sync already in progress.\r\n");
        return -1;
    }

    g_TimeSyncManager.current_mode = mode;
    g_TimeSyncManager.status = TIME_SYNC_IN_PROGRESS;
    g_TimeSyncManager.report_ticks = REPORT_INTERVAL_TICKS; // 立即准备发送第一个"Doing"状态

    switch (mode)
    {
    case SYNC_MODE_GPS:
        xil_printf("CPU1: Starting GPS time synchronization...\r\n");
        g_TimeSyncManager.timeout_ticks = GPS_SYNC_TIMEOUT_TICKS;

        // 复位GPS接收逻辑并使能中断
        GPS_Ctrl_State.uart_cont = 0;
        memset((void *)UART_RX_BUF, 0, sizeof(UART_RX_BUF));
        XIntc_Enable(&AxiIntc_BareMetal, BAREMETAL_INTC_GPS_UART_INTR_ID);
        XScuGic_Enable(&intc, GPS_TTC_INT_IRQ_ID);
        break;

    case SYNC_MODE_IRIGB:
        xil_printf("CPU1: Starting IRIG-B time synchronization...\r\n");
        g_TimeSyncManager.timeout_ticks = IRIGB_SYNC_TIMEOUT_TICKS;

        // 开启软时钟的B码对时,上升沿
        Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG6, 0);
        Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG6, 1);
        break;

    // SNTP和手动都是手动对时
    case SYNC_MODE_MANUAL:
    case SYNC_MODE_SNTP:
    {
        xil_printf("CPU1: Starting Manual/SNTP time setting...\r\n");
        g_TimeSyncManager.timeout_ticks = 0; // 手动对时无超时
        cJSON *time_item = cJSON_GetObjectItem(data, "ManualTime");
        if (time_item && cJSON_IsString(time_item))
        {
            if (parse_and_set_manual_time(time_item->valuestring) == 0)
            {
                g_TimeSyncManager.status = TIME_SYNC_SUCCESS;
            }
            else
            {
                g_TimeSyncManager.status = TIME_SYNC_FAILURE;
            }
        }
        else
        {
            g_TimeSyncManager.status = TIME_SYNC_FAILURE;
        }
        // 手动模式是瞬间完成的，不发送TaskEvent
        g_TimeSyncManager.current_mode = SYNC_MODE_NONE;
        g_TimeSyncManager.status = TIME_SYNC_IDLE;
        break;
    }

    default:
        xil_printf("CPU1: Invalid sync mode requested.\r\n");
        g_TimeSyncManager.status = TIME_SYNC_IDLE;
        return -1;
    }

    return 0;
}

/**
 * @brief 获取当前对时任务的状态
 */
TimeSyncStatus GetSyncStatus(void)
{
    return g_TimeSyncManager.status;
}

/**
 * @brief 对时任务周期性处理器
 */
void TimeSync_TickHandler(void)
{
    if (g_TimeSyncManager.status != TIME_SYNC_IN_PROGRESS)
    {
        return;
    }

    // 处理状态上报
    if (g_TimeSyncManager.report_ticks > 0)
    {
        g_TimeSyncManager.report_ticks--;
        if (g_TimeSyncManager.report_ticks == 0)
        {
            send_task_event("Doing");
            g_TimeSyncManager.report_ticks = REPORT_INTERVAL_TICKS; // 重置为1秒后再次上报
        }
    }

    // 处理超时
    if (g_TimeSyncManager.timeout_ticks > 0)
    {
        g_TimeSyncManager.timeout_ticks--;
        if (g_TimeSyncManager.timeout_ticks == 0)
        {
            xil_printf("CPU1: Sync task timed out.\r\n");
            NotifySyncFailure();
        }
    }
}

/**
 * @brief 从外部事件通知对时任务成功
 */
void NotifySyncSuccess(void)
{
    if (g_TimeSyncManager.status != TIME_SYNC_IN_PROGRESS)
        return;

    xil_printf("CPU1: Sync task successful.\r\n");
    g_TimeSyncManager.status = TIME_SYNC_SUCCESS;
    send_task_event("Success");

    // 清理资源
    if (g_TimeSyncManager.current_mode == SYNC_MODE_GPS)
    {
        XIntc_Disable(&AxiIntc_BareMetal, BAREMETAL_INTC_GPS_UART_INTR_ID);
        XScuGic_Disable(&intc, GPS_TTC_INT_IRQ_ID);
    }
    if (g_TimeSyncManager.current_mode == SYNC_MODE_IRIGB)
    {
        // 关闭软时钟IP核B码对时功能
        Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG6, 0);
    }

    TimeSync_Init(); // 恢复到空闲状态
}

/**
 * @brief 从外部事件通知对时任务失败
 */
void NotifySyncFailure(void)
{
    if (g_TimeSyncManager.status != TIME_SYNC_IN_PROGRESS)
        return;

    xil_printf("CPU1: Sync task failed.\r\n");
    g_TimeSyncManager.status = TIME_SYNC_FAILURE;
    send_task_event("Failure");

    // 清理资源
    if (g_TimeSyncManager.current_mode == SYNC_MODE_GPS)
    {
        XIntc_Disable(&AxiIntc_BareMetal, BAREMETAL_INTC_GPS_UART_INTR_ID);
        XScuGic_Disable(&intc, GPS_TTC_INT_IRQ_ID);
    }
    if (g_TimeSyncManager.current_mode == SYNC_MODE_IRIGB)
    {
        // 关闭软时钟IP核B码对时功能
        Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG6, 0);
    }

    TimeSync_Init(); // 恢复到空闲状态
}

/**
 * @brief 发送TaskEvent上行消息
 */
static void send_task_event(const char *result_str)
{
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "TaskEvent");
    cJSON_AddStringToObject(report, "FunCode", "SetTaskSysTimeSync");
    cJSON_AddStringToObject(report, "Result", result_str);

    cJSON *data = cJSON_CreateObject();
    const char *mode_str = "Unknown";
    switch (g_TimeSyncManager.current_mode)
    {
    case SYNC_MODE_GPS:
        mode_str = "BD";
        break;
    case SYNC_MODE_IRIGB:
        mode_str = "IRIG-B";
        break;
    case SYNC_MODE_MANUAL:
        mode_str = "Manual";
        break;
    case SYNC_MODE_SNTP:
        mode_str = "SNTP";
        break;
    default:
        break;
    }
    cJSON_AddStringToObject(data, "SyncMode", mode_str);

    // 获取并添加当前设备时间
    char time_str[40];
    In_CurrTime current_time;
    read_current_time(&current_time);
    sprintf(time_str, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            current_time.curr_year, current_time.curr_month, current_time.curr_day,
            current_time.curr_hour, current_time.curr_minute, current_time.curr_second,
            (unsigned int)(current_time.curr_subsec / 10000));
    cJSON_AddStringToObject(data, "DevTime", time_str);

    cJSON_AddItemToObject(report, "Data", data);

    // 转换为字符串并发送
    char *string = cJSON_PrintUnformatted(report);
    if (string)
    {
        size_t stringLength = strlen(string);
        char *finalString = (char *)malloc(stringLength + 3);
        if (finalString)
        {
            snprintf(finalString, stringLength + 3, "|%s|", string);
            MsgQue_write(finalString, strlen(finalString));
            free(finalString);
        }
        free(string);
    }
    cJSON_Delete(report);
}

/**
 * @brief 解析 "YYYY-MM-DD HH:MM:SS.ms" 格式的时间字符串并设置系统时间
 */
static int parse_and_set_manual_time(const char *time_str)
{
    int year, month, day, hour, min, sec, ms;

    // 使用sscanf进行安全解析
    if (sscanf(time_str, "%d-%d-%d %d:%d:%d.%d", &year, &month, &day, &hour, &min, &sec, &ms) != 7)
    {
        xil_printf("CPU1: Manual time format error. Expected 'YYYY-MM-DD HH:MM:SS.ms'.\r\n");
        return -1;
    }

    // 填充软时钟结构体
    Out_RealTime time_to_set_soft;
    time_to_set_soft.year = year;
    time_to_set_soft.month = month;
    time_to_set_soft.day = day;
    time_to_set_soft.hour = hour;
    time_to_set_soft.min = min;
    time_to_set_soft.sec = sec;

    int weekday_iso = calculate_weekday_iso(year, month, day);
    time_to_set_soft.week = (weekday_iso > 0) ? (1 << (weekday_iso - 1)) : 1;
    time_to_set_soft.pps_clr_en = true;
    time_to_set_soft.bm_encode_en = true;
    time_to_set_soft.bm_decode_en = false;
    write_soft_timer(&time_to_set_soft);

    // 填充硬件RTC结构体
    RTC_Time_t time_to_set_rtc;
    time_to_set_rtc.year = (u8)(year % 100);
    time_to_set_rtc.month = (u8)month;
    time_to_set_rtc.day = (u8)day;
    time_to_set_rtc.hour = (u8)hour;
    time_to_set_rtc.min = (u8)min;
    time_to_set_rtc.sec = (u8)sec;
    time_to_set_rtc.week = (weekday_iso == 7) ? 0 : (u8)weekday_iso;

    if (Rtc8025_SetTime(RTC_AXI_IIC_BASEADDR, &time_to_set_rtc) != XST_SUCCESS)
    {
        xil_printf("CPU1: Manual time set failed for hardware RTC.\r\n");
        // 即使RTC设置失败，软时钟也已更新，所以不一定返回-1
    }

    xil_printf("CPU1: System time set manually to: %s\r\n", time_str);
    return 0;
}

/**
 * @brief 检查从RTC读取的时间是否有效
 * @param TimePtr 指向待检查的RTC时间结构体
 * @return 如果时间有效则返回true, 否则返回false
 */
bool is_rtc_time_valid(const RTC_Time_t *TimePtr)
{
    if (TimePtr->month < 1 || TimePtr->month > 12)
        return false;
    if (TimePtr->day < 1 || TimePtr->day > 31)
        return false;
    if (TimePtr->hour > 23)
        return false;
    if (TimePtr->min > 59)
        return false;
    if (TimePtr->sec > 59)
        return false;
    if (TimePtr->year > 99)
        return false; // 年份是两位数
    return true;
}

void Handler_BmSyncEnd(void *CallbackRef)
{
    // 1. 清除中断
    XIntc_Acknowledge(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_BM_SYN_END_INTR);

    // 2. 检查当前是否正在进行IRIG-B对时任务
    if (g_TimeSyncManager.current_mode == SYNC_MODE_IRIGB && g_TimeSyncManager.status == TIME_SYNC_IN_PROGRESS)
    {
        // 同步 RTC
        In_CurrTime curr_time;
        RTC_Time_t rtc_time;
        int iso_week = 0;

        // A. 从IP核读取刚刚同步好的高精度时间
        read_current_time(&curr_time);

        // B. 转换格式为 RTC 结构体
        rtc_time.year = (u8)(curr_time.curr_year % 100); // 2025 -> 25
        rtc_time.month = (u8)curr_time.curr_month;
        rtc_time.day = (u8)curr_time.curr_day;
        rtc_time.hour = (u8)curr_time.curr_hour;
        rtc_time.min = (u8)curr_time.curr_minute;
        rtc_time.sec = (u8)curr_time.curr_second;

        // C. 星期转换：软时钟独热码(One-Hot) -> RTC数值(0=Sun, 1=Mon...6=Sat)
        // 假设 curr_week: bit0=Mon ... bit6=Sun
        for (int i = 0; i < 7; i++)
        {
            if ((curr_time.curr_week >> i) & 0x01)
            {
                iso_week = i + 1; // 1=Mon...7=Sun
                break;
            }
        }
        // RX8025 星期通常定义: 0=Sun, 1=Mon, ..., 6=Sat (需根据实际驱动确认，此处兼容 main.c 逻辑)
        rtc_time.week = (iso_week == 7) ? 0 : (u8)iso_week;

        // D. 写入硬件 RTC
        // 注意：RTC_AXI_IIC_BASEADDR 宏在您的 Timer_sync.c 中应已通过头文件可见
        int status = Rtc8025_SetTime(RTC_AXI_IIC_BASEADDR, &rtc_time);

        if (status == XST_SUCCESS)
        {
            xil_printf("CPU1: [ISR] IRIG-B Sync -> RTC Updated: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
                       rtc_time.year, rtc_time.month, rtc_time.day,
                       rtc_time.hour, rtc_time.min, rtc_time.sec);
        }
        else
        {
            xil_printf("CPU1: [ISR] IRIG-B Sync -> RTC Write Failed!\r\n");
        }
        // -----------------------------------------------------

        // 3. 硬件已完成对时，通知系统成功 (清理软时钟B码使能等)
        NotifySyncSuccess();
    }
}

/**
 * @brief 日期更新中断处理函数 (Pin 8)
 * @note 每天 00:00:00 触发，负责更新软时钟日期
 */
void Handler_DateUpdate(void *CallbackRef)
{
    // 1. 清除中断
    XIntc_Acknowledge(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_DATE_UPDATE_INTR);

    xil_printf("CPU1: [ISR] Date Update Triggered at 00:00:00\r\n");

    // 2. 读取当前时间以获取当前日期
    In_CurrTime curr;
    read_current_time(&curr);

    // 3. 计算下一天的日期
    int year = curr.curr_year;
    int month = curr.curr_month;
    int day = curr.curr_day;

    int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 闰年判断
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        days_in_month[2] = 29;
    }

    day++;
    if (day > days_in_month[month])
    {
        day = 1;
        month++;
        if (month > 12)
        {
            month = 1;
            year++;
        }
    }

    // 4. 计算相关的辅助数据 (星期, 年积日)
    int wk_iso = calculate_weekday_iso(year, month, day);
    int wk_onehot = (wk_iso > 0) ? (1 << (wk_iso - 1)) : 1; // 转换为独热码
    int year_day_val = calculate_year_day(month, day, year);

    // 5. 构造写入软时钟寄存器的数据 (参考 soft_timer.c 的写入逻辑)

    // 构造 slv_reg0: [17:10]年BCD | [9:0]年日BCD
    uint8_t yd_d0 = year_day_val % 10;
    uint8_t yd_d1 = (year_day_val / 10) % 10;
    uint8_t yd_d2 = (year_day_val / 100) % 10;
    uint16_t bcd_yearday = (uint16_t)(yd_d2 << 8) | (yd_d1 << 4) | yd_d0;
    uint32_t reg0 = ((uint32_t)int_to_bcd_byte(year % 100) << 10) | (bcd_yearday & 0x3FF);

    // 构造 slv_reg1: [17:13]月BCD | [12:7]日BCD | [6:0]星期
    uint32_t reg1 = ((uint32_t)int_to_bcd_byte(month) << 13) |
                    ((uint32_t)int_to_bcd_byte(day) << 7) |
                    (wk_onehot & 0x7F);

    // 6. 写入寄存器并产生更新脉冲
    // 注意：只更新日期相关寄存器，不触碰 Reg2(时间)，只脉冲 Reg3(wr_date)
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG0, reg0);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG1, reg1);

    // 脉冲 wr_date (slv_reg3[0])
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG3, 0x01);
    // 必要的延时，确保时钟域同步
    for (volatile int i = 0; i < 100; i++)
        ;
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG3, 0x00);
}