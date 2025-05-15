/******************************************************************************
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
 ******************************************************************************/

#include <stdio.h>

/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
/*user includes*/
#include "xil_mmu.h"
#include "Amplifier_Switch.h"
#include "ADDA.h"
#include "Communications_Protocol.h"
#include "PID.h"
#include "Msg_que.h"
#include "My_kissFft.h"
#include "xgpiops.h" //���� PS GPIO �ĺ���
#include "Rc64.h"

#define GPIO_DEVICE_ID XPAR_XGPIOPS_0_DEVICE_ID
#define MIO_USB 8 // ���ӵ� MIO8
// ��FFT���㿪ʼǰ
#define CPU1_PRIORITY_REG 0xF8F00104
void reinitialize_dma_controller();
int main()
{
	sleep(30); // ����Ҫ�еȴ�linux����
	// xil_printf("CPU1: Start\r\n");
	// xil_printf("CPU1: initializing\r\n");
	/************************** USB��ʼ��  ����Ҫ���Ѿ��Ͽ���Mio����*****************************/
	// XGpioPs Gpio; // GPIO �豸����������ʵ��
	// XGpioPs_Config *ConfigPtr;
	// ConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);
	// XGpioPs_CfgInitialize(&Gpio, ConfigPtr, ConfigPtr->BaseAddr);
	// XGpioPs_SetDirectionPin(&Gpio, MIO_USB, 1);
	// XGpioPs_SetOutputEnablePin(&Gpio, MIO_USB, 1);
	// XGpioPs_WritePin(&Gpio, MIO_USB, 0x1);

	
	// // ��ʼ��RC64ģ��
	// RC64_Init();
	// // ��EEPROM��ȡУ׼����
	// RC64_ReadCalibData();
	/************************** DMA��ʼ�� *****************************/
	int status;
	XAxiDma_Config *config;
	config = XAxiDma_LookupConfig(DMA_DEV_ID);
	if (!config)
	{
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);
	}
	// ��ʼ��DMA����
	status = XAxiDma_CfgInitialize(&axidma, config);
	if (status != XST_SUCCESS)
	{
		xil_printf("Initialization failed %d\r\n", status);
	}
	// ��ʼ��dma��־�ź�
	tx_done = 0;
	error = 0;

	/************************** ��ʱ����ʼ�� *****************************/
	status = timer_init(&Timer); // ��ʱ����ʼ��
	if (status != XST_SUCCESS)
	{
		xil_printf("Timer Initial Failed\r\n");
	}
	Timer_Flag = 0;

	// �����ж�ϵͳ
	status = setup_intr_system(&intc, &axidma, &Timer,
							   DMA_RX_INTR_ID, DMA_TX_INTR_ID, Underflow_INTR_ID, TIMER_IRPT_INTR);
	if (status != XST_SUCCESS)
	{
		xil_printf("Failed intr setup\r\n");
	}

	/************************** ����Cache*****************************/
	Xil_SetTlbAttributes(JSON_ADDR, 0x14de2); // ����Cache����	//S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0
	Xil_SetTlbAttributes(UDP_ADDRESS, 0x14de2);
	Xil_SetTlbAttributes(Share_addr, 0x14de2);
	// �޸��ڴ����ԣ�����Ϊ�豸�ڴ棨���ɻ��棩
	Xil_SetTlbAttributes(0x40400000, 0xC02); // DMA�������Ĵ�������
	Xil_SetTlbAttributes(0x43C30000, 0xC02); // ADC�������Ĵ�������

	XScuTimer_Start(&Timer); // ������ʱ��
	InitializeQueues();
	init_JsonUdp();
	PID_Init_All();

	xil_printf("CPU1: Initialization successfully\r\n");
	const char *arm_version_for_print = get_version_string(ARM_Ver_Full);
	xil_printf("CPU1: ARM Version: %s\r\n", arm_version_for_print);

	while (1)
	{
		if (Timer_Flag)
		{

			/************************** ����FFT*****************************/
			// ����AD����
			if (devState.bACRunning == true)
			{
				Adc_Start(sample_points, sample_points * Wave_Frequency, AD_SAMP_CYCLE_NUMBER);

				if (AdcFinish_Flag != 1)
				{
					// printf("CPU1 Warning : ADC data is not ready || ");
					// printf("ADC Error Count = %d\n", error);
				}
				else if (AdcFinish_Flag == 1)
				{
					// ˢ�¹����ڴ�Ļ��棬��֤���ݵ�һ����
					Xil_DCacheFlushRange((UINTPTR)Share_addr, sample_points * 16 * CHANNL_MAX * AD_SAMP_CYCLE_NUMBER);

					// ���ü���ֵ
					double Phase_reference = 0; // ������λ��׼
					lineAC.totalP = 0.0;
					lineAC.totalQ = 0.0;
					lineAC.totalPF = 0.0;
					for (int i = 0; i < 4; i++)
					{
						// ����FFT
						double harmonic_info_U[HarmNumberMax][3] = {0}; // �������ڴ洢г��������
						double harmonic_info_I[HarmNumberMax][3] = {0};

						// AC����Դ����
						AnalyzeWaveform_AcSource(harmonic_info_U, i, Share_addr, sample_points * Wave_Frequency, Wave_Frequency);
						AnalyzeWaveform_AcSource(harmonic_info_I, i + 4, Share_addr, sample_points * Wave_Frequency, Wave_Frequency);

						if (i == 0)
						{
							// ������λ��׼
							Phase_reference = harmonic_info_U[0][2];
						}
						// lineAC - ��������Ľ����䵽UDP�ṹ����
						// ��ȡ��ѹ�͵�����������
						int idx_u = get_voltage_index_by_value(setACS.Vals[i].UR);
						int idx_i = get_current_index_by_value(setACS.Vals[i].IR);

						lineAC.f[i] = harmonic_info_U[0][0]; // Ƶ��
						lineAC.ur[i] = setACS.Vals[0].UR;	 // ��ѹ��λ

						if (AD_Correct[i][idx_u] == 0)
						{
							lineAC.u[i] = 0;
							// ��ֹ����Ϊ0
							printf("CPU1:Warning: Division by zero in lineAC.u[%d] calculation.\n", i);
						}
						else
						{
							// ��ѹ��ֵ V
							lineAC.u[i] = harmonic_info_U[0][1] / AD_Correct[i][idx_u] * setACS.Vals[i].UR;
						}

						lineAC.ir[i] = setACS.Vals[0].IR;													  // ������λ
						lineAC.i[i] = (harmonic_info_I[0][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR; // ���� A
						lineAC.phu[i] = harmonic_info_U[0][2] - Phase_reference;							  // ��ѹ��λ �Ƕ��ƣ�UAΪ�ο���
						if (lineAC.phu[i] < 0)
						{
							lineAC.phu[i] += 360;
						}
						lineAC.phi[i] = harmonic_info_I[0][2] - Phase_reference; // ������λ��UAΪ�ο���
						if (lineAC.phi[i] < 0)
						{
							lineAC.phi[i] += 360;
						}

						// �����ѹ�����֮�����λ��
						double phase_diff = lineAC.phu[i] - lineAC.phi[i];
						// ȷ����λ����-180��180��֮��
						if (phase_diff > 180.0)
						{
							phase_diff -= 360.0;
						}

						else if (phase_diff < -180.0)
						{
							phase_diff += 360.0;
						}

						lineAC.p[i] = (lineAC.u[i] * lineAC.i[i] * cos(phase_diff * M_PI / 180.0f)); // �й�����
						lineAC.q[i] = (lineAC.u[i] * lineAC.i[i] * sin(phase_diff * M_PI / 180.0f)); // �޹�����
						lineAC.pf[i] = cos(phase_diff * M_PI / 180.0f);								 // ��������

						// �ۼӵ��ܹ���
						lineAC.totalP += lineAC.p[i];
						lineAC.totalQ += lineAC.q[i];

						// ��ʼ����г�������ʱ���
						double thdu = 0.0;
						double thdi = 0.0;
						// �����ѹ��г�������� (THDU)
						if (harmonic_info_U[0][1] >= 0.0001)
						{ // ���������
							double sum_of_squares_u = 0.0;
							// ������2��г����32��г��
							for (int h = 1; h < 32; h++)
							{
								// �����i��г���ı�ֵ
								double harmonic_ratio_u = harmonic_info_U[h][1] / harmonic_info_U[0][1];
								// �ۼ�ƽ��
								sum_of_squares_u += harmonic_ratio_u * harmonic_ratio_u;
							}
							// ����ƽ���͵�ƽ�������õ�THD
							thdu = sqrt(sum_of_squares_u);
						}
						else
						{
							// ������ֵΪ�㣬�޷�����THD��������Ҫ���������������
							thdu = 0.0;
						}
						// ���������г�������� (THDI)
						if (harmonic_info_I[0][1] >= 0.0001)
						{ // ���������
							double sum_of_squares_i = 0.0;
							// ������2��г����32��г��
							for (int h = 1; h < 32; h++)
							{
								// �����i��г���ı�ֵ
								double harmonic_ratio_i = harmonic_info_I[h][1] / harmonic_info_I[0][1];
								// �ۼ�ƽ��
								sum_of_squares_i += harmonic_ratio_i * harmonic_ratio_i;
							}
							// ����ƽ���͵�ƽ�������õ�THD
							thdi = sqrt(sum_of_squares_i);
						}
						else
						{
							// ������ֵΪ�㣬�޷�����THD��������Ҫ���������������
							thdi = 0.0;
						}
						// ������
						lineAC.thdu[i] = thdu * 100.0;
						lineAC.thdi[i] = thdi * 100.0;

						/*lineHarm*/
						// ��ʼ����ͨ�����ܹ����ۼӱ���
						lineHarm.harm[i].totalP = 0.0;
						lineHarm.harm[i].totalQ = 0.0;

						// ��ȡ��ѹ�͵�����������
						idx_u = get_voltage_index_by_value(setACS.Vals[i].UR);
						idx_i = get_current_index_by_value(setACS.Vals[i].IR);

						// �洢������ֵ����λ�����ڼ���ٷֱȺ������λ
						double baseU = harmonic_info_U[0][1];
						double baseI = harmonic_info_I[0][1];

						for (int j = 1; j < HarmNumberMax; j++)
						{
							// ��ѹ�͵�����ֵ����
							if (j == 1)
							{
								// ����(j=0)���⴦��
								lineHarm.harm[i].u[j] = (harmonic_info_U[j - 1][1] / AD_Correct[i][idx_u]) * setACS.Vals[i].UR;
								lineHarm.harm[i].i[j] = (harmonic_info_I[j - 1][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR;

								// ������λֱ�Ӳ�������ڲο���λ��ֵ
								lineHarm.harm[i].phu[j] = harmonic_info_U[j - 1][2] - Phase_reference;
								lineHarm.harm[i].phi[j] = harmonic_info_I[j - 1][2] - Phase_reference;
							}
							else
							{
								// г��(j>0)����Ϊ�����İٷֱ�
								if (baseU > 0.0001)
								{ // ������Խӽ����ֵ
									lineHarm.harm[i].u[j] = (harmonic_info_U[j - 1][1] / baseU) * 100.0;
								}
								else
								{
									lineHarm.harm[i].u[j] = 0.0;
								}

								if (baseI > 0.0001)
								{ // ������Խӽ����ֵ
									lineHarm.harm[i].i[j] = (harmonic_info_I[j - 1][1] / baseI) * 100.0;
								}
								else
								{
									lineHarm.harm[i].i[j] = 0.0;
								}

								// г����λ����
								double n = j; // г������
								// ���������λ
								double u_relative_phase = harmonic_info_U[j - 1][2] - n * Phase_reference;
								double i_relative_phase = harmonic_info_I[j - 1][2] - n * Phase_reference;
								// ȷ����λ��0��360��֮��
								lineHarm.harm[i].phu[j] = fmod(u_relative_phase + 360.0, 360.0);
								lineHarm.harm[i].phi[j] = fmod(i_relative_phase + 360.0, 360.0);

								switch ((j - 1) % 4)
								{
								case 0: // 1, 5, 7, 11, 13, 17, 19, 23, 25, 29, 31��
									lineHarm.harm[i].phu[j] -= 0.0;
									lineHarm.harm[i].phi[j] -= 0.0;
									break;
								case 1: // 2, 6, 8, 12, 14, 18, 20, 24, 26, 30��
									lineHarm.harm[i].phu[j] -= 270.0;
									lineHarm.harm[i].phi[j] -= 270.0;
									break;
								case 2: // 3, 7, 9, 13, 15, 19, 21, 25, 27, 31��
									lineHarm.harm[i].phu[j] -= 180.0;
									lineHarm.harm[i].phi[j] -= 180.0;
									break;
								case 3: // 4, 8, 10, 14, 16, 20, 22, 26, 28, 32��
									lineHarm.harm[i].phu[j] -= 90.0;
									lineHarm.harm[i].phi[j] -= 90.0;
									break;
								}
								// ȷ����0-360��
								if (lineHarm.harm[i].phu[j] < 0)
								{
									lineHarm.harm[i].phu[j] += 360;
								}
								if (lineHarm.harm[i].phi[j] < 0)
								{
									lineHarm.harm[i].phi[j] += 360;
								}
							}

							// ����г������λ��Ƕȣ�
							double phase_diff = lineHarm.harm[i].phu[j] - lineHarm.harm[i].phi[j];

							// ����г�����й����޹�����
							lineHarm.harm[i].p[j] = lineHarm.harm[i].u[j] * lineHarm.harm[i].i[j] * cos(phase_diff * M_PI / 180.0);
							lineHarm.harm[i].q[j] = lineHarm.harm[i].u[j] * lineHarm.harm[i].i[j] * sin(phase_diff * M_PI / 180.0);

							// �ۼӵ��ܹ���
							lineHarm.harm[i].totalP += lineHarm.harm[i].p[j];
							lineHarm.harm[i].totalQ += lineHarm.harm[i].q[j];
						}
					}
					// �ܹ�������
					double totalApparentPower = sqrt(lineAC.totalP * lineAC.totalP + lineAC.totalQ * lineAC.totalQ);
					if (totalApparentPower > 0)
					{
						lineAC.totalPF = lineAC.totalP / totalApparentPower;
					}
					else
					{
						lineAC.totalPF = 0.0; // ���������������ù�������Ϊ0
					}
				}

				/*2 PID�ջ��������*/
				// ���ɽ����ź�
				str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);
				//  ���ƹ���
				power_amplifier_control(Wave_Amplitude, Wave_Range, devState.bClosedLoop == 1 ? PID_ON : PID_OFF, POWAMP_ON);
			}
			else
			{
				// ���UDP�ṹ��
				initLineAC(&lineAC);
				initLineHarm(&lineHarm);
				for (int i = 0; i < 4; i++)
				{
					lineAC.ur[i] = setACS.Vals[i].UR;
					lineAC.ir[i] = setACS.Vals[i].IR;
				}
			}

			/*3 �ر�UDP�ṹ��*/
			Timer_Flag = 0;
			ReportUDP_Structure(reportStatus);

			// ����
			// printf("True PhUA= %3.6f || PhUB= %3.6f || PhUC= %3.6f || PhUX= %3.6f\n", lineAC.phu[0], lineAC.phu[1], lineAC.phu[2], lineAC.phu[3]);

			// printf("True PhIA= %3.6f || PhIB= %3.6f || PhIC= %3.6f || PhIX= %3.6f\n", lineAC.phi[0], lineAC.phi[1], lineAC.phi[2], lineAC.phi[3]);

			// printf("True UA= %.6f || UB= %.6f || UC= %.6f || UX= %.6f || SET  UA= %.4f\n", lineAC.u[0], lineAC.u[1], lineAC.u[2], lineAC.u[3], lineAC.ur[0] * Wave_Amplitude[0] / 100);
			// printf("True IA= %.6f || IB= %.6f || IC= %.6f || IX= %.6f || SET  IA= %.4f\n\n", lineAC.i[0], lineAC.i[1], lineAC.i[2], lineAC.i[3], lineAC.ir[0] * Wave_Amplitude[4] / 100);

			//			 printf("UA = %.6f\n", lineAC.u[0]);
			//			 printf("UB = %.6f\n", lineAC.u[1]);
			// printf("UC = %.6f\n", lineAC.u[2]);

			/*4 �������ź�*/
			RdSerial();
		}
	}
}
