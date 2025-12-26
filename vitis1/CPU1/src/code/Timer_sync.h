#ifndef TIMER_SYNC_H_
#define TIMER_SYNC_H_

#include "xil_types.h"
#include "xscugic.h" // 需要 GIC 实例的引用
#include "cJSON.h"   // 用于构建上报的JSON
#include "8025IIC.h"
#include "stdbool.h"
#include "soft_timer.h"
// 对时模式枚举
typedef enum
{
    SYNC_MODE_NONE,   // 无
    SYNC_MODE_GPS,    // GPS/BD 对时
    SYNC_MODE_IRIGB,  // IRIG-B 对时 (待实现)
    SYNC_MODE_MANUAL, // 手动对时
    SYNC_MODE_SNTP    // SNTP对时 (由ARM侧视为手动)
} SyncModeType;

// 对时任务状态枚举
typedef enum
{
    TIME_SYNC_IDLE,        // 空闲状态
    TIME_SYNC_IN_PROGRESS, // 正在进行中
    TIME_SYNC_SUCCESS,     // 成功
    TIME_SYNC_FAILURE      // 失败 (超时或错误)
} TimeSyncStatus;

// 对时管理器结构体
typedef struct
{
    SyncModeType current_mode;
    volatile TimeSyncStatus status;
    volatile int timeout_ticks;   // 超时倒计时 (单位: 0.5秒)
    volatile int report_ticks;    // 状态上报倒计时 (单位: 0.5秒)
} TimeSyncManager_t;

// --- 全局变量声明 ---
extern volatile TimeSyncManager_t g_TimeSyncManager; // 全局对时管理器
extern XScuGic intc;                                 // 从ADDA.c中引用的中断控制器实例

// --- 函数声明 ---

/**
 * @brief 初始化对时管理器
 */
void TimeSync_Init(void);

/**
 * @brief 启动一个对时任务
 * @param mode 要启动的对时模式
 * @param data 可选的、与模式相关的数据 (例如手动对时的时间字符串)
 * @return 0 如果成功启动, -1 如果当前已有任务在进行中或模式无效
 */
int StartSystemSync(SyncModeType mode, cJSON *data);

/**
 * @brief 获取当前对时任务的状态
 * @return TimeSyncStatus 当前的状态
 */
TimeSyncStatus GetSyncStatus(void);

/**
 * @brief 对时任务周期性处理器
 * @note  此函数应在主定时器中断(0.5s)中被周期性调用。
 * 负责处理超时和周期性状态上报。
 */
void TimeSync_TickHandler(void);

/**
 * @brief 从外部事件通知对时任务成功
 * @note  例如由GPS模块在成功解析到时间后调用
 */
void NotifySyncSuccess(void);

/**
 * @brief 从外部事件通知对时任务失败
 * @note  例如GPS模块发现硬件错误时调用
 */
void NotifySyncFailure(void);

bool is_rtc_time_valid(const RTC_Time_t *TimePtr);

void Handler_BmSyncEnd(void *CallbackRef);
void Handler_DateUpdate(void *CallbackRef);
void send_task_event(const char *result_str);
#endif /* TIMER_SYNC_H_ */