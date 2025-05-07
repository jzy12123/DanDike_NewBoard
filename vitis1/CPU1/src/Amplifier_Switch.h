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
#include "Communications_Protocol.h"

#define Amplifier_Switch_BASEADDR XPAR_AC_8_CHANNEL_0_ONOFF_CONFIG_AXI_0_BASEADDR
#define Module_Status_ADDR		0

#define Amplifier_Din0_ADDR 	4
#define Amplifier_Din1_ADDR 	8
#define Amplifier_Din2_ADDR 	12
#define Amplifier_Din3_ADDR		16

#define Switch_Write_ADDR		20
#define Switch_Read_ADDR		4

#define RdSerial_ADDR			8

enum Write_Read{
	bit_8 = 0,
	bit_16,
	bit_24,
	bit_32
};
//定义功放使能枚举
typedef enum
{
	POWAMP_OFF = 0,
	POWAMP_ON = 1
} POWER_AMP_STATE;


u32 invert_Binary (u32 num);
void Write_Read_Switch(u32 Data_width,u32 Data);
void power_amplifier_control(float Wave_Amplitude[], u32 Wave_Range[], uint8_t pid_state, uint8_t enable_amp);
void RdSerial();
unsigned char voltage_to_output(float voltage);
unsigned char current_to_output(float current);
// 计算校准参数（线性拟合两个校准点）
double calculate_correction(int channel, int range_idx, float amplitude_percentage);
#endif /* SRC_AMPLIFIER_SWITCH_H_ */
