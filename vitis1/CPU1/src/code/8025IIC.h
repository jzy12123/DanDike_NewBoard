#ifndef RTC8025_AXI_IIC_H_
#define RTC8025_AXI_IIC_H_

#include "xil_types.h"
#include "xil_printf.h"
#include "xstatus.h"
#include "sleep.h"
#include "xiic_l.h"      // 低层级 AXI IIC 驱动寄存器定义
#include "xparameters.h" // 用于获取硬件参数

#ifndef XIIC_SR_NAK_MASK
#define XIIC_SR_NAK_MASK 0x00000080U /**< NAK (Slave Not Acknowledging) mask */
#endif

#ifndef XIIC_SR_ARB_LOST_MASK
#define XIIC_SR_ARB_LOST_MASK 0x00000010U /**< Arbitration Lost mask */
#endif

// --- Base Address for AXI IIC controller used for RX8025 ---
// 比如 XPAR_AXI_IIC_0_BASEADDR
#define RTC_AXI_IIC_BASEADDR XPAR_RTC_EEPROM_AXI_IIC_0_BASEADDR 

// --- RX8025 Device Definitions ---
#define RTC8025_SLAVE_ADDR 0x32 // RX8025 的 7 位 I2C 从设备地址 (0x64 >> 1)

// --- RX8025 Register Addresses ---
#define RTC8025_REG_SEC 0x00
#define RTC8025_REG_MIN 0x01
#define RTC8025_REG_HOUR 0x02
#define RTC8025_REG_WEEK 0x03
#define RTC8025_REG_DAY 0x04
#define RTC8025_REG_MONTH 0x05
#define RTC8025_REG_YEAR 0x06
#define RTC8025_REG_CTRL1 0x0E
#define RTC8025_REG_CTRL2 0x0F

// --- Timeout and Clock Definitions ---
#define IIC_TIMEOUT_COUNT 1000000     // 增加超时计数，给慢速操作或调试更多时间
#define S_AXI_ACLK_FREQ_HZ 100000000  // *重要: ZYNQ PL端给AXI IIC IP核的AXI Lite接口时钟频率 (你的共用100MHz)
#define I2C_SCL_TARGET_FREQ_HZ 100000 // 目标I2C SCL频率 (e.g., 100kHz standard mode)

// AXI IIC Register Offsets (参考 Xilinx PG091 AXI IIC Bus Interface v2.1)
// 这些通常由 xiic_l.h 中的宏提供，但明确列出有助于理解
// #define XIIC_CR_REG_OFFSET    0x100  // Control Register already in xiic_l.h
// #define XIIC_SR_REG_OFFSET    0x104  // Status Register already in xiic_l.h
// #define XIIC_TX_FIFO_OFFSET   0x108  // TX FIFO already in xiic_l.h
// #define XIIC_RX_FIFO_OFFSET   0x10C  // RX FIFO already in xiic_l.h
// #define XIIC_ADR_REG_OFFSET   0x110  // Address Register (not typically used with xiic_l send/recv)
// #define XIIC_TFO_REG_OFFSET   0x114  // TX FIFO Occupancy (l?nda)
// #define XIIC_RFO_REG_OFFSET   0x118  // RX FIFO Occupancy (l?nda)
#define XIIC_TX_DTR_REG_OFFSET 0x0108 // Data Transmit Register (Same as TX_FIFO_OFFSET)
#define XIIC_RX_DRR_REG_OFFSET 0x010C // Data Receive Register (Same as RX_FIFO_OFFSET)

#define AXI_IIC_DYN_CTRL_REG_OFFSET 0x0128 // Dynamic Control Register (PG091 v2.1) - 可能没有这个
                                           // 时钟分频通常在0x0120 (TX_CLK) 和 0x0124 (RX_CLK)

// TX_CLK_REG (Transmit Clock Register) offset for SCL generation
// 查阅你的 AXI IIC IP Core 版本文档确定准确偏移量，PG091 v2.1 是 0x0120
#define AXI_IIC_TX_CLK_REG_OFFSET 0x0120

// --- BCD Conversion Macros ---
#define BCD_TO_DEC(val) ((((val) & 0xF0) >> 4) * 10 + ((val) & 0x0F))
#define DEC_TO_BCD(val) ((((val) / 10) << 4) + ((val) % 10))

// --- Time structure for RX8025 ---
typedef struct
{
    u8 sec;   // 秒 (0-59)
    u8 min;   // 分 (0-59)
    u8 hour;  // 时 (0-23, 24小时制)
    u8 week;  // 星期 (0-6, 0=周日, 1=周一 ... 6=周六 for this struct)
    u8 day;   // 日 (1-31)
    u8 month; // 月 (1-12)
    u8 year;  // 年 (0-99, 代表 2000-2099)
} RTC_Time_t;

// --- Function Declarations ---
// int Rtc8025_Init(u32 BaseAddr);
int Rtc8025_WriteReg(u32 BaseAddr, u8 RegAddr, u8 Value);
int Rtc8025_ReadReg(u32 BaseAddr, u8 RegAddr, u8 *ValuePtr);
int Rtc8025_SetTime(u32 BaseAddr, const RTC_Time_t *TimePtr);
int Rtc8025_GetTime(u32 BaseAddr, RTC_Time_t *TimePtr);

#endif /* RTC8025_AXI_IIC_H_ */