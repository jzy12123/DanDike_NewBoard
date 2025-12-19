#ifndef MUTEX_UTILS_H
#define MUTEX_UTILS_H

#include "xil_types.h"
#include "sleep.h"
#include "xil_printf.h"

// 锁的状态宏定义
#define LOCK_OWNER_NONE 0 // 锁未被占用
#define LOCK_OWNER_ADC 1  // 锁被ADC操作占用
#define LOCK_OWNER_DAC 2  // 锁被DAC/功放操作占用

extern volatile uint8_t g_adc_dac_resource_lock;

// 获取锁的统一超时时间 (微秒)
#define MUTEX_ADC_ACQUIRE_TIMEOUT_US 350000
#define MUTEX_DAC_ACQUIRE_TIMEOUT_US 350000
int acquire_resource_lock(uint8_t attempted_owner_id, uint32_t timeout_us);
void release_resource_lock(uint8_t current_owner_id);

#endif // MUTEX_UTILS_H