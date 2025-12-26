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
#include "StateSequence.h"

int main()
{
	sleep(30); // 必须要有等待linux启动
	xil_printf("\r\n");
	xil_printf("-----------------------------------------------------------------------------\r\n");
	xil_printf("CPU1: Starting...\r\n");
	int status;

	/************************** GPS初始化 *****************************/
	xil_printf("CPU1: Initializing GPS UART...\r\n");
	status = UartLiteGpsInit(GPS_UARTLITE_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: GPS UART Init Failed.\r\n");
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

	/************************** 状态序列初始化 *****************************/
	xil_printf("CPU1: Initializing State Sequence Module...\r\n");
	status = StateSequence_Init();
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: State Sequence Init Failed (CDMA or TTC error).\r\n");
	}
	sleep(1);

	/************************** 定时器初始化 *****************************/
	xil_printf("CPU1: Initializing Timer...\r\n");
	status = timer_init(&Timer); // 定时器初始化
	if (status != XST_SUCCESS)
	{
		xil_printf("Timer Initial Failed\r\n");
	}

	// 消抖定时器初始化
	xil_printf("CPU1: Initializing Debounce Timer...\r\n");
	status = debounce_timer_init();
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: Debounce Timer Initial Failed\r\n");
	}

	/************************** 建立中断系统 *****************************/
	xil_printf("CPU1: Initializing Interrupt System...\r\n");
	status = setup_intr_system(&intc, &Timer, &DebounceTimer, &GpsUartLiteInst, &SeqTtcInstance);
	if (status != XST_SUCCESS)
	{
		xil_printf("CPU1: Failed intr setup\r\n");
	}
	sleep(2);

	/************************** IIC初始化 *****************************/
	RTC_Time_t rtc_time_read;
	Out_RealTime time_from_rtc_to_soft;
	xil_printf("CPU1: Initializing RTC | RC64 IIC Controller...\r\n");
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
		time_from_rtc_to_soft.bm_encode_en = true; // 默认开启B码输出
		time_from_rtc_to_soft.bm_decode_en = false;

		write_soft_timer(&time_from_rtc_to_soft);
		xil_printf("CPU1: SoftTimer initialized by RTC8025.\r\n");
	}

	/************************** 禁用Cache*****************************/
	Xil_SetTlbAttributes(JSON_ADDR, 0x14de2); // 禁用Cache属性	//S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0
	Xil_SetTlbAttributes(UDP_ADDRESS, 0x14de2);
	Xil_SetTlbAttributes(Share_addr, 0x14de2);
	Xil_SetTlbAttributes(0x40400000, 0xC02); // DMA控制器寄存器区域
	Xil_SetTlbAttributes(0x43C30000, 0xC02); // ADC控制器寄存器区域

	/************************** 其余模块初始化*****************************/
	xil_printf("CPU1: Initializing other modules...\r\n");
	InitializeQueues();
	init_JsonUdp();
	PID_Init_All();
	TimeSync_Init();
	PowerPulse_Init();
	init_EnergyTest();
	WaveRecord_Init();
	
	OnOff_Start(bit_8, 0); // 开关量初始化 放到前面会死机
	OnOff_Start(bit_8, 1);
	sleep(2);
	Adc_Continuous_Start(); // 启动 ADC 连续采集

	const char *arm_version_for_print = get_version_string(ARM_Ver_Full); // 获取ARM版本信息
	xil_printf("CPU1: Initialization successfully || ARM Version: %s\r\n", arm_version_for_print);
	xil_printf("CPU1: Start Main Timer...\r\n");
	XScuTimer_Start(&Timer);
	xil_printf("-----------------------------------------------------------------------------\r\n");
	/*******************************************************************************************/

	while (1)
	{
		// ============================================================
		// 1. [最高优先级] ADC 数据处理 (录波 & FFT)
		// ============================================================
		if (g_ProcessBufferFlag != 0)
		{
			Process_ADC_Buffer();
		}

		// ============================================================
		// 2. 任务状态监控与上报
		// ============================================================
		// 2.1 开入量SOE上报
		Process_DI_Events();

		// 2.2 独立录波任务监控 (SetTaskWaveRecord)
		// 负责检测录波是否完成，并发送 TaskEvent Success
		WaveRecordTask_Check();

		// ============================================================
		// 3. 硬件控制权互斥分发
		// ============================================================
		if (g_StateSeqRuntime.IsRunning ||g_StateSeqRuntime.IsWaiting ||g_StateSeqRuntime.IsHolding)
		{
			// --- 模式 A: 状态序列运行中 ---
			// 此时 DAC 硬件由 CDMA 和 TTC 中断接管。
			// 禁止执行常规 JSON 指令对应的硬件更新，防止冲突。

			// 仅休眠释放 CPU
			usleep(1000);
		}
		else
		{
			// --- 模式 B: 常规稳态控制 ---
			// 仅在非状态序列模式下，响应 SetACS/SetHarm 等常规指令

			if (dac_parameters_updated_by_command)
			{
				// 【更新逻辑】：判断是否处于任何“运行”状态
				bool is_fund_running = (devState.nStatusFund == 1);
				bool is_harm_running = (devState.nStatusHarm == 1);
				bool is_inharm_running = (devState.nStatusInharm == 1);

				if (is_fund_running || is_harm_running || is_inharm_running)
				{
					// --- 运行状态 ---
					enable = 0x00;
					for (int i = 0; i < 8; ++i)
					{
						if (Wave_Amplitude[i] > 0.001f)
							enable |= (1 << i);
					}

					// 写入波形 (带 PID 状态检查)
					str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);

					// 打开功放
					power_amplifier_control(Wave_Amplitude, Wave_Range,
											(devState.bClosedLoop == 1 ? PID_ON : PID_OFF),
											POWAMP_ON);
				}
				else
				{
					// --- 停止状态 ---
					str_wr_bram(PID_OFF); // 全0波形
					power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
				}

				// 清除标志并触发 UDP 更新
				dac_parameters_updated_by_command = false;
				udp_data_changed_flag = true;
			}
			usleep(1000);
		}
	}
}