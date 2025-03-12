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

	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000200);

	// ProectFault前4位代表IXICIBIA 后四位代表UXUCUBUA,如果正常则为11111111，出现故障对应的位会变0.
	u8 ProectFault = (u8)Xil_In32(Amplifier_Switch_BASEADDR + RdSerial_ADDR);

	// 如果当前检测到故障信号
	if (ProectFault != 0xFF)
	{
		// 如果之前已经检测到过一次故障，并且当前故障与上次故障相同
		if (faultDetected && (ProectFault == lastProectFault))
		{
			// 连续两次检测到相同故障，上报
			report_protection_event(ProectFault);
			// 上报后重置标志，避免重复上报同一故障
			faultDetected = false;
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
	if (voltage >= 6)//6.5V
	{
		return 0xC2;
	}
	else if (voltage >= 3)//3.25V
	{
		return 0xD4;
	}
	else if (voltage >= 1)//1.625V
	{
		return 0xA0;
	}
	else
	{
		xil_printf("CPU1: UR NOT CORRECT\n");
		return 0x00; // 默认返回值，表示输入无效
	}
}

// 根据输入电流返回对应的输出值
unsigned char current_to_output(float current)
{
	if (current >= 4)//5A
	{
		return 0xC2;
	}
	else if (current >= 0.5)//1A
	{
		return 0x92;
	}
	else if (current >= 0.1)//0.2A
	{
		return 0xA4;
	}
	else
	{
		xil_printf("CPU1: IR NOT CORRECT\n");
		return 0x00; // 默认返回值，表示输入无效
	}
}

void power_amplifier_control(float Wave_Amplitude[], u32 Wave_Range[], int pid_state)
{
	// PID调整幅值(修改1595)
	double Amplifier_PID_Increment[CHANNL_MAX] = {0};
	if (pid_state == PID_ON)
	{
		for (int i = 0; i < 4; i++)
		{
			Amplifier_PID_Increment[i] = PID_adjust_amplitude(lineAC.ur[i] * Wave_Range[i], lineAC.u[i], &amplitude_pid[i]);
			Amplifier_PID_Increment[i + 4] = PID_adjust_amplitude(lineAC.ir[i] * Wave_Range[i + 4], lineAC.i[i], &amplitude_pid[i + 4]);
		}
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			Amplifier_PID_Increment[i] = 0; // 清空PID累计值
			amplitude_pid[i].integral = 0;
			amplitude_pid[i].prev_error = 0;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 配置595 不开启使能
	//	// slv_reg0: 开关量 start清0; 595置1 1595置0,功放start清0;
	//	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
	//	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
	//	usleep(1000);
	//	//	xil_printf("CPU1:Amplifier config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回0，则配置完成
	//	// 595中的7引脚 ！EN信号对应rd_from_serial8bit读回数据 是0还是1，向595 中写00001000，读回 个是故障信号
	//	// 二进制数据		0 1 2 3 4  5  6  7   对应位数取反
	//	// 对应595引脚	4 5 6 7 14 15 16 17
	//
	//	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)((Wave_Range[1] & 0x7F) << 24) | ((Wave_Range[0] & 0x7F) << 8)); // ub + ua din0发送 00000000为高电平
	//	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)((Wave_Range[3] & 0x7F) << 24) | ((Wave_Range[2] & 0x7F) << 8)); // ux + uc din1发送 ff00为低电平 ux为低，uc为高	// rd_from_serial8bit中该通道读回为0
	//	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)((Wave_Range[5] & 0x7F) << 24) | ((Wave_Range[4] & 0x7F) << 8)); // ib + ia din2发送
	//	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)((Wave_Range[7] & 0x7F) << 24) | ((Wave_Range[6] & 0x7F) << 8)); // ix + ic din3发送
	//
	//	// slv_reg0: 开关量start清0; 595置1 1595置0;  功放start置1
	//	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
	//	usleep(1000);
	//	//	xil_printf("CPU1:Amplifier config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回1，则配置完成

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 配置595 开启使能最高位写1
	// 二级DA start清0; 595置1 1595置0;功放start清0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
	usleep(1000);
	//	xil_printf("CPU1:595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回0，则配置完成

	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0发送 00000000为高电平
	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1发送 ff00为低电平 ux为低，uc为高	// rd_from_serial8bit中该通道读回为0
	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2发送
	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3发送

	// 二级DA start清0; 595置1 1595置0;  功放start置1
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
	usleep(1000);
	//	xil_printf("CPU1:595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回1，则配置完成

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 配置1595
	// 二级DA 开关量start清0; 595置0 1595置1;  功放start清0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000001);
	usleep(1000);
	//	xil_printf("CPU1:1595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回0，则配置完成

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
	u32 UA = (u32)((Wave_Amplitude[0] / 100) * (DA_Correct[0][idx_ua] + Amplifier_PID_Increment[0]));
	u32 UB = ((u32)((Wave_Amplitude[1] / 100) * (DA_Correct[1][idx_ub] + Amplifier_PID_Increment[1]))) << 16;
	u32 UC = (u32)((Wave_Amplitude[2] / 100) * (DA_Correct[2][idx_uc] + Amplifier_PID_Increment[2]));
	u32 UX = ((u32)((Wave_Amplitude[3] / 100) * (DA_Correct[3][idx_ux] + Amplifier_PID_Increment[3]))) << 16;

	// 计算电流通道值
	u32 IA = (u32)((Wave_Amplitude[4] / 100) * (DA_Correct[4][idx_ia] + Amplifier_PID_Increment[4]));
	u32 IB = ((u32)((Wave_Amplitude[5] / 100) * (DA_Correct[5][idx_ib] + Amplifier_PID_Increment[5]))) << 16;
	u32 IC = (u32)((Wave_Amplitude[6] / 100) * (DA_Correct[6][idx_ic] + Amplifier_PID_Increment[6]));
	u32 IX = ((u32)((Wave_Amplitude[7] / 100) * (DA_Correct[7][idx_ix] + Amplifier_PID_Increment[7]))) << 16;

	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, UB + UA); // din0发送，8000半幅值，ub + ua
	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, UX + UC); // din1发送，半幅值，ux + uc
	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, IB + IA); // din2发送，ib + ia
	Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, IX + IC); // din3发送，ix + ic

	// 二级DA 开关量start清0; 595置0 1595置1;  功放start置1
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000101);
	usleep(1000);
	//	xil_printf("CPU1:1595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //返回1，则配置完成
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
void Switch_INT_handler()
{
	//	printf("Switch_Done");
}
void Amplifier_INT_handler()
{
	//	printf("Amplifier_Done");
}
