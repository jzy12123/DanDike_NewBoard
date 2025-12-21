#include "gps.h"
#include "Timer_sync.h" // 引入对时管理模块
#include "soft_timer.h" // 引入软时钟
#include "8025IIC.h"	// 引入硬件RTC
#include "string.h"		// for memset
#include "ADDA.h"		// for intc

XUartLite GpsUartLiteInst;		// GPS Uart实例
XTtcPs GpsTtcTimerInst;			// GPS接收超时TTC定时器实例
GPS_Recv_Ctrl_t GPS_Ctrl_State; // GPS接收控制状态
u8 UART_RX_BUF[1000];
nmea_msg gpsx;
// const u8 *fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode字符串
// const u32 BAUD_id[9] = {4800,9600,19200,38400,57600,115200,230400,460800,921600};//模块支持波特率数组

/**
 * @brief 初始化GPS串口
 */
int UartLiteGpsInit(u16 device_id)
{ //
	int Status;
	XUartLite_Config *ConfigPtr;

	GPS_Ctrl_State.uart_cont = 0;						 //
	GPS_Ctrl_State.REV_Finish_Flag = 0;					 //
	memset((void *)UART_RX_BUF, 0, sizeof(UART_RX_BUF)); //
	memset(&gpsx, 0, sizeof(nmea_msg));					 //

	ConfigPtr = XUartLite_LookupConfig(device_id); //
	if (NULL == ConfigPtr)
		return XST_FAILURE; //

	Status = XUartLite_CfgInitialize(&GpsUartLiteInst, ConfigPtr, ConfigPtr->RegBaseAddr); //
	if (Status != XST_SUCCESS)
		return XST_FAILURE; //

	XUartLite_SetRecvHandler(&GpsUartLiteInst, GpsUartRecvHandler, &GpsUartLiteInst); //
	XUartLite_EnableInterrupt(&GpsUartLiteInst);									  //
	XUartLite_ResetFifos(&GpsUartLiteInst);											  //

	return XST_SUCCESS;
}

/**
 * @brief 初始化GPS超时TTC定时器
 */
int GpsTtcTimerInit(u16 device_id)
{ //
	XTtcPs_Config *TimerConfig;
	s32 Status;

	TimerConfig = XTtcPs_LookupConfig(device_id); //
	if (NULL == TimerConfig)
		return XST_FAILURE; //

	Status = XTtcPs_CfgInitialize(&GpsTtcTimerInst, TimerConfig, TimerConfig->BaseAddress); //
	if (Status != XST_SUCCESS)
		return XST_FAILURE; //

	XTtcPs_SetOptions(&GpsTtcTimerInst, XTTCPS_OPTION_INTERVAL_MODE); //

	XInterval Interval;
	u8 Prescaler;
	XTtcPs_CalcIntervalFromFreq(&GpsTtcTimerInst, 1.0 / GPS_TTC_TIMEOUT_SECONDS, &Interval, &Prescaler); //
	XTtcPs_SetPrescaler(&GpsTtcTimerInst, Prescaler);													 //
	XTtcPs_SetInterval(&GpsTtcTimerInst, Interval);														 //

	XTtcPs_EnableInterrupts(&GpsTtcTimerInst, XTTCPS_IXR_INTERVAL_MASK); //

	return XST_SUCCESS;
}

/**
 * @brief GPS串口接收中断处理函数
 */
void GpsUartRecvHandler(void *CallBackRef, unsigned int EventData)
{ //
	XUartLite *UartLiteInstancePtr = (XUartLite *)CallBackRef;
	u8 RecvChar;

	while (XUartLite_IsReceiveEmpty(UartLiteInstancePtr->RegBaseAddress) == FALSE)
	{
		RecvChar = XUartLite_ReadReg(UartLiteInstancePtr->RegBaseAddress, XUL_RX_FIFO_OFFSET);
		if (GPS_Ctrl_State.uart_cont < (sizeof(UART_RX_BUF) - 2))
		{
			UART_RX_BUF[GPS_Ctrl_State.uart_cont++] = RecvChar;
		}
		else
		{
			GPS_Ctrl_State.uart_cont = 0; // 缓冲区溢出，复位
		}
		XTtcPs_Stop(&GpsTtcTimerInst);
		XTtcPs_Start(&GpsTtcTimerInst);
	}
}

/**
 * @brief GPS接收超时中断处理函数 (修改后)
 */
void GpsTimeoutHandler(void *CallBackRef)
{ //
	XTtcPs *TimerInstancePtr = (XTtcPs *)CallBackRef;
	XTtcPs_ClearInterruptStatus(TimerInstancePtr, XTTCPS_IXR_INTERVAL_MASK);
	XTtcPs_Stop(TimerInstancePtr);

	// --- 修改 --- 仅在GPS对时进行中才处理
	if (g_TimeSyncManager.status != TIME_SYNC_IN_PROGRESS || g_TimeSyncManager.current_mode != SYNC_MODE_GPS)
	{
		return;
	}
	if (GPS_Ctrl_State.uart_cont > 0)
	{
		UART_RX_BUF[GPS_Ctrl_State.uart_cont] = '\0';
		GPS_Analysis(&gpsx, (u8 *)UART_RX_BUF);

		// 检查是否收到有效的RMC数据
		if (gpsx.rmc_status == 'A')
		{									//
			GPS_ConvertUTCToBeijing(&gpsx); //

			if (gpsx.utc.year > 2020)
			{ // 基本的年份有效性检查 //
				Out_RealTime new_time_to_set_soft;
				RTC_Time_t new_time_to_set_rtc;

				new_time_to_set_soft.year = gpsx.utc.year;
				new_time_to_set_soft.month = gpsx.utc.month;
				new_time_to_set_soft.day = gpsx.utc.date;
				new_time_to_set_soft.hour = gpsx.utc.hour;
				new_time_to_set_soft.min = gpsx.utc.min;
				new_time_to_set_soft.sec = gpsx.utc.sec;
				int weekday_iso = calculate_weekday_iso(gpsx.utc.year, gpsx.utc.month, gpsx.utc.date);
				new_time_to_set_soft.week = (weekday_iso > 0) ? (1 << (weekday_iso - 1)) : 1;
				new_time_to_set_soft.pps_clr_en = true;
				new_time_to_set_soft.bm_encode_en = true; // 默认开启B码输出
				new_time_to_set_soft.bm_decode_en = false;
				write_soft_timer(&new_time_to_set_soft);

				new_time_to_set_rtc.year = (u8)(gpsx.utc.year % 100);
				new_time_to_set_rtc.month = (u8)gpsx.utc.month;
				new_time_to_set_rtc.day = (u8)gpsx.utc.date;
				new_time_to_set_rtc.hour = (u8)gpsx.utc.hour;
				new_time_to_set_rtc.min = (u8)gpsx.utc.min;
				new_time_to_set_rtc.sec = (u8)gpsx.utc.sec;
				new_time_to_set_rtc.week = (weekday_iso == 7) ? 0 : (u8)weekday_iso;

				if (Rtc8025_SetTime(RTC_AXI_IIC_BASEADDR, &new_time_to_set_rtc) == XST_SUCCESS)
				{																  //
					xil_printf("GPS SYNC SUCCESS: SoftTimer and RTC updated.\n"); //
				}
				else
				{
					xil_printf("GPS SYNC: SoftTimer updated, RTC set FAILED.\n"); //
				}

				// --- 核心修改 ---: 通知对时管理器任务成功
				NotifySyncSuccess();
			}
			else
			{
				// 年份无效，也视为失败
				NotifySyncFailure();
			}
		}
		else
		{
			// RMC状态无效，也视为失败
			NotifySyncFailure();
		}
	}
	else
	{
		// 缓冲区为空，也视为失败
		NotifySyncFailure();
	}
	GPS_Ctrl_State.uart_cont = 0; // 清理缓冲区
}
// 从buf里面得到第cx个逗号所在的位置
// 返回值:0~0XFE,代表逗号所在位置的偏移
// 0XFF,代表不存在第cx个逗号
u8 NMEA_Comma_Pos(u8 *buf, u8 cx)
{
	u8 *p = buf;
	while (cx)
	{
		if (*buf == '*' || *buf < ' ' || *buf > 'z')
			return 0XFF; // 遇到'*'或者非法字符,则不存在第cx个逗号
		if (*buf == ',')
			cx--;
		buf++;
	}
	return buf - p;
}

// m^n函数
// 返回值:m^n次方
u32 NMEA_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--)
		result *= m;
	return result;
}

// str转换为数字,以','或者'*'结束
// buf:数字存储区
// dx:小数点位数,返回给调用函数
// 返回值:转换后的数值
int NMEA_Str2num(u8 *buf, u8 *dx)
{
	u8 *p = buf;
	u32 ires = 0, fres = 0;
	u8 ilen = 0, flen = 0, i;
	u8 mask = 0;
	int res;
	while (1) // 得到整数和小数的长度
	{
		if (*p == '-')
		{
			mask |= 0X02;
			p++;
		} // 是负数
		if (*p == ',' || (*p == '*'))
			break; // 遇到结束了
		if (*p == '.')
		{
			mask |= 0X01;
			p++;
		} // 遇到小数点了
		else if (*p > '9' || (*p < '0')) // 有非法字符
		{
			ilen = 0;
			flen = 0;
			break;
		}
		if (mask & 0X01)
			flen++;
		else
			ilen++;
		p++;
	}
	if (mask & 0X02)
		buf++;				   // 去掉负号
	for (i = 0; i < ilen; i++) // 得到整数部分数据
	{
		ires += NMEA_Pow(10, ilen - 1 - i) * (buf[i] - '0');
	}
	if (flen > 5)
		flen = 5;			   // 最多取5位小数
	*dx = flen;				   // 小数点位数
	for (i = 0; i < flen; i++) // 得到小数部分数据
	{
		fres += NMEA_Pow(10, flen - 1 - i) * (buf[ilen + 1 + i] - '0');
	}
	res = ires * NMEA_Pow(10, flen) + fres;
	if (mask & 0X02)
		res = -res;
	return res;
}

// 分析GPGSV信息
// gpsx:nmea信息结构体
// buf:接收到的GPS数据缓冲区首地址
void NMEA_GPGSV_Analysis(nmea_msg *gpsx, u8 *buf)
{
	u8 *p, *p1, dx;
	u8 len, i, j, slx = 0;
	u8 posx;
	p = buf;
	p1 = (u8 *)strstr((const char *)p, "$GPGSV");
	len = p1[7] - '0';			  // 得到GPGSV的条数
	posx = NMEA_Comma_Pos(p1, 3); // 得到可见卫星总数
	if (posx != 0XFF)
		gpsx->svnum = NMEA_Str2num(p1 + posx, &dx);
	for (i = 0; i < len; i++)
	{
		p1 = (u8 *)strstr((const char *)p, "$GPGSV");
		for (j = 0; j < 4; j++)
		{
			posx = NMEA_Comma_Pos(p1, 4 + j * 4);
			if (posx != 0XFF)
				gpsx->slmsg[slx].num = NMEA_Str2num(p1 + posx, &dx); // 得到卫星编号
			else
				break;
			posx = NMEA_Comma_Pos(p1, 5 + j * 4);
			if (posx != 0XFF)
				gpsx->slmsg[slx].eledeg = NMEA_Str2num(p1 + posx, &dx); // 得到卫星仰角
			else
				break;
			posx = NMEA_Comma_Pos(p1, 6 + j * 4);
			if (posx != 0XFF)
				gpsx->slmsg[slx].azideg = NMEA_Str2num(p1 + posx, &dx); // 得到卫星方位角
			else
				break;
			posx = NMEA_Comma_Pos(p1, 7 + j * 4);
			if (posx != 0XFF)
				gpsx->slmsg[slx].sn = NMEA_Str2num(p1 + posx, &dx); // 得到卫星信噪比
			else
				break;
			slx++;
		}
		p = p1 + 1; // 切换到下一个GPGSV信息
	}
}

// 分析BDGSV信息
// gpsx:nmea信息结构体
// buf:接收到的北斗数据缓冲区首地址
void NMEA_BDGSV_Analysis(nmea_msg *gpsx, u8 *buf)
{
	u8 *p, *p1, dx;
	u8 len, i, j, slx = 0;
	u8 posx;
	p = buf;
	p1 = (u8 *)strstr((const char *)p, "$BDGSV");
	len = p1[7] - '0';			  // 得到BDGSV的条数
	posx = NMEA_Comma_Pos(p1, 3); // 得到可见北斗卫星总数
	if (posx != 0XFF)
		gpsx->beidou_svnum = NMEA_Str2num(p1 + posx, &dx);
	for (i = 0; i < len; i++)
	{
		p1 = (u8 *)strstr((const char *)p, "$BDGSV");
		for (j = 0; j < 4; j++)
		{
			posx = NMEA_Comma_Pos(p1, 4 + j * 4);
			if (posx != 0XFF)
				gpsx->beidou_slmsg[slx].beidou_num = NMEA_Str2num(p1 + posx, &dx); // 得到卫星编号
			else
				break;
			posx = NMEA_Comma_Pos(p1, 5 + j * 4);
			if (posx != 0XFF)
				gpsx->beidou_slmsg[slx].beidou_eledeg = NMEA_Str2num(p1 + posx, &dx); // 得到卫星仰角
			else
				break;
			posx = NMEA_Comma_Pos(p1, 6 + j * 4);
			if (posx != 0XFF)
				gpsx->beidou_slmsg[slx].beidou_azideg = NMEA_Str2num(p1 + posx, &dx); // 得到卫星方位角
			else
				break;
			posx = NMEA_Comma_Pos(p1, 7 + j * 4);
			if (posx != 0XFF)
				gpsx->beidou_slmsg[slx].beidou_sn = NMEA_Str2num(p1 + posx, &dx); // 得到卫星信噪比
			else
				break;
			slx++;
		}
		p = p1 + 1; // 切换到下一个BDGSV信息
	}
}

// 分析GNGGA信息
// gpsx:nmea信息结构体
// buf:接收到的GPS/北斗数据缓冲区首地址
void NMEA_GNGGA_Analysis(nmea_msg *gpsx, u8 *buf)
{
	u8 *p1, dx;
	u8 posx;
	p1 = (u8 *)strstr((const char *)buf, "$GPGGA");
	posx = NMEA_Comma_Pos(p1, 6); // 得到GPS状态
	if (posx != 0XFF)
		gpsx->gpssta = NMEA_Str2num(p1 + posx, &dx);
	posx = NMEA_Comma_Pos(p1, 7); // 得到用于定位的卫星数
	if (posx != 0XFF)
		gpsx->posslnum = NMEA_Str2num(p1 + posx, &dx);
	posx = NMEA_Comma_Pos(p1, 9); // 得到海拔高度
	if (posx != 0XFF)
		gpsx->altitude = NMEA_Str2num(p1 + posx, &dx);
}

// 分析GNGSA信息
// gpsx:nmea信息结构体
// buf:接收到的GPS/北斗数据缓冲区首地址
void NMEA_GNGSA_Analysis(nmea_msg *gpsx, u8 *buf)
{
	u8 *p1, dx;
	u8 posx;
	u8 i;
	p1 = (u8 *)strstr((const char *)buf, "$GPGSA");
	posx = NMEA_Comma_Pos(p1, 2); // 得到定位类型
	if (posx != 0XFF)
		gpsx->fixmode = NMEA_Str2num(p1 + posx, &dx);
	for (i = 0; i < 12; i++) // 得到定位卫星编号
	{
		posx = NMEA_Comma_Pos(p1, 3 + i);
		if (posx != 0XFF)
			gpsx->possl[i] = NMEA_Str2num(p1 + posx, &dx);
		else
			break;
	}
	posx = NMEA_Comma_Pos(p1, 15); // 得到PDOP位置精度因子
	if (posx != 0XFF)
		gpsx->pdop = NMEA_Str2num(p1 + posx, &dx);
	posx = NMEA_Comma_Pos(p1, 16); // 得到HDOP位置精度因子
	if (posx != 0XFF)
		gpsx->hdop = NMEA_Str2num(p1 + posx, &dx);
	posx = NMEA_Comma_Pos(p1, 17); // 得到VDOP位置精度因子
	if (posx != 0XFF)
		gpsx->vdop = NMEA_Str2num(p1 + posx, &dx);
}

// 分析GNRMC信息
// gpsx:nmea信息结构体
// buf:接收到的GPS/北斗数据缓冲区首地址
void NMEA_GNRMC_Analysis(nmea_msg *gpsx, u8 *buf)
{
	u8 *p1, dx;
	u8 posx;
	u32 temp;
	p1 = (u8 *)strstr((const char *)buf, "$GPRMC"); // 通常 GPRMC 或 GNRMC 都可以
	if (p1 == NULL)
	{ // 如果没找到 $GPRMC, 尝试 $GNRMC
		p1 = (u8 *)strstr((const char *)buf, "$GNRMC");
		if (p1 == NULL)
			return; // 如果都没找到，则返回
	}

	posx = NMEA_Comma_Pos(p1, 2); // 得到RMC状态 (A/V)
	if (posx != 0XFF)
		gpsx->rmc_status = *(p1 + posx);
	else
		gpsx->rmc_status = 'V'; // 如果找不到，默认为无效

	posx = NMEA_Comma_Pos(p1, 1); // 得到UTC时间
	if (posx != 0XFF)
	{
		// ... (时间解析部分保持不变) ...
		temp = NMEA_Str2num(p1 + posx, &dx);
		if (dx > 2)
		{								  // NMEA_Str2num 可能会包含毫秒的小数点位数
			temp /= NMEA_Pow(10, dx - 2); // 保留到秒的百分位
		}
		//  temp = NMEA_Str2num(p1+posx,&dx)/NMEA_Pow(10,dx); // 原始逻辑，去掉ms
		gpsx->utc.hour = temp / 1000000; // hhmmss.ss (ss是小数部分，所以除以1000000)
		gpsx->utc.min = (temp / 10000) % 100;
		gpsx->utc.sec = (temp / 100) % 100; // 秒的整数部分
											// 如果需要更精确，可以保留小数部分，但nmea_utc_time结构体是u8
	}
	// --- 对时间解析进行修正，确保正确处理 hhmmss.ss ---
	posx = NMEA_Comma_Pos(p1, 1); // 得到UTC时间字符串起始位置
	if (posx != 0XFF)
	{
		char time_str[12] = {0}; // hhmmss.ss
		char *time_ptr = (char *)(p1 + posx);
		int k = 0;
		while (*time_ptr != ',' && *time_ptr != '*' && k < 11)
		{
			time_str[k++] = *time_ptr++;
		}
		if (k >= 6)
		{ // 确保至少有 hhmmss
			gpsx->utc.hour = (time_str[0] - '0') * 10 + (time_str[1] - '0');
			gpsx->utc.min = (time_str[2] - '0') * 10 + (time_str[3] - '0');
			gpsx->utc.sec = (time_str[4] - '0') * 10 + (time_str[5] - '0');
		}
	}

	posx = NMEA_Comma_Pos(p1, 3);			 // 得到纬度
	if (posx != 0XFF && *(p1 + posx) != ',') // 确保字段非空
	{
		temp = NMEA_Str2num(p1 + posx, &dx);
		// NMEA格式: dddmm.mmmm (度 和 分，分的小数)
		// 转换为: 度.dddddd (纯度，小数形式)
		// temp 此时是 dddmmllll (llll是分的小数部分，dx是llll的位数)
		u32 deg = temp / NMEA_Pow(10, dx + 2);		// 取出 ddd (度)
		u32 min_full = temp % NMEA_Pow(10, dx + 2); // 取出 mmllll (分和分的小数)
		// 将 mmllll (假设dx是分的小数位数) 转换为纯小数形式的分: (mm.llll)
		// gpsx->latitude 存储的是度的小数形式 * 100000
		// 所以 (deg + (min_full / (NMEA_Pow(10, dx) * 60.0))) * 100000
		gpsx->latitude = deg * 100000 + (min_full * 100000 / NMEA_Pow(10, dx)) / 60;
	}
	else
	{
		gpsx->latitude = 0;
	}

	posx = NMEA_Comma_Pos(p1, 4); // 南纬还是北纬
	if (posx != 0XFF)
		gpsx->nshemi = *(p1 + posx);
	else
		gpsx->nshemi = ' ';

	posx = NMEA_Comma_Pos(p1, 5);			 // 得到经度
	if (posx != 0XFF && *(p1 + posx) != ',') // 确保字段非空
	{
		temp = NMEA_Str2num(p1 + posx, &dx);
		u32 deg = temp / NMEA_Pow(10, dx + 2);
		u32 min_full = temp % NMEA_Pow(10, dx + 2);
		gpsx->longitude = deg * 100000 + (min_full * 100000 / NMEA_Pow(10, dx)) / 60;
	}
	else
	{
		gpsx->longitude = 0;
	}

	posx = NMEA_Comma_Pos(p1, 6); // 东经还是西经
	if (posx != 0XFF)
		gpsx->ewhemi = *(p1 + posx);
	else
		gpsx->ewhemi = ' ';

	posx = NMEA_Comma_Pos(p1, 9);			 // 得到UTC日期
	if (posx != 0XFF && *(p1 + posx) != ',') // 确保字段非空
	{
		temp = NMEA_Str2num(p1 + posx, &dx); // 得到UTC日期 ddmmyy
		gpsx->utc.date = temp / 10000;
		gpsx->utc.month = (temp / 100) % 100;
		gpsx->utc.year = 2000 + temp % 100;
	}
	else
	{ // 如果日期字段为空，给一个默认或无效值
		gpsx->utc.date = 0;
		gpsx->utc.month = 0;
		gpsx->utc.year = 0;
	}
}

// 分析GNVTG信息
// gpsx:nmea信息结构体
// buf:接收到的GPS/北斗数据缓冲区首地址
void NMEA_GNVTG_Analysis(nmea_msg *gpsx, u8 *buf)
{
	u8 *p1, dx;
	u8 posx;
	p1 = (u8 *)strstr((const char *)buf, "$GNVTG");
	posx = NMEA_Comma_Pos(p1, 7); // 得到地面速率
	if (posx != 0XFF)
	{
		gpsx->speed = NMEA_Str2num(p1 + posx, &dx);
		if (dx < 3)
			gpsx->speed *= NMEA_Pow(10, 3 - dx); // 确保扩大1000倍
	}
}

// 提取NMEA-0183信息
// gpsx:nmea信息结构体
// buf:接收到的GPS/北斗数据缓冲区首地址
void GPS_Analysis(nmea_msg *gpsx, u8 *buf)
{
	// NMEA_GPGSV_Analysis(gpsx, buf);	//GPGSV解析
	//	NMEA_BDGSV_Analysis(gpsx, buf);	//BDGSV解析
	// NMEA_GNGGA_Analysis(gpsx, buf);	//GPGGA解析
	// NMEA_GNGSA_Analysis(gpsx, buf);	//GPGSA解析
	NMEA_GNRMC_Analysis(gpsx, buf); // GPRMC解析
									//	NMEA_GNVTG_Analysis(gpsx, buf);	//GNVTG解析
}

// 显示GPS定位信息
void Gps_Msg_Show()
{
	// 函数功能：显示gpsx结构体中的信息，包括经纬度、定位状态和存储的时间
	// 注意：时间部分直接显示 gpsx.utc 中的值，不在此函数内做时区转换
	float latitude_f, longitude_f;
	char status_str[20];

	// 转换经纬度以便显示
	// 结构体中存储的是 度 * 100000 (此计算逻辑与原版一致)
	latitude_f = (float)gpsx.latitude / 100000.0;
	longitude_f = (float)gpsx.longitude / 100000.0;

	// 根据RMC状态字符判断定位状态
	if (gpsx.rmc_status == 'A')
	{
		sprintf(status_str, "Valid (A)"); // 定位有效
	}
	else if (gpsx.rmc_status == 'V')
	{
		sprintf(status_str, "Invalid (V)"); // 定位无效
	}
	else
	{
		sprintf(status_str, "Unknown"); // 未知状态
	}

	// 使用 xil_printf 打印信息
	xil_printf("--- GPS Information ---\r\n");							 // 标题 (printf for baremetal)
	xil_printf(" Latitude     : %.5f %c\r\n", latitude_f, gpsx.nshemi);	 // 纬度
	xil_printf(" Longitude    : %.5f %c\r\n", longitude_f, gpsx.ewhemi); // 经度
	xil_printf(" Fix Status   : %s\r\n", status_str);					 // 定位状态

	// 显示存储在 gpsx.utc 中的时间，不进行转换
	// 这里的年份、月份、日期、小时、分钟、秒钟直接来自 gpsx.utc 结构体
	if (gpsx.utc.year == 0) // 通常年份为0表示GPS日期时间数据无效或未同步
	{
		xil_printf(" Stored Time  : Invalid or Not Synced (Year is 0)\r\n"); // 时间无效提示
	}
	else
	{
		xil_printf(" Stored Time  : %04u-%02u-%02u %02u:%02u:%02u (Timezone as in struct)\r\n", // 直接打印存储的时间
				   gpsx.utc.year, gpsx.utc.month, gpsx.utc.date,
				   gpsx.utc.hour, gpsx.utc.min, gpsx.utc.sec);
	}
	// 可以选择性地显示其他gpsx成员，如卫星数量、PDOP等
	// xil_printf(" Satellites   : %d (GPS), %d (Beidou)\r\n", gpsx.svnum, gpsx.beidou_svnum); // 可见卫星数 (如果解析了)
	// xil_printf(" Altitude     : %.1f m\r\n", (float)gpsx.altitude / 10.0); // 海拔高度 (如果解析了)
	// xil_printf(" Speed        : %.3f km/h\r\n", (float)gpsx.speed / 1000.0); // 地面速率 (如果解析了)
	xil_printf("--- End GPS Information ---\r\n\r\n"); // 结束符
}

// 辅助函数：判断是否是闰年
static int is_leap_year(int year)
{
	// 内部实现：判断闰年
	// 如果年份能被4整除但不能被100整除，或者能被400整除，则为闰年
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// 辅助函数：获取某年某月的天数
static int days_in_month(int year, int month)
{
	// 内部实现：根据年月返回天数
	if (month < 1 || month > 12)
		return 0;														  // 无效月份，返回0或者进行错误处理
	int days_arr[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // 各个月份的天数，0索引不用
	if (is_leap_year(year) && month == 2)								  // 如果是闰年的2月
	{
		return 29;
	}
	return days_arr[month];
}

/**
 * @brief 将nmea_msg结构体中的UTC时间转换为北京时间 (UTC+8)
 * @param gps_data 指向 nmea_msg 结构体的指针，函数将直接修改此结构体中的 utc 成员
 */
void GPS_ConvertUTCToBeijing(nmea_msg *gps_data)
{
	// 函数功能：将gps_data中的UTC时间转换为北京时间并更新结构体
	// 输入：指向nmea_msg结构体的指针
	// 输出：无 (直接修改传入的结构体)
	if (gps_data == NULL || gps_data->utc.year == 0)
	{
		// 如果指针为空或GPS时间无效，则不进行转换
		return;
	}

	// 从结构体中获取UTC时间成员
	int year = gps_data->utc.year;
	int month = gps_data->utc.month;
	int day = gps_data->utc.date;
	int hour = gps_data->utc.hour;
	// min 和 sec 不需要改变，所以直接使用 gps_data->utc.min 和 gps_data->utc.sec

	// 应用 +8 小时时差
	hour += 8;

	// 处理小时进位
	if (hour >= 24)
	{
		hour -= 24; // 小时数调整
		day += 1;	// 日期进位

		// 处理日期进位（月份和年份）
		int current_month_days = days_in_month(year, month);
		if (current_month_days == 0)
		{ // 无效月份或年份导致无法获取天数
			// 可以选择报错或保持原样，这里简单返回避免后续错误
			return;
		}

		if (day > current_month_days)
		{
			day = 1;	// 日期重置为1号
			month += 1; // 月份进位

			if (month > 12)
			{
				month = 1; // 月份重置为1月
				year += 1; // 年份进位
			}
		}
	}

	// 将转换后的北京时间写回结构体
	gps_data->utc.year = (u16)year;
	gps_data->utc.month = (u8)month;
	gps_data->utc.date = (u8)day;
	gps_data->utc.hour = (u8)hour;
	// gps_data->utc.min 和 gps_data->utc.sec 保持不变
}
