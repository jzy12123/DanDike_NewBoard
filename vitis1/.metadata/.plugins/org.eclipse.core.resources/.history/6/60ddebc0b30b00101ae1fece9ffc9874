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
#include "xgpiops.h" //包含 PS GPIO 的函数

#define GPIO_DEVICE_ID XPAR_XGPIOPS_0_DEVICE_ID
#define MIO_USB 8 // 连接到 MIO8
// 在FFT计算开始前
#define CPU1_PRIORITY_REG 0xF8F00104
void reinitialize_dma_controller();
int main()
{
	xil_printf("CPU1: initializing\r\n");
	/************************** USB初始化 *****************************/
	XGpioPs Gpio; // GPIO 设备的驱动程序实例
	XGpioPs_Config *ConfigPtr;
	ConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);
	XGpioPs_CfgInitialize(&Gpio, ConfigPtr, ConfigPtr->BaseAddr);
	XGpioPs_SetDirectionPin(&Gpio, MIO_USB, 1);
	XGpioPs_SetOutputEnablePin(&Gpio, MIO_USB, 1);
	XGpioPs_WritePin(&Gpio, MIO_USB, 0x1);

	sleep(17); // 必须要有等待linux启动

	// 重置和重新初始化外设
	reinitialize_dma_controller();
	/************************** DMA初始化 *****************************/
	int status;
	XAxiDma_Config *config;
	config = XAxiDma_LookupConfig(DMA_DEV_ID);
	if (!config)
	{
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);
	}
	// 初始化DMA引擎
	status = XAxiDma_CfgInitialize(&axidma, config);
	if (status != XST_SUCCESS)
	{
		xil_printf("Initialization failed %d\r\n", status);
	}
	// 初始化dma标志信号
	tx_done = 0;
	error = 0;

	/************************** 定时器初始化 *****************************/
	status = timer_init(&Timer); // 定时器初始化
	if (status != XST_SUCCESS)
	{
		xil_printf("Timer Initial Failed\r\n");
	}
	Timer_Flag = 0;

	// 建立中断系统
	status = setup_intr_system(&intc, &axidma, &Timer,
							   DMA_RX_INTR_ID, DMA_TX_INTR_ID, Underflow_INTR_ID, Switch_INT_ID, Amplifier_INT_ID, SOFT_INTR_ID_TO_CPU1, TIMER_IRPT_INTR);
	if (status != XST_SUCCESS)
	{
		xil_printf("Failed intr setup\r\n");
	}

	/************************** 禁用Cache*****************************/
	Xil_SetTlbAttributes(JSON_ADDR, 0x14de2); // 禁用Cache属性	//S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0
	Xil_SetTlbAttributes(UDP_ADDRESS, 0x14de2);
	Xil_SetTlbAttributes(Share_addr, 0x14de2);
	// 修改内存属性，设置为设备内存（不可缓存）
	Xil_SetTlbAttributes(0x40400000, 0xC02); // DMA控制器寄存器区域
	Xil_SetTlbAttributes(0x43C30000, 0xC02); // ADC控制器寄存器区域

	XScuTimer_Start(&Timer); // 启动定时器
	InitializeQueues();
	init_JsonUdp();
	PID_Init_All();

	Xil_Out32(CPU1_PRIORITY_REG, 0xF0); // 提高CPU1优先级
	xil_printf("CPU1: Initialization successfully \r\n");

	/************************** 测试FFT*****************************/
	numHarmonics[0] = 6;
	harmonics[0][0] = 0.0;
	harmonics[0][1] = 0.0;
	harmonics[0][2] = 0.0;
	harmonics[0][3] = 0.0;
	harmonics[0][4] = 0.0;
	harmonics[0][5] = 0.0;

	numHarmonics[4] = 6;
	harmonics[4][0] = 0.0;
	harmonics[4][1] = 0.0;
	harmonics[4][2] = 0.0;
	harmonics[4][3] = 0.0;
	harmonics[4][4] = 0.0;
	harmonics[4][5] = 0.0;
	// 生成交流信号
	str_wr_bram(PID_OFF);

	// 修改二级DA 波形幅度 量程
	for (int i = 0; i < 4; i++)
	{
		Wave_Amplitude[i] = (float)(setACS.Vals[i].U / setACS.Vals[i].UR) * 100;
		Wave_Amplitude[i + 4] = (float)(setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100;
		Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
		Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
	}
	power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF);

	/************************** 测试FFT*****************************/
	// 开启第一次
	Adc_Start(sample_points, sample_points * Wave_Frequency, AD_SAMP_CYCLE_NUMBER);

	while (1)
	{
		if (Timer_Flag)
		{

			/************************** 测试FFT*****************************/
			// 开启AD采样
			Adc_Start(sample_points, sample_points * Wave_Frequency, AD_SAMP_CYCLE_NUMBER);
			
			if (AdcFinish_Flag != 1)
			{
				printf("CPU1 Warning : ADC data is not ready || ");
				printf("ADC Error Count = %d\n", error);
			}
			else
			{
				// 刷新共享内存的缓存，保证数据的一致性
				Xil_DCacheFlushRange((UINTPTR)Share_addr, sample_points * 16 * CHANNL_MAX * AD_SAMP_CYCLE_NUMBER);

				// 重置计算值
				double Phase_reference = 0; // 定义相位基准
				lineAC.totalP = 0.0;
				lineAC.totalQ = 0.0;
				lineAC.totalPF = 0.0;
				for (int i = 0; i < 4; i++)
				{
					// 分析FFT
					double harmonic_info_U[HarmNumberMax][3] = {0}; // 创建用于存储谐波的数组
					double harmonic_info_I[HarmNumberMax][3] = {0};

					//				AnalyzeWaveform(harmonic_info_U, i);	//分析谐波 交流表
					//				AnalyzeWaveform(harmonic_info_I, i + 5);

					// 使用两块DDR地址交替进行FFT分析
					AnalyzeWaveform_WithDDR(harmonic_info_U, i, Share_addr, sample_points * Wave_Frequency, Wave_Frequency);
					AnalyzeWaveform_WithDDR(harmonic_info_I, i + 4, Share_addr, sample_points * Wave_Frequency, Wave_Frequency);

					if (i == 0)
					{
						// 定义相位基准
						Phase_reference = harmonic_info_U[0][2];
					}
					// lineAC - 将分析后的结果填充到UDP结构体里
					// 获取电压和电流量程索引
					int idx_u = get_voltage_index_by_value(setACS.Vals[i].UR);
					int idx_i = get_current_index_by_value(setACS.Vals[i].IR);

					lineAC.f[i] = harmonic_info_U[0][0];												  // 频率
					lineAC.ur[i] = setACS.Vals[0].UR;													  // 电压档位
					lineAC.u[i] = (harmonic_info_U[0][1] / AD_Correct[i][idx_u]) * setACS.Vals[i].UR;	  // 电压幅值 V
					lineAC.ir[i] = setACS.Vals[0].IR;													  // 电流档位
					lineAC.i[i] = (harmonic_info_I[0][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR; // 电流 A
					lineAC.phu[i] = harmonic_info_U[0][2] - Phase_reference;							  // 电压相位 角度制（UA为参考）
					if (lineAC.phu[i] < 0)
					{
						lineAC.phu[i] += 360;
					}
					lineAC.phi[i] = harmonic_info_I[0][2] - Phase_reference; // 电流相位（UA为参考）
					if (lineAC.phi[i] < 0)
					{
						lineAC.phi[i] += 360;
					}
					lineAC.p[i] = (lineAC.u[i] * lineAC.i[i] * cos(lineAC.phi[i] * M_PI / 180.0f)); // 有功功率
					lineAC.q[i] = (lineAC.u[i] * lineAC.i[i] * sin(lineAC.phi[i] * M_PI / 180.0f)); // 无功功率
					lineAC.pf[i] = cos(lineAC.phi[i] * M_PI / 180.0f);								// 功率因数

					// 累加到总功率
					lineAC.totalP += lineAC.p[i];
					lineAC.totalQ += lineAC.q[i];

					// 初始化总谐波畸变率变量
					double thdu = 0.0;
					double thdi = 0.0;
					// 计算电压总谐波畸变率 (THDU)
					if (harmonic_info_U[0][1] >= 0.0001)
					{ // 避免除以零
						double sum_of_squares_u = 0.0;
						// 遍历从2次谐波到32次谐波
						for (int h = 1; h < 32; h++)
						{
							// 计算第i次谐波的比值
							double harmonic_ratio_u = harmonic_info_U[h][1] / harmonic_info_U[0][1];
							// 累加平方
							sum_of_squares_u += harmonic_ratio_u * harmonic_ratio_u;
						}
						// 计算平方和的平方根，得到THD
						thdu = sqrt(sum_of_squares_u);
					}
					else
					{
						// 基波幅值为零，无法计算THD，可能需要处理这种特殊情况
						thdu = 0.0;
					}
					// 计算电流总谐波畸变率 (THDI)
					if (harmonic_info_I[0][1] >= 0.0001)
					{ // 避免除以零
						double sum_of_squares_i = 0.0;
						// 遍历从2次谐波到32次谐波
						for (int h = 1; h < 32; h++)
						{
							// 计算第i次谐波的比值
							double harmonic_ratio_i = harmonic_info_I[h][1] / harmonic_info_I[0][1];
							// 累加平方
							sum_of_squares_i += harmonic_ratio_i * harmonic_ratio_i;
						}
						// 计算平方和的平方根，得到THD
						thdi = sqrt(sum_of_squares_i);
					}
					else
					{
						// 基波幅值为零，无法计算THD，可能需要处理这种特殊情况
						thdi = 0.0;
					}
					// 保存结果
					lineAC.thdu[i] = thdu * 100.0;
					lineAC.thdi[i] = thdi * 100.0;

					// lineHarm
					for (int j = 0; j < HarmNumberMax; j++)
					{
						lineHarm.harm[i].u[j] = (harmonic_info_U[j][1] / AD_UA_Correct) * setACS.Vals[0].UR;
						lineHarm.harm[i].i[j] = (harmonic_info_I[j][1] / AD_IA_Correct) * setACS.Vals[0].IR;
						lineHarm.harm[i].phu[j] = harmonic_info_U[j][2];
						lineHarm.harm[i].phi[j] = harmonic_info_I[j][2];
					}
				}
				// 总功率因数
				double totalApparentPower = sqrt(lineAC.totalP * lineAC.totalP + lineAC.totalQ * lineAC.totalQ);
				if (totalApparentPower > 0)
				{
					lineAC.totalPF = lineAC.totalP / totalApparentPower;
				}
				else
				{
					lineAC.totalPF = 0.0; // 避免除以零错误，设置功率因数为0
				}

				/************************** 测试FFT*****************************/
			}

			/*2 回报UDP结构体*/
			Timer_Flag = 0;
			ReportUDP_Structure(reportStatus);

			/*3 PID闭环调整输出*/
			// 生成交流信号
			str_wr_bram(PID_OFF);
			// 调试
			// printf("True PhUA= %3.4f || PhUB= %3.4f || PhUC= %3.4f || PhUX= %3.4f\n", lineAC.phu[0], lineAC.phu[1], lineAC.phu[2], lineAC.phu[3]);
			// printf("True PhIA= %3.4f || PhIB= %3.4f || PhIC= %3.4f || PhIX= %3.4f\n", lineAC.phi[0], lineAC.phi[1], lineAC.phi[2], lineAC.phi[3]);

			// printf("True UA= %.6f || UB= %.6f || UC= %.6f || UX= %.6f || SET  UA= %.4f\n", lineAC.u[0], lineAC.u[1], lineAC.u[2], lineAC.u[3], lineAC.ur[0] * Wave_Amplitude[0] / 100);
			// printf("True IA= %.4f || IB= %.4f || IC= %.4f || IX= %.4f || SET  IA= %.4f\n\n", lineAC.i[0], lineAC.i[1], lineAC.i[2], lineAC.i[3], lineAC.ir[0] * Wave_Amplitude[4] / 100);

			//  控制功放
			power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF);

			/*4 读故障信号*/
			RdSerial();
		}
	}
}

void reinitialize_dma_controller()
{
	// 完全重置DMA控制器
	XAxiDma_Reset(&axidma);

	// 等待重置完成
	int timeout = RESET_TIMEOUT_COUNTER;
	while (timeout)
	{
		if (XAxiDma_ResetIsDone(&axidma))
			break;
		timeout -= 1;
	}

	// 重新配置DMA
	XAxiDma_Config *config = XAxiDma_LookupConfig(DMA_DEV_ID);
	XAxiDma_CfgInitialize(&axidma, config);

	// 重新设置中断
	XAxiDma_IntrDisable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
}
