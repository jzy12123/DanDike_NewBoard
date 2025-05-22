/*
 * Amplifier_Switch.c
 *
 *  Created on: 2024年2月1日
 *      Author: saber
 */
#include "Amplifier_Switch.h"

void RdSerial()
{
	static u8 lastProectFault = 0xFF;  // 保存上一次的故障状态，初始为无故障
	static bool faultDetected = false; // 是否已经检测到一次故障

	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000200); // slv_reg0 的 rdserial—enable 置 1

	// ProectFault前4位代表IXICIBIA 后四位代表UXUCUBUA,如果正常则为11111111，出现故障对应的位会变0.
	u8 ProectFault = (u8)Xil_In32(Amplifier_Switch_BASEADDR + RdSerial_ADDR); // 读 slv_reg2 的 rdserial——dataout

	// 如果当前检测到故障信号
	if (ProectFault != 0xFF)
	{
		// 如果之前已经检测到过一次故障，并且当前故障与上次故障相同
		if (faultDetected && (ProectFault == lastProectFault))
		{
			// 连续两次检测到相同故障，上报
			report_protection_event(ProectFault);
			// 上报后重置标志,避免重复上报同一故障
			faultDetected = false;
			// 要清除二级功放电压电流的EN,清除故障锁存
		}
		else if (!faultDetected)
		{
			// 第一次检测到故障，记录并设置标志
			faultDetected = true;
			lastProectFault = ProectFault;
		}
		// 如果故障不同，更新为新的故障
		else if (ProectFault != lastProectFault)
		{
			lastProectFault = ProectFault;
		}
	}
	else
	{
		// 当前无故障，重置标志
		faultDetected = false;
	}
}

// 根据输入电压返回对应的输出值
unsigned char voltage_to_output(float voltage)
{
	if (voltage >= 6) // 6.5V
	{
		return 0xC2;
	}
	else if (voltage >= 3) // 3.25V
	{
		return 0xD4;
	}
	else if (voltage >= 1) // 1.625V
	{
		return 0xA0;
	}
	else
	{
		xil_printf("CPU1: UR NOT CORRECT SET 6.5V\n");
		return 0xC2; // 默认返回值，表示输入无效
	}
}

// 根据输入电流返回对应的输出值
unsigned char current_to_output(float current)
{
	if (current >= 4) // 5A
	{
		return 0xC2;
	}
	else if (current >= 0.5) // 1A
	{
		return 0x92;
	}
	else if (current >= 0.1) // 0.2A
	{
		return 0xA4;
	}
	else
	{
		xil_printf("CPU1: IR NOT CORRECT SET 5A\n");
		return 0xC2; // 默认返回值，表示输入无效
	}
}

void power_amplifier_control(float Wave_Amplitude[], u32 Wave_Range[], uint8_t pid_state, uint8_t enable_amp)
{
	if (enable_amp == POWAMP_ON)
	{
		/*1 配置595 开启使能最高位写1*/
		// 595置1 1595置0;功放start清0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
		usleep(100);
		//  xil_printf("CPU1:595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回0，则配置完成

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0发送 00000000为高电平
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1发送 ff00为低电平
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2发送
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3发送

		// 595置1 1595置0;  功放start置1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
		usleep(100);
		//  xil_printf("CPU1:595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回1，则配置完成

		/*2 PID调整幅值(修改1595)*/
		double Amplifier_PID_Increment[8] = {0};
		if (pid_state == PID_ON)
		{
			for (int i = 0; i < 4; i++)
			{
				Amplifier_PID_Increment[i] = PID_adjust_amplitude((lineAC.ur[i] * Wave_Amplitude[i]) / 100, lineAC.u[i], &amplitude_pid[i]);
				Amplifier_PID_Increment[i + 4] = PID_adjust_amplitude((lineAC.ir[i] * Wave_Amplitude[i + 4]) / 100, lineAC.i[i], &amplitude_pid[i + 4]);
			}
		}
		else
		{
			for (int i = 0; i < 8; i++)
			{ // 清空PID累计值
				amplitude_pid[i].integral = 0;
				amplitude_pid[i].prev_error = 0;
			}
		}

		/*3 配置1595*/
		// 595置0 1595置1;  功放start清0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000001);
		usleep(100);
		//  xil_printf("CPU1:1595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回0，则配置完成

		// 获取每个通道的量程索引
		int idx_ua = get_voltage_index_by_value(setACS.Vals[0].UR);
		int idx_ub = get_voltage_index_by_value(setACS.Vals[1].UR);
		int idx_uc = get_voltage_index_by_value(setACS.Vals[2].UR);
		int idx_ux = get_voltage_index_by_value(setACS.Vals[3].UR);

		int idx_ia = get_current_index_by_value(setACS.Vals[0].IR);
		int idx_ib = get_current_index_by_value(setACS.Vals[1].IR);
		int idx_ic = get_current_index_by_value(setACS.Vals[2].IR);
		int idx_ix = get_current_index_by_value(setACS.Vals[3].IR);

		// 计算电压通道值
		double correction_ua = calculate_correction(0, idx_ua, Wave_Amplitude[0]); // DAC原始输出
		double PID_correction_ua = (32767 / setACS.Vals[0].UR) * Amplifier_PID_Increment[0]; //PID原始输出
		u32 UA = (u32)((Wave_Amplitude[0] / 100) * (correction_ua + PID_correction_ua));
		double correction_ub = calculate_correction(1, idx_ub, Wave_Amplitude[1]);
		double PID_correction_ub = (32767 / setACS.Vals[1].UR) * Amplifier_PID_Increment[1]; // PID原始输出
		u32 UB = ((u32)((Wave_Amplitude[1] / 100) * (correction_ub + PID_correction_ub))) << 16;
		double correction_uc = calculate_correction(2, idx_uc, Wave_Amplitude[2]);
		double PID_correction_uc = (32767 / setACS.Vals[2].UR) * Amplifier_PID_Increment[2]; // PID原始输出
		u32 UC = (u32)((Wave_Amplitude[2] / 100) * (correction_uc + PID_correction_uc));
		double correction_ux = calculate_correction(3, idx_ux, Wave_Amplitude[3]);
		double PID_correction_ux = (32767 / setACS.Vals[3].UR) * Amplifier_PID_Increment[3]; // PID原始输出
		u32 UX = ((u32)((Wave_Amplitude[3] / 100) * (correction_ux + PID_correction_ux))) << 16;

		// 计算电流通道值
		double correction_ia = calculate_correction(4, idx_ia, Wave_Amplitude[4]);
		double PID_correction_ia = (32767 / setACS.Vals[0].IR) * Amplifier_PID_Increment[4]; // PID原始输出
		u32 IA = (u32)((Wave_Amplitude[4] / 100) * (correction_ia + PID_correction_ia));
		double correction_ib = calculate_correction(5, idx_ib, Wave_Amplitude[5]);
		double PID_correction_ib = (32767 / setACS.Vals[1].IR) * Amplifier_PID_Increment[5]; // PID原始输出
		u32 IB = ((u32)((Wave_Amplitude[5] / 100) * (correction_ib + PID_correction_ib))) << 16;
		double correction_ic = calculate_correction(6, idx_ic, Wave_Amplitude[6]);
		double PID_correction_ic = (32767 / setACS.Vals[2].IR) * Amplifier_PID_Increment[6]; // PID原始输出
		u32 IC = (u32)((Wave_Amplitude[6] / 100) * (correction_ic + PID_correction_ic));
		double correction_ix = calculate_correction(7, idx_ix, Wave_Amplitude[7]);
		double PID_correction_ix = (32767 / setACS.Vals[3].IR) * Amplifier_PID_Increment[7]; // PID原始输出
		u32 IX = ((u32)((Wave_Amplitude[7] / 100) * (correction_ix + PID_correction_ix))) << 16;

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, UB + UA); // din0发送，8000半幅值，ub + ua
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, UX + UC); // din1发送，半幅值，ux + uc
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, IB + IA); // din2发送，ib + ia
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, IX + IC); // din3发送，ix + ic

		// 595置0 1595置1;  功放start置1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000101);
		usleep(100);
		//  xil_printf("CPU1:1595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回1，则配置完成
	}
	else
	{
		/*1 配置595 关闭功放使能 只需要把Wave_Range的最高位写0*/
		// 595置1 1595置0 功放start清0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
		usleep(100);
		//  xil_printf("CPU1:595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回0，则配置完成

		// 修改Wave_Range,把第7位清0，为了清空二级功放的硬件保护：
		for (int i = 0; i < CHANNL_MAX; i++)
		{
			Wave_Range[i] &= ~(1 << 7);
		}

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0发送
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1发送
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2发送
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3发送

		// 595置1 1595置0 功放start置1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
		usleep(100);
		//  xil_printf("CPU1:595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回1，则配置完成
		print("CPU1: POWAMP Closed!\n");

		/*2 清空PID累计值*/
		for (int i = 0; i < 4; i++)
		{
			amplitude_pid[i].integral = 0;
			amplitude_pid[i].prev_error = 0;
		}
	}
}

void Write_Read_Switch(u32 Data_width, u32 Data)
{
	u32 Read_Switch;

	// 开关量为8位 开关量start清0  关闭595 1595使能  功放start清0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x00000000 | (Data_width + 1) << 16));
	usleep(10000);
	// 开关量为8位 开关量start置1  关闭595 1595使能  功放start清0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x01000000 | (Data_width + 1) << 16));
	usleep(10000);
	///////////////////////////////////////////////////////////////////
	// 模块进行一次读写
	///////////////////////////////////////////////////////////////////
	// 开关量为8位 开关量start清0  关闭595 1595使能  功放start清0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x00000000 | (Data_width + 1) << 16));
	usleep(10000);
	// 开关量为8位 开关量start置1  关闭595 1595使能  功放start清0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x01000000 | (Data_width + 1) << 16));
	usleep(10000);

	// 开关量模块输出
	Xil_Out32(Amplifier_Switch_BASEADDR + Switch_Write_ADDR, (u32)Data);
	// 开关量模块输入
	switch (Data_width)
	{
	case 0:
		Read_Switch = (u8)invert_Binary(Xil_In32(Amplifier_Switch_BASEADDR + Switch_Read_ADDR));
		break;
	case 1:
		Read_Switch = (u16)invert_Binary(Xil_In32(Amplifier_Switch_BASEADDR + Switch_Read_ADDR));
		break;
	case 2:
		Read_Switch = (u32)invert_Binary(Xil_In32(Amplifier_Switch_BASEADDR + Switch_Read_ADDR));
		break;
	case 3:
		Read_Switch = (u32)invert_Binary(Xil_In32(Amplifier_Switch_BASEADDR + Switch_Read_ADDR));
		break;
	default:
		break;
	}
	char s[32];
	itoa(Read_Switch, s, 2);
	xil_printf("Read switch_mudule Data : %s\n", s);
}
u32 invert_Binary(u32 num)
{
	u32 m1 = 0x55555555; // 01010101...
	u32 m2 = 0x33333333; // 00110011...
	u32 m4 = 0x0f0f0f0f; // 00001111...
						 //	u32 m8 = 0x00ff00ff; // 0000000011111111

	num = ((num >> 1) & m1) | ((num & m1) << 1);
	num = ((num >> 2) & m2) | ((num & m2) << 2);
	num = ((num >> 4) & m4) | ((num & m4) << 4);
	//	num = ((num >> 8) & m8 )| ((num & m8) << 8);
	//	num = num >> 16 | num << 16;

	return num;
}

/**
 * @brief 线性拟合两个校准点计算校准参数
 *
 * 根据当前幅值百分比，使用20%幅值和100%幅值时的校准参数进行线性拟合
 *
 * @param channel 通道索引 (0-7)
 * @param range_idx 量程索引 (0-2)
 * @param amplitude_percentage 幅值百分比 (0-100)
 * @return 经过线性拟合后的校准参数
 */
double calculate_correction(int channel, int range_idx, float amplitude_percentage)
{
	if (amplitude_percentage <= 20.0)
	{
		// 如果幅值小于等于20%，直接使用20%时的校准参数
		return DA_Correct_20[channel][range_idx];
	}
	else if (amplitude_percentage >= 100.0)
	{
		// 如果幅值大于等于100%，直接使用100%时的校准参数
		return DA_Correct_100[channel][range_idx];
	}
	else
	{
		// 在20%到100%之间进行线性拟合
		double ratio = (amplitude_percentage - 20.0) / 80.0; // 归一化比例
		double correction_20 = DA_Correct_20[channel][range_idx];
		double correction_100 = DA_Correct_100[channel][range_idx];
		return correction_20 + ratio * (correction_100 - correction_20);
	}
}
