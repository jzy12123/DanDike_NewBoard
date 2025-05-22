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

	Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000200); // slv_reg0 �� rdserial��enable �� 1

	// ProectFaultǰ4λ����IXICIBIA ����λ����UXUCUBUA,���������Ϊ11111111�����ֹ��϶�Ӧ��λ���0.
	u8 ProectFault = (u8)Xil_In32(Amplifier_Switch_BASEADDR + RdSerial_ADDR); // �� slv_reg2 �� rdserial����dataout

	// �����ǰ��⵽�����ź�
	if (ProectFault != 0xFF)
	{
		// ���֮ǰ�Ѿ���⵽��һ�ι��ϣ����ҵ�ǰ�������ϴι�����ͬ
		if (faultDetected && (ProectFault == lastProectFault))
		{
			// �������μ�⵽��ͬ���ϣ��ϱ�
			report_protection_event(ProectFault);
			// �ϱ������ñ�־,�����ظ��ϱ�ͬһ����
			faultDetected = false;
			// Ҫ����������ŵ�ѹ������EN,�����������
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
		xil_printf("CPU1: UR NOT CORRECT SET 6.5V\n");
		return 0xC2; // Ĭ�Ϸ���ֵ����ʾ������Ч
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
		xil_printf("CPU1: IR NOT CORRECT SET 5A\n");
		return 0xC2; // Ĭ�Ϸ���ֵ����ʾ������Ч
	}
}

void power_amplifier_control(float Wave_Amplitude[], u32 Wave_Range[], uint8_t pid_state, uint8_t enable_amp)
{
	if (enable_amp == POWAMP_ON)
	{
		/*1 ����595 ����ʹ�����λд1*/
		// 595��1 1595��0;����start��0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
		usleep(100);
		//  xil_printf("CPU1:595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����0�����������

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0���� 00000000Ϊ�ߵ�ƽ
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1���� ff00Ϊ�͵�ƽ
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2����
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3����

		// 595��1 1595��0;  ����start��1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
		usleep(100);
		//  xil_printf("CPU1:595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����1�����������

		/*2 PID������ֵ(�޸�1595)*/
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
			{ // ���PID�ۼ�ֵ
				amplitude_pid[i].integral = 0;
				amplitude_pid[i].prev_error = 0;
			}
		}

		/*3 ����1595*/
		// 595��0 1595��1;  ����start��0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000001);
		usleep(100);
		//  xil_printf("CPU1:1595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����0�����������

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
		double correction_ua = calculate_correction(0, idx_ua, Wave_Amplitude[0]); // DACԭʼ���
		double PID_correction_ua = (32767 / setACS.Vals[0].UR) * Amplifier_PID_Increment[0]; //PIDԭʼ���
		u32 UA = (u32)((Wave_Amplitude[0] / 100) * (correction_ua + PID_correction_ua));
		double correction_ub = calculate_correction(1, idx_ub, Wave_Amplitude[1]);
		double PID_correction_ub = (32767 / setACS.Vals[1].UR) * Amplifier_PID_Increment[1]; // PIDԭʼ���
		u32 UB = ((u32)((Wave_Amplitude[1] / 100) * (correction_ub + PID_correction_ub))) << 16;
		double correction_uc = calculate_correction(2, idx_uc, Wave_Amplitude[2]);
		double PID_correction_uc = (32767 / setACS.Vals[2].UR) * Amplifier_PID_Increment[2]; // PIDԭʼ���
		u32 UC = (u32)((Wave_Amplitude[2] / 100) * (correction_uc + PID_correction_uc));
		double correction_ux = calculate_correction(3, idx_ux, Wave_Amplitude[3]);
		double PID_correction_ux = (32767 / setACS.Vals[3].UR) * Amplifier_PID_Increment[3]; // PIDԭʼ���
		u32 UX = ((u32)((Wave_Amplitude[3] / 100) * (correction_ux + PID_correction_ux))) << 16;

		// �������ͨ��ֵ
		double correction_ia = calculate_correction(4, idx_ia, Wave_Amplitude[4]);
		double PID_correction_ia = (32767 / setACS.Vals[0].IR) * Amplifier_PID_Increment[4]; // PIDԭʼ���
		u32 IA = (u32)((Wave_Amplitude[4] / 100) * (correction_ia + PID_correction_ia));
		double correction_ib = calculate_correction(5, idx_ib, Wave_Amplitude[5]);
		double PID_correction_ib = (32767 / setACS.Vals[1].IR) * Amplifier_PID_Increment[5]; // PIDԭʼ���
		u32 IB = ((u32)((Wave_Amplitude[5] / 100) * (correction_ib + PID_correction_ib))) << 16;
		double correction_ic = calculate_correction(6, idx_ic, Wave_Amplitude[6]);
		double PID_correction_ic = (32767 / setACS.Vals[2].IR) * Amplifier_PID_Increment[6]; // PIDԭʼ���
		u32 IC = (u32)((Wave_Amplitude[6] / 100) * (correction_ic + PID_correction_ic));
		double correction_ix = calculate_correction(7, idx_ix, Wave_Amplitude[7]);
		double PID_correction_ix = (32767 / setACS.Vals[3].IR) * Amplifier_PID_Increment[7]; // PIDԭʼ���
		u32 IX = ((u32)((Wave_Amplitude[7] / 100) * (correction_ix + PID_correction_ix))) << 16;

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, UB + UA); // din0���ͣ�8000���ֵ��ub + ua
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, UX + UC); // din1���ͣ����ֵ��ux + uc
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, IB + IA); // din2���ͣ�ib + ia
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, IX + IC); // din3���ͣ�ix + ic

		// 595��0 1595��1;  ����start��1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000101);
		usleep(100);
		//  xil_printf("CPU1:1595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����1�����������
	}
	else
	{
		/*1 ����595 �رչ���ʹ�� ֻ��Ҫ��Wave_Range�����λд0*/
		// 595��1 1595��0 ����start��0
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000002);
		usleep(100);
		//  xil_printf("CPU1:595 config clear = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����0�����������

		// �޸�Wave_Range,�ѵ�7λ��0��Ϊ����ն������ŵ�Ӳ��������
		for (int i = 0; i < CHANNL_MAX; i++)
		{
			Wave_Range[i] &= ~(1 << 7);
		}

		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0����
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1����
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2����
		Xil_Out32(Amplifier_Switch_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3����

		// 595��1 1595��0 ����start��1
		Xil_Out32(Amplifier_Switch_BASEADDR + Module_Status_ADDR, (u32)0x00000102);
		usleep(100);
		//  xil_printf("CPU1:595 config done = %d\r\n",  (Xil_In32(Amplifier_Switch_BASEADDR + Module_Status_ADDR) & 0x8000) >> 15);  //����1�����������
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

/**
 * @brief �����������У׼�����У׼����
 *
 * ���ݵ�ǰ��ֵ�ٷֱȣ�ʹ��20%��ֵ��100%��ֵʱ��У׼���������������
 *
 * @param channel ͨ������ (0-7)
 * @param range_idx �������� (0-2)
 * @param amplitude_percentage ��ֵ�ٷֱ� (0-100)
 * @return ����������Ϻ��У׼����
 */
double calculate_correction(int channel, int range_idx, float amplitude_percentage)
{
	if (amplitude_percentage <= 20.0)
	{
		// �����ֵС�ڵ���20%��ֱ��ʹ��20%ʱ��У׼����
		return DA_Correct_20[channel][range_idx];
	}
	else if (amplitude_percentage >= 100.0)
	{
		// �����ֵ���ڵ���100%��ֱ��ʹ��100%ʱ��У׼����
		return DA_Correct_100[channel][range_idx];
	}
	else
	{
		// ��20%��100%֮������������
		double ratio = (amplitude_percentage - 20.0) / 80.0; // ��һ������
		double correction_20 = DA_Correct_20[channel][range_idx];
		double correction_100 = DA_Correct_100[channel][range_idx];
		return correction_20 + ratio * (correction_100 - correction_20);
	}
}
