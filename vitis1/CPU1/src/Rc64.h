#ifndef RC64_H
#define RC64_H

// --- Standard and Xilinx Includes ---
#include "xparameters.h" // 用于获取硬件参数，如基地址和设备ID
#include "xil_types.h"   // 定义 u8, u16, u32 等类型
#include "xil_printf.h"  // 用于打印调试信息
#include "xstatus.h"     // 定义 XST_SUCCESS, XST_FAILURE 等状态码
#include "sleep.h"       // 提供 usleep 函数 (用于延时)
#include "xiic_l.h"      // 低层级 AXI IIC 驱动寄存器定义
#include <string.h>      // 提供 memcpy 函数

// --- Base Address Definition ---
// 使用 xparameters.h 中定义的 AXI IIC 控制器基地址
#define IIC_BASE_ADDRESS XPAR_IIC_0_BASEADDR

// --- EEPROM Device Definitions ---
#define I2C_DEVICE_ID XPAR_IIC_0_DEVICE_ID // AXI IIC 设备ID (低层级驱动可能不直接使用)
#define EEPROM_ADDRESS 0x50                // EEPROM 的 7 位 I2C 从设备地址
#define EEPROM_PAGE_SIZE 32                // EEPROM 的页大小 (字节)

// --- Timeout and Retry Definitions ---
// 用于 I2C 操作的超时和重试参数 (需要根据实际硬件和时钟速度调整)
#define IIC_TIMEOUT_COUNT 100000           // 等待总线空闲或操作完成的超时计数值
#define EEPROM_WRITE_ACK_POLL_RETRIES 500  // EEPROM 写操作后轮询 ACK 的最大重试次数
#define EEPROM_WRITE_ACK_POLL_DELAY_US 500 // EEPROM 写操作后轮询 ACK 的间隔 (微秒)
#define EEPROM_ADDR_SEND_RETRIES 100       // EEPROM 读操作时发送地址的最大重试次数
#define EEPROM_ADDR_SEND_DELAY_US 100      // EEPROM 读操作时发送地址的重试间隔 (微秒)

// --- Array Dimensions ---
// 校准数据数组的维度
#define ROWS 8
#define COLS 3
#define DOUBLE_SIZE sizeof(double) // double 类型的大小

// --- Calculated Sizes ---
// 计算出的数组元素数量和总字节数
#define ARRAY_ELEMENT_COUNT (ROWS * COLS)
#define ARRAY_BYTES (ARRAY_ELEMENT_COUNT * DOUBLE_SIZE) // 每个校准数组的总字节数 (8*3*8 = 192 bytes)

// --- EEPROM Address Mapping ---
// 定义校准数据在 EEPROM 中的存储起始地址
#define EEPROM_ADDR_DA_CORRECT_100 (0)                                            // 第一个数组的起始地址
#define EEPROM_ADDR_DA_CORRECT_20 (EEPROM_ADDR_DA_CORRECT_100 + ARRAY_BYTES)      // 第二个数组的起始地址
#define EEPROM_ADDR_DA_CORRECTPHASE_100 (EEPROM_ADDR_DA_CORRECT_20 + ARRAY_BYTES) // 第三个数组的起始地址
#define EEPROM_ADDR_AD_CORRECT (EEPROM_ADDR_DA_CORRECTPHASE_100 + ARRAY_BYTES)    // 第四个数组的起始地址
#define EEPROM_TOTAL_CALIB_BYTES (EEPROM_ADDR_AD_CORRECT + ARRAY_BYTES)           // 校准数据占用的总字节数

// --- Type Definition for EEPROM Internal Address ---
// 定义 EEPROM 内部地址的数据类型 (通常为 1 或 2 字节)
// 对于大于 256 字节的 EEPROM，通常需要 16 位地址
typedef u16 AddressType; // 将 EEPROM 内部地址类型定义为 u16 (2字节)

// --- Function Declarations ---

/**
 * @brief 初始化 IIC 硬件 (低层级)
 * @return XST_SUCCESS 如果成功，否则返回 XST_FAILURE
 */
int RC64_Init(void);

/**
 * @brief 从 EEPROM 读取所有校准数据
 * @return XST_SUCCESS 如果读取成功，否则返回 XST_FAILURE
 */
int RC64_ReadCalibData(void);

/**
 * @brief 将所有校准数据写入 EEPROM
 * @return XST_SUCCESS 如果写入成功，否则返回 XST_FAILURE
 */
int RC64_WriteCalibData(void);

/**
 * @brief 低层级函数，将缓冲区数据写入 IIC EEPROM，处理页写逻辑，并包含超时处理
 * @param Address EEPROM 中的起始地址
 * @param BufferPtr 指向要写入数据的缓冲区
 * @param ByteCount 要写入的总字节数
 * @return XST_SUCCESS 如果成功，XST_FAILURE 如果失败
 */
int EepromWriteData(AddressType Address, u8 *BufferPtr, u16 ByteCount);

/**
 * @brief 低层级函数，从 IIC EEPROM 读取数据到缓冲区，并包含超时处理
 * @param Address EEPROM 中的起始地址
 * @param BufferPtr 指向存储读取数据的缓冲区
 * @param ByteCount 要读取的字节数
 * @return XST_SUCCESS 如果成功，XST_FAILURE 如果失败
 */
int EepromReadData(AddressType Address, u8 *BufferPtr, u16 ByteCount);

#endif /* RC64_H */