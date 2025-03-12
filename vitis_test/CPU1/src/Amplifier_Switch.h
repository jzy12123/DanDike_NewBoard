/*
 * Amplifier_Switch.h
 *
 *  Created on: 2024年2月1日
 *      Author: saber
 */

#ifndef SRC_AMPLIFIER_SWITCH_H_
#define SRC_AMPLIFIER_SWITCH_H_
#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"
#include "sleep.h"
#include "stdlib.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "ADDA.h"

#define Amplifier_Switch_BASEADDR XPAR_AC_8_CHANNEL_0_ONOFF_CONFIG_AXI_0_BASEADDR
#define Module_Status_ADDR		0

#define Amplifier_Din0_ADDR 	4
#define Amplifier_Din1_ADDR 	8
#define Amplifier_Din2_ADDR 	12
#define Amplifier_Din3_ADDR		16

#define Switch_Write_ADDR		20
#define Switch_Read_ADDR		4

#define RdSerial_ADDR			8

#define INTC_DEVICE_ID			XPAR_SCUGIC_SINGLE_DEVICE_ID   //中断控制器器件ID，0
#define	Switch_INT_ID			68U 		 	//PL端的中断ID
#define Amplifier_INT_ID		69U		 		//PL端的中断ID

enum Write_Read{
	bit_8 = 0,
	bit_16,
	bit_24,
	bit_32
};


void Switch_INT_handler();
void Amplifier_INT_handler();
u32 invert_Binary (u32 num);
void Write_Read_Switch(u32 Data_width,u32 Data);
void power_amplifier_control(float Wave_Amplitude[],u32 Wave_Range[]);
void RdSerial();
unsigned char voltage_to_output(float voltage);
unsigned char current_to_output(float current);
#endif /* SRC_AMPLIFIER_SWITCH_H_ */
