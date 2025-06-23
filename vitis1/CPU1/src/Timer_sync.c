#include "Timer_sync.h"
#include "gps.h"
#include "ADDA.h"
#include "xil_printf.h"
#include "Msg_Que.h"
#include "soft_timer.h"
#include "8025IIC.h"
#include <stdio.h>

// --- 全局变量定义 ---
volatile TimeSyncManager_t g_TimeSyncManager;

// --- 静态辅助函数声明 ---
static void send_task_event(const char *result_str);
static int parse_and_set_manual_time(const char *time_str);

// Timeout definitions (in 0.5s ticks)
#define GPS_SYNC_TIMEOUT_TICKS 10  // 5秒
#define IRIGB_SYNC_TIMEOUT_TICKS 6 // 3秒
#define REPORT_INTERVAL_TICKS 2    // 1秒

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
        XScuGic_InterruptMaptoCpu(&intc, CPU1_ID, GPS_UARTLITE_INT_IRQ_ID);
        XScuGic_InterruptMaptoCpu(&intc, CPU1_ID, GPS_TTC_INT_IRQ_ID);
        XScuGic_Enable(&intc, GPS_UARTLITE_INT_IRQ_ID);
        XScuGic_Enable(&intc, GPS_TTC_INT_IRQ_ID);
        break;

    case SYNC_MODE_IRIGB:
        xil_printf("CPU1: Starting IRIG-B time synchronization...\r\n");
        g_TimeSyncManager.timeout_ticks = IRIGB_SYNC_TIMEOUT_TICKS;
        // TODO: 在此添加启动IRIG-B硬件解码和中断的逻辑
        // 暂时直接设置为失败
        NotifySyncFailure();
        break;

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
        XScuGic_Disable(&intc, GPS_UARTLITE_INT_IRQ_ID);
        XScuGic_Disable(&intc, GPS_TTC_INT_IRQ_ID);
    }
    // TODO: 添加其他模式的清理逻辑

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
        XScuGic_Disable(&intc, GPS_UARTLITE_INT_IRQ_ID);
        XScuGic_Disable(&intc, GPS_TTC_INT_IRQ_ID);
    }
    // TODO: 添加其他模式的清理逻辑

    TimeSync_Init(); // 恢复到空闲状态
}

/**
 * @brief 发送TaskEvent上行消息
 */
static void send_task_event(const char *result_str)
{
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "TaskEvent");
    cJSON_AddStringToObject(report, "FunCode", "SetSysTimeSyncMode");
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