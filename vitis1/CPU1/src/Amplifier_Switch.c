/*
 * Amplifier_Switch.c
 *
 *  Created on: 2024��2��1��
 *      Author: saber
 */
#include "Amplifier_Switch.h"

void RdSerial()
{
	static u8 lastProectFault = 0xFF;  // ������һ�εĹ���״̬����ʼΪ�޹���
	static bool faultDetected = false; // �Ƿ��Ѿ���⵽һ�ι���

	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000200);

	// ProectFaultǰ4λ����IXICIBIA ����λ����UXUCUBUA,���������Ϊ11111111�����ֹ��϶�Ӧ��λ���0.
	u8 ProectFault = (u8)Xil_In32(Amplifier_Switch_BASEADDR + RdSerial_ADDR);

	// �����ǰ��⵽�����ź�
	if (ProectFault != 0xFF)
	{
		// ���֮ǰ�Ѿ���⵽��һ�ι��ϣ����ҵ�ǰ�������ϴι�����ͬ
		if (faultDetected && (ProectFault == lastProectFault))
		{
			// �������μ�⵽��ͬ���ϣ��ϱ�
			report_protection_event(ProectFault);
			// �ϱ������ñ�־�������ظ��ϱ�ͬһ����
			faultDetected = false;
		}
		else if (!faultDetected)
		{
			// ��һ�μ�⵽���ϣ���¼�����ñ�־
			faultDetected = true;
			lastProectFault = ProectFault;
		}
		// ������ϲ�ͬ������Ϊ�µĹ���
		else if (ProectFault != lastProectFault)
		{
			lastProectFault = ProectFault;
		}
	}
	else
	{
		// ��ǰ�޹��ϣ����ñ�־
		faultDetected = false;
	}
}

// ���������ѹ���ض�Ӧ�����ֵ
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
		xil_printf("CPU1: UR NOT CORRECT\n");
		return 0x00; // Ĭ�Ϸ���ֵ����ʾ������Ч
	}
}

// ��������������ض�Ӧ�����ֵ
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
		xil_printf("CPU1: IR NOT CORRECT\n");
		return 0x00; // Ĭ�Ϸ���ֵ����ʾ������Ч
	}
}

/**
 * @brief ���ʷŴ������ƺ���
 *
 * @param Wave_Amplitude ���η������飬����8��ͨ���ķ���
 * @param Wave_Range ���η�Χ���飬����8��ͨ���ķ�Χ
 * @param PIDmode PIDģʽ���ã�PID_ON��PID_OFF��
 * @param enable_amp ����ʹ�ܱ�־��1��ʾ������0��ʾ�ر�
 */
void power_amplifier_control(float Wave_Amplitude[], u32 Wave_Range[], uint8_t pid_state, uint8_t enable_amp)
{
	double Amplifier_PID_Increment[CHANNL_MAX] = {0};

	if (enable_amp == POWAMP_ON)
	{
		/*1 ����595 ����ʹ�����λд1*/
		// 595��1 1595��0;����start��0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
		usleep(100);
		//	xil_printf("CPU1:595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����0�����������

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0���� 00000000Ϊ�ߵ�ƽ
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1���� ff00Ϊ�͵�ƽ
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2����
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3����

		// 595��1 1595��0;  ����start��1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
		usleep(100);
		//	xil_printf("CPU1:595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����1�����������

		/*2 PID������ֵ(�޸�1595)*/

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
				Amplifier_PID_Increment[i] = 0; // ���PID�ۼ�ֵ
				amplitude_pid[i].integral = 0;
				amplitude_pid[i].prev_error = 0;
			}
		}

		/*3 ����1595*/
		// 595��0 1595��1;  ����start��0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000001);
		usleep(100);
		//	xil_printf("CPU1:1595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����0�����������

		// ��ȡÿ��ͨ������������
		int idx_ua = get_voltage_index_by_value(setACS.Vals[0].UR);
		int idx_ub = get_voltage_index_by_value(setACS.Vals[1].UR);
		int idx_uc = get_voltage_index_by_value(setACS.Vals[2].UR);
		int idx_ux = get_voltage_index_by_value(setACS.Vals[3].UR);

		int idx_ia = get_current_index_by_value(setACS.Vals[0].IR);
		int idx_ib = get_current_index_by_value(setACS.Vals[1].IR);
		int idx_ic = get_current_index_by_value(setACS.Vals[2].IR);
		int idx_ix = get_current_index_by_value(setACS.Vals[3].IR);

		// �����ѹͨ��ֵ
		u32 UA = (u32)((Wave_Amplitude[0] / 100) * (DA_Correct[0][idx_ua] + Amplifier_PID_Increment[0]));
		u32 UB = ((u32)((Wave_Amplitude[1] / 100) * (DA_Correct[1][idx_ub] + Amplifier_PID_Increment[1]))) << 16;
		u32 UC = (u32)((Wave_Amplitude[2] / 100) * (DA_Correct[2][idx_uc] + Amplifier_PID_Increment[2]));
		u32 UX = ((u32)((Wave_Amplitude[3] / 100) * (DA_Correct[3][idx_ux] + Amplifier_PID_Increment[3]))) << 16;

		// �������ͨ��ֵ
		u32 IA = (u32)((Wave_Amplitude[4] / 100) * (DA_Correct[4][idx_ia] + Amplifier_PID_Increment[4]));
		u32 IB = ((u32)((Wave_Amplitude[5] / 100) * (DA_Correct[5][idx_ib] + Amplifier_PID_Increment[5]))) << 16;
		u32 IC = (u32)((Wave_Amplitude[6] / 100) * (DA_Correct[6][idx_ic] + Amplifier_PID_Increment[6]));
		u32 IX = ((u32)((Wave_Amplitude[7] / 100) * (DA_Correct[7][idx_ix] + Amplifier_PID_Increment[7]))) << 16;

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, UB + UA); // din0���ͣ�8000���ֵ��ub + ua
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, UX + UC); // din1���ͣ����ֵ��ux + uc
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, IB + IA); // din2���ͣ�ib + ia
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, IX + IC); // din3���ͣ�ix + ic

		// 595��0 1595��1;  ����start��1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000101);
		usleep(100);
		//	xil_printf("CPU1:1595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����1�����������
	}
	else
	{
		/*1 ����595 �رչ���ʹ�� ֻ��Ҫ��Wave_Range�����λд0*/
		// 595��1 1595��0;����start��0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
		usleep(100);
		//	xil_printf("CPU1:595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����0�����������

		// �޸�Wave_Range,�ѵ�8λ��0��
		for (int i = 0; i < CHANNL_MAX; i++)
		{
			Wave_Range[i] &= ~(1 << 8);
		}
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0���� 00000000Ϊ�ߵ�ƽ
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1���� ff00Ϊ�͵�ƽ
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2����
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3����

		// 595��1 1595��0;  ����start��1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
		usleep(100);
		//	xil_printf("CPU1:595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����1�����������
		print("CPU1: POWAMP Closed!\n");

		/*2 ���PID�ۼ�ֵ*/
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

	// ������Ϊ8λ ������start��0  �ر�595 1595ʹ��  ����start��0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x00000000 | (Data_width + 1) << 16));
	usleep(10000);
	// ������Ϊ8λ ������start��1  �ر�595 1595ʹ��  ����start��0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x01000000 | (Data_width + 1) << 16));
	usleep(10000);
	///////////////////////////////////////////////////////////////////
	// ģ�����һ�ζ�д
	///////////////////////////////////////////////////////////////////
	// ������Ϊ8λ ������start��0  �ر�595 1595ʹ��  ����start��0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x00000000 | (Data_width + 1) << 16));
	usleep(10000);
	// ������Ϊ8λ ������start��1  �ر�595 1595ʹ��  ����start��0
	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)(0x01000000 | (Data_width + 1) << 16));
	usleep(10000);

	// ������ģ�����
	Xil_Out32(Amplifier_Switch_BASEADDR + Switch_Write_ADDR, (u32)Data);
	// ������ģ������
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
