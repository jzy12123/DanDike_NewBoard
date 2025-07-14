/******************************************************************************

 ******************************************************************************/
/* Xilinx includes. */
#include "xil_printf.h"
#include "xil_mmu.h"
#include "xparameters.h"
/*user includes*/
#include "Amplifier_Switch.h"
#include "ADDA.h"
#include "Communications_Protocol.h"
#include "PID.h"
#include "Msg_que.h"
#include "My_kissFft.h"
#include "Rc64.h"
#include "mutex_utils.h"
#include "gps.h"
#include "Timer_sync.h"
#include "soft_timer.h"
#include "8025IIC.h"
#include "IIC_Master.h"
#include "power_pulse.h"

// ================= 功能函数声明 =================
static void RunADCPIDCycle(void);
// ===============================================
int main()
{
	sleep(30); // 必须要有等待linux启动
	xil_printf("\r\n");
	xil_printf("-----------------------------------------------------------------------------\r\n");
	xil_printf("CPU1: Starting...\r\n");
	int status;
	RTC_Time_t rtc_time_read;
	Out_RealTime time_from_rtc_to_soft;
	xil_printf("CPU1: Initializing RTC | RC64 IIC Controller...\r\n");
	// IIC初始化
	status = IIC_Master_Init();
	if (status != XST_SUCCESS)
	{
		xil_printf("FATAL: RTC | RC64 IIC Bus Initialization Failed. Halting.\r\n");
	}
	// 从EEPROM读取校准参数
	RC64_ReadCalibData();

	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: AXI IIC Init for RTC8025 Failed! RTC will not be used.\r\n");
	}
	else
	{
		xil_printf("CPU1: AXI IIC for RTC8025 Initialized.\r\n");

		xil_printf("CPU1: Attempting to read from RTC to initialize SoftTimer...\r\n");
		if (Rtc8025_GetTime(RTC_AXI_IIC_BASEADDR, &rtc_time_read) != XST_SUCCESS || !is_rtc_time_valid(&rtc_time_read))
		{
			/******************** 新增：无效时间处理 ********************/
			xil_printf("CPU1: RTC data is invalid or read failed. Initializing RTC to a default time.\r\n");
			RTC_Time_t default_time = {0, 0, 12, 5, 8, 6, 25}; // 2025年6月8日, 星期五, 12:00:00
			if (Rtc8025_SetTime(RTC_AXI_IIC_BASEADDR, &default_time) == XST_SUCCESS)
			{
				xil_printf("CPU1: RTC set to default: 2025-06-08 12:00:00\r\n");
				// 重新读取以确认
				Rtc8025_GetTime(RTC_AXI_IIC_BASEADDR, &rtc_time_read);
			}
			else
			{
				xil_printf("CPU1: FATAL: Failed to set default RTC time.\r\n");
			}
			/************************************************************/
		}

		// 此时 rtc_time_read 中应为有效时间
		xil_printf("CPU1: Using RTC time for initialization: 20%02d-%02d-%02d Wk:%d %02d:%02d:%02d\r\n",
				   rtc_time_read.year, rtc_time_read.month, rtc_time_read.day,
				   rtc_time_read.week, rtc_time_read.hour,
				   rtc_time_read.min, rtc_time_read.sec);

		// 将从RTC读取的时间填充到软时钟的结构体
		time_from_rtc_to_soft.year = 2000 + rtc_time_read.year;
		time_from_rtc_to_soft.month = rtc_time_read.month;
		time_from_rtc_to_soft.day = rtc_time_read.day;
		time_from_rtc_to_soft.hour = rtc_time_read.hour;
		time_from_rtc_to_soft.min = rtc_time_read.min;
		time_from_rtc_to_soft.sec = rtc_time_read.sec;

		int rtc_week_val = rtc_time_read.week;
		int iso_weekday_from_rtc = (rtc_week_val == 0) ? 7 : rtc_week_val;
		time_from_rtc_to_soft.week = 1 << (iso_weekday_from_rtc - 1);

		time_from_rtc_to_soft.pps_clr_en = true;
		time_from_rtc_to_soft.bm_encode_en = false;
		time_from_rtc_to_soft.bm_decode_en = false;

		write_soft_timer(&time_from_rtc_to_soft);
		xil_printf("CPU1: SoftTimer initialized by RTC8025.\r\n");
	}

	xil_printf("CPU1: Initializing GPS UART...\r\n");
	status = UartLiteGpsInit(GPS_UARTLITE_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: GPS UART Init Failed.\r\n");
		return XST_FAILURE;
	}
	xil_printf("CPU1: GPS UART Initialized.\r\n");

	xil_printf("CPU1: Initializing GPS Timer...\r\n");
	status = GpsTtcTimerInit(GPS_TTC_DEVICE_ID); // GPS超时定时器初始化
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: GPS Timer Initial Failed\r\n");
		return XST_FAILURE;
	}
	/************************** DMA初始化 *****************************/
	xil_printf("CPU1: Initializing DMA...\r\n");
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

	/************************** 定时器初始化 *****************************/
	xil_printf("CPU1: Initializing Timer...\r\n");
	status = timer_init(&Timer); // 定时器初始化
	if (status != XST_SUCCESS)
	{
		xil_printf("Timer Initial Failed\r\n");
	}

	xil_printf("CPU1: Initializing Debounce Timer...\r\n");
	status = debounce_timer_init();
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: Debounce Timer Initial Failed\r\n");
	}
	/************************** 建立中断系统 *****************************/
	xil_printf("CPU1: Initializing Interrupt System...\r\n");
	status = setup_intr_system(&intc, &Timer, &DebounceTimer, &GpsUartLiteInst, &GpsTtcTimerInst);
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: Failed intr setup\r\n");
	}

	/************************** 禁用Cache*****************************/
	Xil_SetTlbAttributes(JSON_ADDR, 0x14de2); // 禁用Cache属性	//S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0
	Xil_SetTlbAttributes(UDP_ADDRESS, 0x14de2);
	Xil_SetTlbAttributes(Share_addr, 0x14de2);
	Xil_SetTlbAttributes(0x40400000, 0xC02); // DMA控制器寄存器区域
	Xil_SetTlbAttributes(0x43C30000, 0xC02); // ADC控制器寄存器区域

	/************************** 其余模块初始化 *****************************/
	xil_printf("CPU1: Initializing other modules...\r\n");
	InitializeQueues();
	init_JsonUdp();
	PID_Init_All();
	TimeSync_Init();
	PowerPulse_Init();
	// /*******************************************************************************************/
	// /* VITIS 裸机调试 - 手动设置启动参数                                         */
	// /*******************************************************************************************/
	// xil_printf("CPU1: VITIS DEBUG - Manually setting startup parameters...\r\n");
	// memset(Wave_Amplitude, 0, sizeof(Wave_Amplitude)); // 幅值全部清零
	// memset(harmonics, 0, sizeof(harmonics));		   // 谐波清零
	// memset(numHarmonics, 0, sizeof(numHarmonics));
	// // 1. 设置交流输出参数，模拟 "SetACS" 指令
	// Wave_Frequency = 50.0f;		// 设置频率为50Hz

	// setACS.Vals[0].U =6.5;			// 设置电压幅值为0.5V
	// setACS.Vals[0].I_ = 5.0;
	// setACS.Vals[1].U = 6.5;
	// setACS.Vals[1].I_ = 5.0;
	// setACS.Vals[2].U = 6.5;
	// setACS.Vals[2].I_ = 5.0;
	// setACS.Vals[3].U = 6.5;
	// setACS.Vals[3].I_ = 5.0;
	// for (int i = 0; i < 4; i++)		// 遍历4个通道
	// {
	// 	// 设置电压通道参数
	// 	setACS.Vals[i].UR = 6.5f;											 // 电压量程设置为6.5V

	// 	Wave_Amplitude[i] = (setACS.Vals[i].U / setACS.Vals[i].UR) * 100.0f; // 计算幅值百分比
	// 	Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);				 // 获取量程对应的硬件编码

	// 	// 设置电流通道参数 (这里我们让电流为0，只测试电压)
	// 	setACS.Vals[i].IR = 5.0f; // 电流量程设置为5A

	// 	Wave_Amplitude[i + 4] = (setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100.0f; // 计算幅值百分比				 // 获取量程对应的硬件编码
	// 	Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
	// }
	// xil_printf("CPU1: VITIS DEBUG - Waveform parameters set for 6.5V output, 0A current.\r\n");

	// // 2. 设置设备运行状态标志
	// devState.bACRunning = 1;  // 设置为运行状态 (1=运行)
	// devState.bClosedLoop = 0; // 设置为开环模式，调试初期避免PID控制器干扰
	// xil_printf("CPU1: VITIS DEBUG - AC Running State ENABLED (bACRunning = 1), Open Loop Mode (bClosedLoop = 0).\r\n");

	// // 3. 设置参数更新标志，以触发主循环中的硬件应用逻辑
	// dac_parameters_updated_by_command = true;
	// xil_printf("CPU1: VITIS DEBUG - Parameter update flag SET (dac_parameters_updated_by_command = true).\r\n");

	// /************************** 初始化完成，准备进入主循环 *****************************/
	xil_printf("CPU1: Start Main Timer...\r\n");
	XScuTimer_Start(&Timer);											  // 启动主循环定时器
	const char *arm_version_for_print = get_version_string(ARM_Ver_Full); // 获取ARM版本信息
	xil_printf("CPU1: Initialization successfully || ARM Version: %s\r\n", arm_version_for_print);
	xil_printf("-----------------------------------------------------------------------------\r\n");
	//开关量初始化 放到前面会死机
	OnOff_Start(bit_8, 0);
	OnOff_Start(bit_8, 1);

	/*******************************************************************************************/
	while (1)
	{
		/* 1. 应用硬件参数的逻辑（如果被JSON指令修改） */
		if (dac_parameters_updated_by_command)
		{

			if (devState.bACRunning == 1) // 运行状态
			{
				// 根据当前的 Wave_Amplitude, Phase_shift, Wave_Range, enable, harmonic settings, PID state 准备输出

				enable = 0x00; // 先清零，根据幅值重新计算
				for (int i = 0; i < 8; ++i)
				{
					if (Wave_Amplitude[i] > 0.001f)
					{ // 幅值大于阈值才使能通道和功放
						enable |= (1 << i);
					}
				}
				str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);
				power_amplifier_control(Wave_Amplitude, Wave_Range, (devState.bClosedLoop == 1 ? PID_ON : PID_OFF), POWAMP_ON);
			}
			else if (devState.bACRunning == 2) // 暂停状态
			{
				str_wr_bram(PID_OFF); // 使用暂停前的闭环状态
				// 硬件输出幅值清零，但全局Wave_Amplitude保留暂停前的值
				float temp_Wave_Amplitude[8];								 // 为暂停状态声明一个临时幅值数组
				memset(temp_Wave_Amplitude, 0, sizeof(temp_Wave_Amplitude)); // 将临时幅值数组清零
				power_amplifier_control(temp_Wave_Amplitude, Wave_Range, PID_OFF, target_powamp_enable_state_after_pause);
			}
			else // 停止状态 (devState.bACRunning == 0)
			{
				// Wave_Amplitude 已经在 handle_SetACStatus 中被设为0
				// enable 已经在 handle_SetACStatus 中被设为0x00
				str_wr_bram(PID_OFF);
				power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
			}

			usleep(50000);							   // 确保硬件执行的延时
			dac_parameters_updated_by_command = false; // 清除标志
			udp_data_changed_flag = true;			   // 触发UDP回报当前状态
		}

		/*2 AC交流源 ADC采集与处理 */
		// 无论运行还是暂停，只要不是停止状态，都尝试获取锁并执行ADC
		if ((devState.bACRunning == 1 || devState.bACRunning == 2) && acquire_resource_lock(LOCK_OWNER_ADC, MUTEX_ADC_ACQUIRE_TIMEOUT_US)) // 检查交流源是否配置为运行状态
		{
			// AdcFinish_Flag 在 Adc_Start 中被清零
			Adc_Start(sample_points, sample_points * Wave_Frequency, AD_SAMP_CYCLE_NUMBER); // 启动ADC采样

			// 等待ADC采集和初步处理完成 (AdcFinish_Flag 由中断服务程序 rx_intr_handler 设置)
			// 此等待过程期间，锁 LOCK_OWNER_ADC 保持被持有状态
			uint32_t adc_wait_timeout_us = 350000; // ADC采样理论320ms，这里设置350ms超时
			uint32_t adc_poll_interval_us = 1000;  // 1ms轮询间隔
			uint32_t adc_elapsed_time_us = 0;

			while (!AdcFinish_Flag && adc_elapsed_time_us < adc_wait_timeout_us)
			{
				usleep(adc_poll_interval_us);
				adc_elapsed_time_us += adc_poll_interval_us;
			}
			release_resource_lock(LOCK_OWNER_ADC); // ADC操作完成后立即释放锁，无论成功与否
			// 当前正在采集中，检查是否完成
			if (AdcFinish_Flag == 1)
			{
				RunADCPIDCycle(); // 执行FFT计算、PID调整和功放输出等
			}
			else
			{
				// ADCDMA失败
				//  printf("ADC NotReady !\r\n");
			}
		}
		/*AC交流源关闭或者没有获得锁*/
		else
		{
			usleep(10000); // 延时10ms
		}
	}
}

void RunADCPIDCycle(void)
{
	// 刷新共享内存的缓存，保证数据的一致性
	Xil_DCacheFlushRange((UINTPTR)Share_addr, sample_points * 16 * CHANNL_MAX * AD_SAMP_CYCLE_NUMBER);
	// 重置计算值
	double Phase_reference = 0; // 定义相位基准
	lineAC.totalP = 0.0;
	lineAC.totalQ = 0.0;
	lineAC.totalPF = 0.0;

	// 循环处理4个通道（A, B, C, X），但只累加前3个通道的总功率
	for (int i = 0; i < 4; i++)
	{
		// 分析FFT
		double harmonic_info_U[HarmNumberMax][3] = {0}; // 创建用于存储谐波的数组
		double harmonic_info_I[HarmNumberMax][3] = {0};

		// AC交流源分析
		AnalyzeWaveform_AcSource(harmonic_info_U, i, Share_addr, sample_points * Wave_Frequency, Wave_Frequency);
		AnalyzeWaveform_AcSource(harmonic_info_I, i + 4, Share_addr, sample_points * Wave_Frequency, Wave_Frequency);

		if (i == 0)
		{
			// 定义相位基准
			Phase_reference = harmonic_info_U[0][2];
		}
		// lineAC - 将分析后的结果填充到UDP结构体里
		// 获取电压和电流量程索引
		int idx_u = get_voltage_index_by_value(setACS.Vals[i].UR);
		int idx_i = get_current_index_by_value(setACS.Vals[i].IR);

		lineAC.f[i] = harmonic_info_U[0][0]; // 频率
		lineAC.ur[i] = setACS.Vals[0].UR;	 // 电压档位

		if (AD_Correct[i][idx_u] == 0)
		{
			lineAC.u[i] = 0;
			// 防止除数为0
			printf("CPU1:Warning: Division by zero in lineAC.u[%d] calculation.\n", i);
		}
		else
		{
			// 电压幅值 V
			lineAC.u[i] = harmonic_info_U[0][1] / AD_Correct[i][idx_u] * setACS.Vals[i].UR;
		}

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

		// 计算电压与电流之间的相位差
		double phase_diff = lineAC.phu[i] - lineAC.phi[i];
		// 确保相位差在-180到180度之间
		if (phase_diff > 180.0)
		{
			phase_diff -= 360.0;
		}

		else if (phase_diff < -180.0)
		{
			phase_diff += 360.0;
		}

		lineAC.p[i] = (lineAC.u[i] * lineAC.i[i] * cos(phase_diff * M_PI / 180.0f)); // 有功功率
		lineAC.q[i] = (lineAC.u[i] * lineAC.i[i] * sin(phase_diff * M_PI / 180.0f)); // 无功功率
		lineAC.pf[i] = cos(phase_diff * M_PI / 180.0f);								 // 功率因数

		// *************** 只累加前三个通道(A, B, C)的功率 ***************
		if (i < 3)
		{
			lineAC.totalP += lineAC.p[i];
			lineAC.totalQ += lineAC.q[i];
		}

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

		/*lineHarm*/
		// 初始化该通道的总功率累加变量
		lineHarm.harm[i].totalP = 0.0;
		lineHarm.harm[i].totalQ = 0.0;

		// 获取电压和电流量程索引
		idx_u = get_voltage_index_by_value(setACS.Vals[i].UR);
		idx_i = get_current_index_by_value(setACS.Vals[i].IR);

		// 存储基波幅值和相位，用于计算百分比和相对相位
		double baseU = harmonic_info_U[0][1];
		double baseI = harmonic_info_I[0][1];

		for (int j = 1; j < HarmNumberMax; j++)
		{
			// 电压和电流幅值处理
			if (j == 1)
			{
				// 基波(j=0)特殊处理
				lineHarm.harm[i].u[j] = (harmonic_info_U[j - 1][1] / AD_Correct[i][idx_u]) * setACS.Vals[i].UR;
				lineHarm.harm[i].i[j] = (harmonic_info_I[j - 1][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR;

				// 基波相位直接采用相对于参考相位的值
				lineHarm.harm[i].phu[j] = harmonic_info_U[j - 1][2] - Phase_reference;
				lineHarm.harm[i].phi[j] = harmonic_info_I[j - 1][2] - Phase_reference;
			}
			else
			{
				// 谐波(j>0)计算为基波的百分比
				if (baseU > 0.0001)
				{ // 避免除以接近零的值
					lineHarm.harm[i].u[j] = (harmonic_info_U[j - 1][1] / baseU) * 100.0;
				}
				else
				{
					lineHarm.harm[i].u[j] = 0.0;
				}

				if (baseI > 0.0001)
				{ // 避免除以接近零的值
					lineHarm.harm[i].i[j] = (harmonic_info_I[j - 1][1] / baseI) * 100.0;
				}
				else
				{
					lineHarm.harm[i].i[j] = 0.0;
				}

				// 谐波相位计算
				double n = j; // 谐波次数
				// 计算相对相位
				double u_relative_phase = harmonic_info_U[j - 1][2] - n * Phase_reference;
				double i_relative_phase = harmonic_info_I[j - 1][2] - n * Phase_reference;
				// 确保相位在0到360度之间
				lineHarm.harm[i].phu[j] = fmod(u_relative_phase + 360.0, 360.0);
				lineHarm.harm[i].phi[j] = fmod(i_relative_phase + 360.0, 360.0);

				switch ((j - 1) % 4)
				{
				case 0: // 1, 5, 7, 11, 13, 17, 19, 23, 25, 29, 31次
					lineHarm.harm[i].phu[j] -= 0.0;
					lineHarm.harm[i].phi[j] -= 0.0;
					break;
				case 1: // 2, 6, 8, 12, 14, 18, 20, 24, 26, 30次
					lineHarm.harm[i].phu[j] -= 270.0;
					lineHarm.harm[i].phi[j] -= 270.0;
					break;
				case 2: // 3, 7, 9, 13, 15, 19, 21, 25, 27, 31次
					lineHarm.harm[i].phu[j] -= 180.0;
					lineHarm.harm[i].phi[j] -= 180.0;
					break;
				case 3: // 4, 8, 10, 14, 16, 20, 22, 26, 28, 32次
					lineHarm.harm[i].phu[j] -= 90.0;
					lineHarm.harm[i].phi[j] -= 90.0;
					break;
				}
				// 确保在0-360度
				lineHarm.harm[i].phu[j] = fmod(lineHarm.harm[i].phu[j], 360.0);
				if (lineHarm.harm[i].phu[j] < 0)
				{
					lineHarm.harm[i].phu[j] += 360;
				}
				lineHarm.harm[i].phi[j] = fmod(lineHarm.harm[i].phi[j], 360.0);
				if (lineHarm.harm[i].phi[j] < 0)
				{
					lineHarm.harm[i].phi[j] += 360;
				}
			}

			// 计算谐波的相位差（角度）
			double phase_diff = lineHarm.harm[i].phu[j] - lineHarm.harm[i].phi[j];

			// 计算谐波的有功和无功功率
			lineHarm.harm[i].p[j] = lineHarm.harm[i].u[j] * lineHarm.harm[i].i[j] * cos(phase_diff * M_PI / 180.0);
			lineHarm.harm[i].q[j] = lineHarm.harm[i].u[j] * lineHarm.harm[i].i[j] * sin(phase_diff * M_PI / 180.0);

			// 累加到总功率
			lineHarm.harm[i].totalP += lineHarm.harm[i].p[j];
			lineHarm.harm[i].totalQ += lineHarm.harm[i].q[j];
		}
		// // 调试信息：打印最终计算出的电压和电流值
		// printf("CPU1_Debug: Channel=%d, Final U=%.4f, Final I=%.4f\r\n", i, lineAC.u[i], lineAC.i[i]);
	}
	// 总功率因数
	double totalApparentPower = sqrt(lineAC.totalP * lineAC.totalP + lineAC.totalQ * lineAC.totalQ);
	if (totalApparentPower > 0.0001) // 增加一个小的阈值防止除以极小值
	{
		lineAC.totalPF = lineAC.totalP / totalApparentPower;
	}
	else
	{
		lineAC.totalPF = 0.0; // 避免除以零错误，设置功率因数为0
	}
	// 标记UDP数据已更新
	udp_data_changed_flag = true;
	dac_parameters_updated_by_command = true;
}
