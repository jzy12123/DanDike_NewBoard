/*
 * Amplifier_Switch.h
 *
 *  Created on: 2024年2月1日
 *      Author: saber
 */

#ifndef AMPLIFIER_SWITCH_H_
#define AMPLIFIER_SWITCH_H_

#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"
#include "sleep.h"
#include "stdlib.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "ADDA.h"
#include "mutex_utils.h"
#include "Communications_Protocol.h"
#include "xttcps.h" // TTC驱动头文件
#include "xpseudo_asm_gcc.h"
#include "cJSON.h"		// <--- 新增: 用于创建JSON
#include "Msg_Que.h"	// <--- 新增: 用于发送消息
#include "soft_timer.h" // <--- 新增: 用于获取年月日
// Zynq-7000 SLCR (System Level Control Registers) 的基地址
#define SLCR_BASE_ADDR 0xF8000000

// TTC 时钟控制寄存器在 SLCR 内的偏移
// XPAR_XTTCPS_0_DEVICE_ID 对应 TTC0, XPAR_XTTCPS_1_DEVICE_ID 对应 TTC1
#define TTC0_CLK_CTRL_OFFSET 0x0170 // 对应TTC0, TTC1
#define TTC1_CLK_CTRL_OFFSET 0x0174 // 对应TTC2, TTC3 (Zynq有两个TTC模块，每个模块包含3个定时器)

// 防抖定时器
#define DEBOUNCE_TIMER_DEVICE_ID XPAR_XTTCPS_0_DEVICE_ID // 假设使用TTC0
#define DEBOUNCE_TIMER_IRPT_INTR XPAR_XTTCPS_0_INTR		 // TTC0的中断ID
#define DEBOUNCE_TIME_10MS (10.0)						 // 防抖时间 10ms

// IP核基地址
#define Amplifier_OnOff_BASEADDR XPAR_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_BASEADDR
// 模块
#define Amplifier_Status_ADDR (14 * 4) // REG14
#define Amplifier_Din0_ADDR (9 * 4)	   // REG9
#define Amplifier_Din1_ADDR (10 * 4)   // REG10
#define Amplifier_Din2_ADDR (11 * 4)   // REG11
#define Amplifier_Din3_ADDR (12 * 4)   // REG12
// 读故障
#define RdSerial_Status_ADDR (15 * 4) // REG15
#define RdSerial_ADDR (10 * 4)		  // REG10
// 开关量
#define OnOff_Status_ADDR (8 * 4)		   // REG8
#define OnOff_Write_ADDR (13 * 4)		   // REG13
#define OnOff_Read_ADDR (9 * 4)			   // REG9
#define LATCH_TIME_HMS_REG_OFFSET (11 * 4) // slv_reg11: 存储时、分、秒的BCD码
#define LATCH_DAYSEC_REG_OFFSET (12 * 4)   // slv_reg12: 存储日内秒 (二进制)
#define LATCH_SUBSEC_REG_OFFSET (13 * 4)   // slv_reg13: 存储亚秒 (二进制)
// 中断
#define OnOffDone_INTR_ID 87U
// 定义开关量位数
typedef enum
{
	bit_8 = 0,
	bit_16 = 1,
	bit_24 = 2,
	bit_32 = 3
} Read_Bit;
extern Read_Bit g_onoff_bit_width; //声明全局位宽变量
// 定义开入功能
typedef enum
{
	Off = 0,
	One_ReadWrite = 1,
	Random_ReadWrite = 2,
	Time_ReadWrite = 3
} Operation_Mode;
// 定义功放使能枚举
typedef enum
{
	POWAMP_OFF = 0,
	POWAMP_ON = 1
} POWER_AMP_STATE;
// 用于保存锁存的时间戳的结构体
typedef struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint32_t day_sec;
	uint32_t sub_sec;
} OnOff_Timestamp_t;

extern XTtcPs DebounceTimer;	 // TTC定时器实例
extern float g_debounce_time_ms; // <--- 新增：声明全局防抖时间变量

void power_amplifier_control(float Wave_Amplitude[], u32 Wave_Range[], uint8_t pid_state, uint8_t enable_amp);
void RdSerial();
unsigned char voltage_to_output(float voltage);
unsigned char current_to_output(float current);
// 计算校准参数（线性拟合两个校准点）
double calculate_correction(int channel, int range_idx, float amplitude_percentage);

// 开关量
u32 invert_Binary(u32 num);
void OnOff_Start(Read_Bit bit_width, uint8_t start);
void OnOff_Stop();
void OnOff_Write_Continuous(uint32_t output_data);
void OnOff_Read_LatchedData(Read_Bit bit_width, uint32_t *read_data, volatile OnOff_Timestamp_t *timestamp);
u32 OnOff_Read_Current_Input(Read_Bit bit_width);
void onoff_handler(void);
void report_di_soe_event(uint32_t stable_data, uint32_t changed_bits, const volatile OnOff_Timestamp_t *timestamp);
// 防抖定时器
int debounce_timer_init();
void debounce_timer_handler(void *CallBackRef);
#endif /* SRC_AMPLIFIER_SWITCH_H_ */
