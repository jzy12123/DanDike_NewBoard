#include "gps_sync.h"
#include "gps.h"
#include "ADDA.h" // 为了引用 GPS_UARTLITE_INT_IRQ_ID, GPS_TTC_INT_IRQ_ID
#include "xil_printf.h"

// 全局对时状态变量定义
volatile TimeSyncStatus g_gps_sync_status = TIME_SYNC_IDLE;

// 10秒超时计数器 (假设主定时器是0.5秒一次)
// 10s / 0.5s = 20
#define GPS_SYNC_TIMEOUT_TICKS 20
static volatile int g_gps_sync_timeout_counter = 0;

/**
 * @brief 启动GPS对时流程
 * @return 0 如果成功启动, -1 如果当前已有对时任务在进行中
 */
int StartGpsTimeSync(void)
{
    // 检查是否已有一个任务在运行
    if (g_gps_sync_status == TIME_SYNC_IN_PROGRESS)
    {
        xil_printf("CPU1: GPS time sync already in progress.\r\n");
        return -1;
    }

    xil_printf("CPU1: Starting GPS time synchronization (10s window)...\r\n");

    // 1. 设置状态和超时计数器
    g_gps_sync_status = TIME_SYNC_IN_PROGRESS;
    g_gps_sync_timeout_counter = GPS_SYNC_TIMEOUT_TICKS;

    // 2. 复位GPS接收逻辑
    GPS_Ctrl_State.uart_cont = 0;
    memset((void *)UART_RX_BUF, 0, sizeof(UART_RX_BUF));
    GPS_Ctrl_State.REV_Finish_Flag = 0;

    // --- 【关键的冗余配置，关闭竞态条件窗口】 ---
    // 在每次使能中断之前，都强制重新设置中断的目标CPU。
    // 这确保了即使Linux因为某些原因重置了GIC分配器，我们的设置也会被立即恢复。

    //! a. 强制将GPS UART中断(IRQ 66)的目标设置为CPU1
    XScuGic_InterruptMaptoCpu(&intc, CPU1_ID, GPS_UARTLITE_INT_IRQ_ID);

    //! b. 强制将GPS TTC定时器中断的目标设置为CPU1
    XScuGic_InterruptMaptoCpu(&intc, CPU1_ID, GPS_TTC_INT_IRQ_ID);
    // --- 【冗余配置结束】 ---

    // 3. 使能GPS相关的中断
    XScuGic_Enable(&intc, GPS_UARTLITE_INT_IRQ_ID);
    XScuGic_Enable(&intc, GPS_TTC_INT_IRQ_ID);

    return 0;
}

/**
 * @brief 获取当前GPS对时任务的状态
 * @return TimeSyncStatus 当前的状态
 */
TimeSyncStatus GetGpsTimeSyncStatus(void)
{
    return g_gps_sync_status;
}

/**
 * @brief 对时任务超时处理函数
 * @note  这个函数应该在一个周期性定时器（例如主定时器）中被调用
 */
void TimeSync_TimeoutHandler(void)
{

    if (g_gps_sync_timeout_counter > 0)
    {
        g_gps_sync_timeout_counter--;

        if (g_gps_sync_timeout_counter == 0)
        {
            // 定时器到期，说明10秒超时
            xil_printf("CPU1: GPS sync timed out after 10 seconds.\r\n");

            // 设置状态为失败
            g_gps_sync_status = TIME_SYNC_FAILURE;

            // 禁能GPS中断，停止处理
            XScuGic_Disable(&intc, GPS_UARTLITE_INT_IRQ_ID);
            XScuGic_Disable(&intc, GPS_TTC_INT_IRQ_ID);
        }
    }
}
/**
 * @brief 停止GPS对时超时定时器 (新增的函数实现)
 * @note  通过将计数器清零来停止倒计时。
 */
void StopGpsTimeSyncTimer(void)
{
    g_gps_sync_timeout_counter = 0;
}