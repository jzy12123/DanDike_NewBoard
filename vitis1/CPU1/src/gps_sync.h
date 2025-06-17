#ifndef GPS_SYNC_H_
#define GPS_SYNC_H_

#include "xil_types.h"
#include "xscugic.h" // 需要 GIC 实例的引用

// 对时任务状态枚举
typedef enum
{
    TIME_SYNC_IDLE,        // 空闲状态
    TIME_SYNC_IN_PROGRESS, // 正在进行中
    TIME_SYNC_SUCCESS,     // 成功
    TIME_SYNC_FAILURE      // 失败 (超时)
} TimeSyncStatus;

// 全局变量声明
extern volatile TimeSyncStatus g_gps_sync_status; // GPS对时状态
extern XScuGic intc;                              // 从ADDA.c中引用的中断控制器实例

// 函数声明
/**
 * @brief 启动GPS对时流程
 * @return 0 如果成功启动, -1 如果当前已有对时任务在进行中
 */
int StartGpsTimeSync(void);

/**
 * @brief 获取当前GPS对时任务的状态
 * @return TimeSyncStatus 当前的状态
 */
TimeSyncStatus GetGpsTimeSyncStatus(void);

/**
 * @brief 对时任务超时处理函数
 * @note  这个函数应该在一个周期性定时器（例如主定时器）中被调用
 */
void TimeSync_TimeoutHandler(void);
/**
 * @brief 停止GPS对时超时定时器
 * @note  当外部事件（如成功接收GPS）发生时，调用此函数来提前中止10秒超时。
 */
void StopGpsTimeSyncTimer(void); // <-- 新增的函数声明
#endif /* GPS_SYNC_H_ */