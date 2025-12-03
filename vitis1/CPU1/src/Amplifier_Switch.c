/*
 * Amplifier_Switch.c
 *
 *  Created on: 2024年2月1日
 *      Author: saber
 */
#include "Amplifier_Switch.h"
// 全局变量定义
XTtcPs DebounceTimer;
float g_debounce_time_ms = 100.0;					  // 全局可配置的防抖时间，默认100ms
Read_Bit g_onoff_bit_width = bit_8;					  // 新增：定义全局变量并提供默认值
static volatile uint32_t last_onoff_data;			  // 存储最后一次中断触发时的数据
static volatile OnOff_Timestamp_t last_captured_time; // 存储最后一次中断触发的时间戳
static uint32_t previous_stable_onoff_data = 0;		  // 新增: 存储上一次稳定的开入状态
/**
 * @brief 读取串行数据，检测并处理保护故障
 *
 * 该函数用于从指定的地址读取串行数据，并检测是否有保护故障发生。
 * 如果检测到故障，会根据故障情况采取不同的处理措施。
 *
 * @details 函数内部使用了静态变量来保存上一次检测到的故障状态和是否已经检测到故障的标志。
 * 函数首先通过写操作使能串行数据读取，然后从指定的地址读取串行数据。
 * 根据读取的数据判断是否有故障发生，如果有则根据故障情况采取相应的处理措施，
 * 包括上报故障、清除二级功放电压电流的使能以及清除故障锁存等。
 *
 * @note 故障状态由8位表示，前4位代表IXICIBIA（具体含义未明确），后4位代表UXUCUBUA（具体含义未明确）。
 * 如果所有位都为1，则表示无故障；如果出现0，则表示对应位置出现故障。
 */
void RdSerial()
{
	static u8 lastProectFault = 0xFF;  // 保存上一次的故障状态，初始为无故障
	static bool faultDetected = false; // 是否已经检测到一次故障

	Xil_Out32(Amplifier_OnOff_BASEADDR + RdSerial_Status_ADDR, (u32)0x1); // slv_reg15 的 rdserial—enable 置 1
	// ProectFault前4位代表IXICIBIA 后四位代表UXUCUBUA,如果正常则为11111111，出现故障对应的位会变0.
	u8 ProectFault = (u8)Xil_In32(Amplifier_OnOff_BASEADDR + RdSerial_ADDR); // 读 slv_reg2 的 rdserial——dataout

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
	// 尝试获取DAC/功放操作的锁
	if (!acquire_resource_lock(LOCK_OWNER_DAC, MUTEX_DAC_ACQUIRE_TIMEOUT_US))
	{
		printf("CPU1: power_amplifier_control could not acquire lock.\n");
		return; // 获取锁失败，不执行后续操作
	}

	if (enable_amp == POWAMP_ON)
	{
		/*1 配置595 开启使能最高位写1*/
		// 595置1 1595置0;功放start清0
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000002);
		usleep(100);

		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0发送
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1发送
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2发送
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3发送

		// 595置1 1595置0;  功放start置1
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000102);
		usleep(100);

		/*******************************************************************************************/
		/* VITIS 代码修改 - 修正PID调节逻辑                                                         */
		/*******************************************************************************************/
		double Amplifier_PID_Increment[8] = {0};
		if (pid_state == PID_ON)
		{
			for (int i = 0; i < 4; i++)
			{
				// --- 电压通道PID ---
				// 中文注释: 设定点就是期望输出的总有效值
				double total_voltage_target = (setACS.Vals[i].UR * Wave_Amplitude[i]) / 100.0;

				// 中文注释: PID直接比较 "目标总有效值" 和 "测量总有效值(lineAC.u[i])"
				Amplifier_PID_Increment[i] = PID_adjust_amplitude(total_voltage_target, lineAC.u[i], &amplitude_pid[i]);

				// --- 电流通道PID (逻辑相同) ---
				int current_channel_index = i + 4;
				// 中文注释: 设定点就是期望输出的总有效值
				double total_current_target = (setACS.Vals[i].IR * Wave_Amplitude[current_channel_index]) / 100.0;

				// 中文注释: PID直接比较 "目标总有效值" 和 "测量总有效值(lineAC.i[i])"
				Amplifier_PID_Increment[current_channel_index] = PID_adjust_amplitude(total_current_target, lineAC.i[i], &amplitude_pid[current_channel_index]);
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
		/*******************************************************************************************/

		/*3 配置1595*/
		// 595置0 1595置1;  功放start清0
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000001);
		usleep(100);

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
		double correction_ua = calculate_correction(0, idx_ua, Wave_Amplitude[0]);			 // DAC原始输出
		double PID_correction_ua = (32767 / setACS.Vals[0].UR) * Amplifier_PID_Increment[0]; // PID原始输出
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

		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din0_ADDR, UB + UA); // din0发送，8000半幅值，ub + ua
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din1_ADDR, UX + UC); // din1发送，半幅值，ux + uc
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din2_ADDR, IB + IA); // din2发送，ib + ia
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din3_ADDR, IX + IC); // din3发送，ix + ic

		// 595置0 1595置1;  功放start置1
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000101);
		usleep(100);
	}
	else
	{
		/*1 配置595 关闭功放使能 只需要把Wave_Range的最高位写0*/
		// 595置1 1595置0 功放start清0
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000000);
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000002);
		usleep(100);

		// 修改Wave_Range,把第7位清0，为了清空二级功放的硬件保护：
		for (int i = 0; i < CHANNL_MAX; i++)
		{
			Wave_Range[i] &= ~(1 << 7);
		}

		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din0_ADDR, (u32)(Wave_Range[1] << 24) | (Wave_Range[0] << 8)); // ub + ua din0发送
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din1_ADDR, (u32)(Wave_Range[3] << 24) | (Wave_Range[2] << 8)); // ux + uc din1发送
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din2_ADDR, (u32)(Wave_Range[5] << 24) | (Wave_Range[4] << 8)); // ib + ia din2发送
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Din3_ADDR, (u32)(Wave_Range[7] << 24) | (Wave_Range[6] << 8)); // ix + ic din3发送

		// 595置1 1595置0 功放start置1
		Xil_Out32(Amplifier_OnOff_BASEADDR + Amplifier_Status_ADDR, (u32)0x00000102);
		usleep(100);
		print("CPU1: POWAMP Closed!\r\n");

		/*2 清空PID累计值*/
		for (int i = 0; i < 4; i++)
		{
			amplitude_pid[i].integral = 0;
			amplitude_pid[i].prev_error = 0;
		}
	}
	// 释放锁
	release_resource_lock(LOCK_OWNER_DAC);
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
 * @brief 初始化用于防抖的TTC定时器
 * @return 成功返回 XST_SUCCESS, 否则返回 XST_FAILURE
 */
int debounce_timer_init()
{
	XTtcPs_Config *TimerConfig;
	s32 Status;

	// 查找TTC设备配置
	TimerConfig = XTtcPs_LookupConfig(DEBOUNCE_TIMER_DEVICE_ID);
	if (NULL == TimerConfig)
	{
		return XST_FAILURE;
	}
	TimerConfig->InputClockHz = 111111115; // TTC时钟频率设置为 111.111115 MHz
	xil_printf("CPU1: TTC0 clock frequency set to %u Hz based on hardware design.\r\n", (unsigned int)TimerConfig->InputClockHz);

	// 使用修正后的配置初始化TTC设备驱动
	Status = XTtcPs_CfgInitialize(&DebounceTimer, TimerConfig, TimerConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// 设置定时器为间隔模式，并停止它以进行配置
	XTtcPs_SetOptions(&DebounceTimer, XTTCPS_OPTION_INTERVAL_MODE);
	XTtcPs_Stop(&DebounceTimer);

	return XST_SUCCESS;
}

/**
 * @brief 设置并启动防抖定时器 (已修正)
 * @param timeout_ms 需要的防抖延时，单位为毫秒 (ms)
 * @comment 此函数现在会根据传入的延时时间和TTC的实际输入时钟频率，
 * 动态计算并设置正确的Interval和Prescaler值。
 */
void start_debounce_timer(float timeout_ms)
{
	// 检查输入参数有效性，防止除零错误
	if (timeout_ms <= 0.0f)
	{
		printf("Error: Debounce timeout must be positive.\n");
		return;
	}

	// 目标频率 = 1 / 延时(秒)
	// 例如，10ms延时 -> 目标频率 = 1 / 0.01s = 100Hz
	u32 target_freq = (u32)(1000.0f / timeout_ms);

	XInterval Interval;
	u8 Prescaler;

	// 使用Xilinx驱动函数来自动计算最佳的Interval和Prescaler
	// DebounceTimer.Config.InputClockHz 是在 debounce_timer_init 中获取并设置的实际时钟频率
	XTtcPs_CalcIntervalFromFreq(&DebounceTimer, target_freq, &Interval, &Prescaler);

	// 停止定时器以安全地配置它
	XTtcPs_Stop(&DebounceTimer);

	// 设置新计算出的分频和计数值
	XTtcPs_SetPrescaler(&DebounceTimer, Prescaler);
	XTtcPs_SetInterval(&DebounceTimer, Interval);

	// 启动定时器
	XTtcPs_Start(&DebounceTimer);
}

/**
 * @brief 创建并上报开入量事件记录 (DISOE)
 * @param stable_data 稳定的开入数据
 * @param changed_bits 与上次相比发生变化的位
 * @param timestamp 事件发生的时间戳
 * @comment 此函数被 debounce_timer_handler 调用，负责生成新的JSON格式并发送
 */
void report_di_soe_event(uint32_t stable_data, uint32_t changed_bits, const volatile OnOff_Timestamp_t *timestamp)
{
	// 1. 创建顶层JSON对象
	cJSON *report = cJSON_CreateObject();
	cJSON_AddStringToObject(report, "FunType", "Report");
	cJSON_AddStringToObject(report, "FunCode", "DISOE");

	// 2. 创建新的 "Data" 对象
	cJSON *data_obj = cJSON_CreateObject();

	// 3. 确定当前有效的开入通道数
	int num_bits;
	switch (g_onoff_bit_width)
	{
	case bit_16:
		num_bits = 16;
		break;
	case bit_24:
		num_bits = 24;
		break;
	case bit_32:
		num_bits = 32;
		break;
	case bit_8:
	default:
		num_bits = 8;
		break;
	}

	// 4. 创建并填充 "DIVals" 数组 (COS - Change of State)
	cJSON *di_vals_array = cJSON_CreateArray();
	for (int i = 0; i < num_bits; i++)
	{
		// 硬件0代表动作，JSON中1代表动作，所以需要逻辑取反
		int value = 1 - ((stable_data >> i) & 1);
		cJSON_AddItemToArray(di_vals_array, cJSON_CreateNumber(value));
	}
	cJSON_AddItemToObject(data_obj, "DIVals", di_vals_array);

	// 5. 创建并填充 "SOE" 数组 (SOE - Sequence of Events)
	cJSON *soe_array = cJSON_CreateArray();
	// 格式化时间戳字符串
	char time_str[40];
	In_CurrTime current_time;
	read_current_time(&current_time); // 获取当前的年月日
	sprintf(time_str, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
			current_time.curr_year, current_time.curr_month, current_time.curr_day,
			timestamp->hour, timestamp->minute, timestamp->second,
			(unsigned int)(timestamp->sub_sec / 10000)); // 亚秒单位是0.1us, 转为ms

	// 遍历变化的位
	for (int i = 0; i < num_bits; i++)
	{
		if ((changed_bits >> i) & 1)
		{
			cJSON *event_item = cJSON_CreateObject();
			int channel_num = i + 1;
			int value = 1 - ((stable_data >> i) & 1); // 同样需要逻辑取反

			cJSON_AddStringToObject(event_item, "Time", time_str);
			cJSON_AddNumberToObject(event_item, "Chn", channel_num);
			cJSON_AddNumberToObject(event_item, "val", value);

			cJSON_AddItemToArray(soe_array, event_item);
		}
	}
	cJSON_AddItemToObject(data_obj, "SOE", soe_array);

	// 6. 将 "Data" 对象添加到主对象
	cJSON_AddItemToObject(report, "Data", data_obj);

	// 7. 转换JSON为字符串并发送
	char *string = cJSON_PrintUnformatted(report);
	if (string == NULL)
	{
		xil_printf("Failed to print JSON.\r\n");
		cJSON_Delete(report);
		return;
	}

	size_t stringLength = strlen(string);
	char *finalString = (char *)malloc(stringLength + 3);
	if (finalString == NULL)
	{
		xil_printf("Failed to allocate memory for final JSON string.\r\n");
		cJSON_Delete(report);
		free(string);
		return;
	}
	snprintf(finalString, stringLength + 3, "|%s|", string);

	// 写入消息队列
	ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
	if (bytesWritten < 0)
	{
		printf("CPU1: DISOE Report: Failed to write to message queue.\r\n");
	}
	else
	{
		printf("CPU1: DISOE Report Sent: %s\r\n", finalString);
	}
	free(finalString);
	free(string);

	// 8. 清理
	cJSON_Delete(report);
}

/**
 * @brief 开关中断处理函数
 *
 * 当检测到开关中断时，该函数将被调用。
 *
 * 该函数会立即禁用onoff_done中断，以防止在抖动期间重复触发。
 * 然后启动一个一次性防抖定时器，等待指定的防抖时间。
 *
 * @note 防抖时间由全局变量g_debounce_time_ms指定。
 */
void onoff_handler(void)
{
	// 1. 每次中断触发，都重新读取硬件锁存的最新数据和时间戳
	//    这些数据将用于防抖定时器到期后的比较
	OnOff_Read_LatchedData(g_onoff_bit_width, (uint32_t *)&last_onoff_data, &last_captured_time);

	// 2. 启动或重新启动防抖定时器
	start_debounce_timer(g_debounce_time_ms);
}
/**
 * @brief 防抖定时器的中断服务程序 (最终版)
 * @param CallBackRef 回调引用
 * @comment 此函数在防抖延时结束后执行，进行数据稳定新检查，并精确测量TTC延时。
 */
void debounce_timer_handler(void *CallBackRef)
{

	// 1. 停止定时器并清除中断状态
	XTtcPs_Stop(&DebounceTimer);
	XTtcPs_ClearInterruptStatus(&DebounceTimer, XTTCPS_IXR_INTERVAL_MASK);

	// 2. 读取防抖结束后，当前稳定的硬件输入数据 (使用新函数，不更新时间戳)
	uint32_t stable_input_data = OnOff_Read_Current_Input(bit_8);

	// 3. 检查信号是否真的稳定
	if (stable_input_data == last_onoff_data)
	{
		// 信号稳定，是有效事件
		// 计算变化的位
		uint32_t changed_bits = stable_input_data ^ previous_stable_onoff_data;

		if (changed_bits != 0)
		{

			// 将当前的稳定DI值传入状态序列状态机，状态机内部会判断当前Step是否配置了DI触发逻辑
			// 优先处理状态序列跳转
			StateSequence_DI_Check(changed_bits, stable_input_data);

			// 调用辅助函数上报JSON
			report_di_soe_event(stable_input_data, changed_bits, &last_captured_time);

			// 更新上一次的稳定状态
			previous_stable_onoff_data = stable_input_data;

			// 根据当前位宽，更新 lineDI 结构体用于UDP上报
			int num_bits;
			switch (g_onoff_bit_width)
			{
			case bit_16:
				num_bits = 16;
				break;
			case bit_24:
				num_bits = 24;
				break;
			case bit_32:
				num_bits = 32;
				break;
			case bit_8:
			default:
				num_bits = 8;
				break;
			}

			for (int i = 0; i < num_bits; ++i)
			{
				lineDI.DI[i].v = (1 - ((stable_input_data >> i) & 1)); // 核心修改：对硬件状态位进行逻辑反转，与DISOE上报逻辑保持一致
			}
			// 将未使用的位清零，确保UDP报文的整洁
			for (int i = num_bits; i < ChnsDI; ++i)
			{
				lineDI.DI[i].v = 0;
			}
			udp_data_changed_flag = true; // 触发UDP更新
		}
	}
	else
	{
		// 信号不稳定（抖动）
		printf("CPU1: Input bounce detected and ignored. Last captured data: 0x%08lX, but stable data is now: 0x%08lX\r\n", last_onoff_data, stable_input_data);
	}
}

/**
 * @brief 启动开关量模块
 *
 * 该函数启动开关量模块，并根据输入的比特宽度配置寄存器。
 *
 * @param bit_width 开关量模块的比特宽度，即读取的比特数
 */
void OnOff_Start(Read_Bit bit_width, uint8_t start)
{
	uint32_t reg_control_value = 0;
	uint8_t local_start_bit = 0;
	sleep(1); // 等待硬件初始化完成

	// 2. 设置全局位宽变量
	g_onoff_bit_width = bit_width;

	// 3. 配置并启动硬件模块
	local_start_bit = start;
	reg_control_value = (((uint32_t)local_start_bit & 0x1) << 24) | (((g_onoff_bit_width + 1) & 0x7) << 16);
	Xil_Out32(Amplifier_OnOff_BASEADDR + OnOff_Status_ADDR, reg_control_value);
	printf("CPU1: OnOff Module Start to %d-bit Mode, start is %u.\r\n", 8 * (g_onoff_bit_width + 1), local_start_bit);

	usleep(1000); // 短暂延时，确保硬件状态稳定

	if (start == 1)
	{
		// 4. 在硬件启动后，读取真实的初始状态来同步软件变量
		previous_stable_onoff_data = OnOff_Read_Current_Input(g_onoff_bit_width);
		printf("CPU1: Initial DI state synchronized to: 0x%lX\r\n", previous_stable_onoff_data);

		// 5. 初始化开出(DO)通道状态
		g_do_output_state = 0;					   // 软件状态清零
		OnOff_Write_Continuous(g_do_output_state); // 硬件状态清零
		printf("CPU1: Initial DO state set to OFF (0x00).\r\n");

		// 6. 使能中断，准备接收硬件变化通知
		usleep(1000); // 短暂延时，确保硬件状态稳定
		XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_ONOFF_DONE_INTR);
	}
}
/**
 * @brief 停止开关量模块
 *
 * 该函数通过写入特定的值到寄存器来停止开关量模块的工作。
 *
 * @note 本函数会向指定寄存器写入一个值，该值通过参数组合生成，用于控制开关量模块的状态。
 */
void OnOff_Stop()
{
	uint32_t reg8_control_value = 0; // 用于构建写入 slv_reg8 的值
	uint8_t local_start_bit = 0;
	/*开关量模块配置*/
	local_start_bit = 1;
	reg8_control_value = (((uint32_t)local_start_bit & 0x1) << 24);
	Xil_Out32(Amplifier_OnOff_BASEADDR + OnOff_Status_ADDR, reg8_control_value);
	printf("CPU1: OnOff Module Stop.\r\n");
}
/**
 * @brief 连续写入开关量输出数据
 *
 * 在模块被初始化为连续读写模式（如Random_ReadWrite或Time_ReadWrite）后调用。
 * 此函数仅更新需要输出到TPIC6B595的数据，硬件会自动串行移出。
 * 此操作不触发 `done` 信号。
 * @param output_data 要写入的32位数据。
 */
void OnOff_Write_Continuous(uint32_t output_data)
{
	// 直接向OnOff_Write_ADDR (slv_reg13) 写入数据
	Xil_Out32(Amplifier_OnOff_BASEADDR + OnOff_Write_ADDR, output_data);
}

/**
 * @brief 读取锁存的开关量输入数据和时间戳
 *
 * 此函数应在 `done` 中断触发后调用。
 * 它从硬件寄存器中读取因输入变化而被锁存的数据和高精度时间戳。
 * @param read_data 用于存储读取的32位开关量数据的指针。
 * @param timestamp 用于存储读取的时间戳的结构体指针。
 */
void OnOff_Read_LatchedData(Read_Bit bit_width, uint32_t *read_data, volatile OnOff_Timestamp_t *timestamp)
{
	if (!read_data || !timestamp)
	{
		return;
	}

	/*读取开关量数据寄存器*/
	uint32_t raw_onoff_data = Xil_In32(Amplifier_OnOff_BASEADDR + OnOff_Read_ADDR);
	switch (bit_width)
	{
	case bit_8:
		*read_data = raw_onoff_data & 0xFF;
		break;
	case bit_16:
		*read_data = raw_onoff_data & 0xFFFF;
		break;
	case bit_24:
		*read_data = raw_onoff_data & 0xFFFFFF;
		break;
	case bit_32:
	default:
		*read_data = raw_onoff_data;
		break;
	}

	/*读取时间戳寄存器*/
	uint32_t reg11_val, reg12_val, reg13_val;
	uint8_t latch_hour_bcd, latch_minute_bcd, latch_second_bcd;
	uint32_t latch_daysec_bin, latch_subsec_bin;

	uint8_t hour_tens, hour_ones;
	uint8_t minute_tens, minute_ones;
	uint8_t second_tens, second_ones;

	// 从 slv_reg11 读取锁存的时、分、秒 (BCD码)
	reg11_val = Xil_In32(Amplifier_OnOff_BASEADDR + LATCH_TIME_HMS_REG_OFFSET);
	// 根据Verilog文件: slv_reg11: {12'd0, latch_hour[5:0], latch_minute[6:0], latch_second[6:0]}
	latch_hour_bcd = (uint8_t)((reg11_val >> 14) & 0x3F);  // 提取6位BCD小时
	latch_minute_bcd = (uint8_t)((reg11_val >> 7) & 0x7F); // 提取7位BCD分钟
	latch_second_bcd = (uint8_t)(reg11_val & 0x7F);		   // 提取7位BCD秒

	// 从 slv_reg12 读取锁存的日内秒 (二进制)
	reg12_val = Xil_In32(Amplifier_OnOff_BASEADDR + LATCH_DAYSEC_REG_OFFSET);
	// 根据Verilog文件: slv_reg12: {15'd0, latch_daysec[16:0]}
	latch_daysec_bin = reg12_val & 0x1FFFF; // 提取17位二进制日内秒

	// 从 slv_reg13 读取锁存的亚秒 (二进制)
	reg13_val = Xil_In32(Amplifier_OnOff_BASEADDR + LATCH_SUBSEC_REG_OFFSET);
	// 根据Verilog文件: slv_reg13: {8'd0,  latch_subsec[23:0]}
	latch_subsec_bin = reg13_val & 0xFFFFFF; // 提取24位二进制亚秒

	// 将BCD码转换为十进制进行打印
	// 小时 (6位BCD: HH, 高2位为十位(0-2), 低4位为个位(0-9))
	hour_tens = (latch_hour_bcd >> 4) & 0x03;
	hour_ones = latch_hour_bcd & 0x0F;

	// 分钟 (7位BCD: MM, 高3位为十位(0-5), 低4位为个位(0-9))
	minute_tens = (latch_minute_bcd >> 4) & 0x07;
	minute_ones = latch_minute_bcd & 0x0F;

	// 秒 (7位BCD: SS, 高3位为十位(0-5), 低4位为个位(0-9))
	second_tens = (latch_second_bcd >> 4) & 0x07;
	second_ones = latch_second_bcd & 0x0F;

	// 打印信息
	// xil_printf("CPU1: OnOff Latch Time - HH:MM:SS = %d%d:%d%d:%d%d (BCD)\r\n",
	// 		   hour_tens, hour_ones,
	// 		   minute_tens, minute_ones,
	// 		   second_tens, second_ones);

	// 解析并填充时间戳结构体
	timestamp->hour = (uint8_t)(hour_tens * 10 + hour_ones);
	timestamp->minute = (uint8_t)(minute_tens * 10 + minute_ones);
	timestamp->second = (uint8_t)(second_tens * 10 + second_ones);
	timestamp->day_sec = latch_daysec_bin;
	timestamp->sub_sec = latch_subsec_bin;
}

/**
 * @brief 读取当前实时的开关量输入值，不影响锁存的时间戳
 */
u32 OnOff_Read_Current_Input(Read_Bit bit_width)
{
	uint32_t current_data;
	current_data = Xil_In32(Amplifier_OnOff_BASEADDR + OnOff_Read_ADDR);

	// 根据位宽进行屏蔽
	switch (bit_width)
	{
	case bit_8:
		return current_data & 0xFF;
	case bit_16:
		return current_data & 0xFFFF;
	case bit_24:
		return current_data & 0xFFFFFF;
	case bit_32:
	default:
		return current_data;
	}
}
