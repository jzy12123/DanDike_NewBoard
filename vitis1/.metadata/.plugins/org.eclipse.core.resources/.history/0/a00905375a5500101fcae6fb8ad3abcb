#include "mutex_utils.h"

// 全局锁变量定义
volatile uint8_t g_adc_dac_resource_lock = LOCK_OWNER_NONE;

/**
 * @brief 尝试获取资源锁
 * @param attempted_owner_id 尝试获取锁的操作方ID (LOCK_OWNER_ADC 或 LOCK_OWNER_DAC)
 * @param timeout_us 超时时间 (微秒)
 * @return 1 如果成功获取锁, 0 如果超时或失败
 * @comment 此函数会尝试原子地获取锁。如果锁已被占用，则会等待直到锁被释放或超时。
 */
int acquire_resource_lock(uint8_t attempted_owner_id, uint32_t timeout_us)
{
    uint32_t wait_time = 0;
    const uint32_t poll_interval_us = 1000; // 轮询间隔1ms

    // 使用GCC内置的原子操作 __sync_bool_compare_and_swap
    // 该函数尝试原子地比较 g_adc_dac_resource_lock 与 LOCK_OWNER_NONE，
    // 如果相等，则将其设置为 attempted_owner_id，并返回 true。否则返回 false。
    while (1)
    {
        if (__sync_bool_compare_and_swap(&g_adc_dac_resource_lock, LOCK_OWNER_NONE, attempted_owner_id))
        {
            // xil_printf("CPU1: Lock acquired by owner %d.\n", attempted_owner_id); // 调试输出
            return 1; // 成功获取锁
        }

        // 如果锁已被占用 (CAS失败)
        if (wait_time >= timeout_us)
        {
            // xil_printf("CPU1: Timeout acquiring lock for attempting_owner %d. Lock currently held by %d. Waited for %lu us.\n", attempted_owner_id, g_adc_dac_resource_lock, wait_time);
            return 0; // 获取锁失败 (超时)
        }

        usleep(poll_interval_us); // 等待一小段时间再尝试
        wait_time += poll_interval_us;
    }
}

/**
 * @brief 释放资源锁
 * @param current_owner_id 当前持有锁的操作方ID
 * @comment 此函数会原子地释放锁，仅当调用者是当前锁的持有者时。
 */
void release_resource_lock(uint8_t current_owner_id)
{
    // 确保只有锁的当前持有者才能释放它
    if (g_adc_dac_resource_lock == current_owner_id)
    {
        if (!__sync_bool_compare_and_swap(&g_adc_dac_resource_lock, current_owner_id, LOCK_OWNER_NONE))
        {
            // 如果CAS失败，说明在检查和尝试释放之间锁的状态发生了意外变化。
            // 这在单核且无抢占的简单场景下不太可能发生，但为了稳健性可以添加日志。
            xil_printf("CPU1: Error! Lock was unexpectedly changed during release by owner %d. Current state: %d\n", current_owner_id, g_adc_dac_resource_lock); // 调试输出
        }
        else
        {
            // xil_printf("CPU1: Lock released by owner %d.\n", current_owner_id); // 调试输出
        }
    }
    else if (g_adc_dac_resource_lock == LOCK_OWNER_NONE)
    {
        xil_printf("CPU1: Warning - Attempt to release an already unlocked lock by owner %d.\n", current_owner_id); // 调试输出
    }
    else
    {
        xil_printf("CPU1: Warning - Owner %d attempting to release lock currently held by %d.\n", current_owner_id, g_adc_dac_resource_lock); // 调试输出
    }
}