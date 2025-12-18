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
	status = setup_intr_system(&intc, &Timer, &DebounceTimer, &GpsUartLiteInst, &GpsTtcTimerInst, &SeqTtcInstance);
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
		time_from_rtc_to_soft.bm_encode_en = false;
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
	// devState.nStatusFund = 1;  // 设置为运行状态 (1=运行)
	// devState.bClosedLoop = 0; // 设置为开环模式，调试初期避免PID控制器干扰
	// xil_printf("CPU1: VITIS DEBUG - AC Running State ENABLED (nStatusFund = 1), Open Loop Mode (bClosedLoop = 0).\r\n");

	// // 3. 设置参数更新标志，以触发主循环中的硬件应用逻辑
	// dac_parameters_updated_by_command = true;
	// xil_printf("CPU1: VITIS DEBUG - Parameter update flag SET (dac_parameters_updated_by_command = true).\r\n");

	// /************************** 初始化完成，准备进入主循环 *****************************/
	xil_printf("CPU1: Start Main Timer...\r\n");
	XScuTimer_Start(&Timer);											  // 启动主循环定时器
	const char *arm_version_for_print = get_version_string(ARM_Ver_Full); // 获取ARM版本信息
	xil_printf("CPU1: Initialization successfully || ARM Version: %s\r\n", arm_version_for_print);
	// 开关量初始化 放到前面会死机
	OnOff_Start(bit_8, 0);
	OnOff_Start(bit_8, 1);
	sleep(1);
	Adc_Continuous_Start(); // 启动 ADC 连续采集
	xil_printf("-----------------------------------------------------------------------------\r\n");
	/*******************************************************************************************/

	while (1)
	{
		// ============================================================
		// 1. [最高优先级] ADC 数据处理 (录波 & FFT)
		// ============================================================
		// 查询 DMA 中断产生的标志位
		if (g_ProcessBufferFlag != 0)
		{
			// 处理 500ms 数据块
			// 内部逻辑：
			//   - 分支1: 如果开启了录波，将数据写入共享内存 (不受状态序列影响)
			//   - 分支2: 执行 FFT 和 PID (内部会检查锁，且在状态序列运行时不刷新DAC)
			Process_ADC_Buffer();
		}

		// ============================================================
		// 2. 状态序列运行时的特殊处理
		// ============================================================
		// 如果状态序列正在运行，主循环不能去修改 DAC 参数，也不能执行常规指令的硬件映射
		if (g_StateSeqRuntime.IsRunning)
		{
			// 仅休眠一小段时间释放 CPU，以便让出时间给中断处理录波数据
			usleep(1000);
		}
		else
		{
			// ============================================================
			// 3. 常规稳态控制逻辑 (仅在非状态序列模式下执行)
			// ============================================================

			/* 应用硬件参数的逻辑（如果被JSON指令修改） */
			if (dac_parameters_updated_by_command)
			{
				// 【更新逻辑】：判断是否处于任何“运行”状态
				// 只有当 基波、谐波 或 间谐波 任意一个为 1 (运行) 时，才视为系统运行
				bool is_fund_running = (devState.nStatusFund == 1);
				bool is_harm_running = (devState.nStatusHarm == 1);
				bool is_inharm_running = (devState.nStatusInharm == 1); // 预留

				if (is_fund_running || is_harm_running || is_inharm_running)
				{
					// --- 运行状态 ---
					// 只要有一个组件运行，我们就需要打开功放并计算使能通道

					enable = 0x00; // 先清零，根据幅值重新计算
					for (int i = 0; i < 8; ++i)
					{
						// 判断通道使能：
						// 只要设定了 Wave_Amplitude 且处于任意运行态，我们就使能通道。
						if (Wave_Amplitude[i] > 0.001f)
						{
							enable |= (1 << i);
						}
					}

					// 写入波形数据 (BRAM)
					// 注意：str_wr_bram 内部已更新，会检查运行状态来生成波形
					str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);

					// 控制功放打开 (Hardware Registers)
					power_amplifier_control(Wave_Amplitude, Wave_Range, (devState.bClosedLoop == 1 ? PID_ON : PID_OFF), POWAMP_ON);
				}
				else
				{
					// --- 停止或全暂停状态 ---
					// 所有组件都非运行

					// 写入全0波形
					str_wr_bram(PID_OFF);

					// 关闭功放
					power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
				}

				// 清除更新标志
				dac_parameters_updated_by_command = false;

				// 触发UDP回报当前状态 (告诉上位机状态已变更)
				udp_data_changed_flag = true;
			}

			// 可以在这里添加其他低优先级的任务
			usleep(1000); // 可选：空闲时稍微休眠
		}
	}
}