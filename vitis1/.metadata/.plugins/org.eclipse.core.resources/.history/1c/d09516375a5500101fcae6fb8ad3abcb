#include "soft_timer.h"
#include "xil_printf.h"
#include "sleep.h" // 包含 usleep (如果是裸机环境) 或 <unistd.h> (如果是Linux用户空间)

// 如果是 PetaLinux 用户空间应用, 使用 #include <unistd.h> for usleep
// 如果是裸机 (standalone BSP), Xilinx SDK/Vitis 提供的 sleep.h 包含 usleep (通常是忙等待)

/**
 * @brief 将一个0-99的整数转换为BCD码 (存储在一个字节中)
 */
uint8_t int_to_bcd_byte(uint8_t value)
{
    if (value > 99)
        value = 99;
    return ((value / 10) << 4) | (value % 10);
}

/**
 * @brief 将一个字节的BCD码转换为整数
 */
uint8_t bcd_to_int_byte(uint8_t bcd_value)
{
    return (((bcd_value >> 4) & 0x0F) * 10) + (bcd_value & 0x0F);
}

/**
 * @brief 计算给定日期是一年中的第几天
 */
int calculate_year_day(int month, int day, int year)
{
    int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12 || day < 1)
        return 0;
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        days_in_month[2] = 29;
    }
    if (day > days_in_month[month])
        return 0;
    int year_day_val = 0;
    for (int i = 1; i < month; i++)
    {
        year_day_val += days_in_month[i];
    }
    year_day_val += day;
    return year_day_val;
}

/**
 * @brief 计算给定日期的星期几 (ISO 8601标准: 1 = 周一, ..., 7 = 周日)
 */
int calculate_weekday_iso(int y, int m, int d)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    int day_of_week = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; // 0 for Sunday
    if (day_of_week == 0)
        day_of_week = 7; // Convert Sunday from 0 to 7
    return day_of_week;
}

/**
 * @brief 写入时间数据到软时钟IP核
 */
void write_soft_timer(Out_RealTime *time_data)
{
    uint32_t reg_val_0, reg_val_1, reg_val_2;
    int full_year = time_data->year; // 假设传入的已经是四位年份

    int year_day_val = calculate_year_day(time_data->month, time_data->day, full_year);
    if (year_day_val == 0)
    {
        // xil_printf("SoftTimer Write Error: Invalid date for year_day (%d-%d-%d)\r\n", full_year, time_data->month, time_data->day);
        return;
    }

    uint8_t bcd_yy = int_to_bcd_byte(full_year % 100);
    uint8_t yd_d0 = year_day_val % 10;
    uint8_t yd_d1 = (year_day_val / 10) % 10;
    uint8_t yd_d2 = (year_day_val / 100) % 10;
    uint16_t final_bcd_year_day = (uint16_t)(yd_d2 << 8) | (yd_d1 << 4) | yd_d0;

    reg_val_0 = (uint32_t)(bcd_yy << 10) | (final_bcd_year_day & 0x03FF);

    reg_val_1 = (uint32_t)(int_to_bcd_byte(time_data->month) << 13) |
                (uint32_t)(int_to_bcd_byte(time_data->day) << 7) |
                (time_data->week & 0x7F);

    reg_val_2 = (uint32_t)(int_to_bcd_byte(time_data->hour) << 14) |
                (uint32_t)(int_to_bcd_byte(time_data->min) << 7) |
                (uint32_t)(int_to_bcd_byte(time_data->sec));

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG0, reg_val_0);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG1, reg_val_1);
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG2, reg_val_2);

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG5, (time_data->bm_encode_en ? 1 : 0));
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG6, (time_data->bm_decode_en ? 1 : 0));
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG7, (time_data->pps_clr_en ? 1 : 0));

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG3, 1); // wr_date = 1
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG4, 1); // wr_time = 1

    usleep(10); // 稍作延时确保IP核捕获，1us可能不够，10-100us更稳妥

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG3, 0); // wr_date = 0
    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG4, 0); // wr_time = 0
}

/**
 * @brief 读取软时钟IP核的当前时间
 */
void read_current_time(In_CurrTime *Curr_Time)
{
    uint32_t reg_data3, reg_data4, reg_data5, reg_data6, reg_data7;

    reg_data3 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG3);
    reg_data4 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG4);
    reg_data5 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG5);
    reg_data6 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG6);
    reg_data7 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG7);

    Curr_Time->curr_year = 2000 + bcd_to_int_byte((reg_data3 >> 10) & 0xFF);
    uint16_t bcd_yd_read = reg_data3 & 0x03FF;
    uint8_t yd_r_d0 = bcd_yd_read & 0xF;
    uint8_t yd_r_d1 = (bcd_yd_read >> 4) & 0xF;
    uint8_t yd_r_d2 = (bcd_yd_read >> 8) & 0x3;
    Curr_Time->curr_yearday = (uint16_t)yd_r_d2 * 100 + (uint16_t)yd_r_d1 * 10 + yd_r_d0;

    Curr_Time->curr_month = bcd_to_int_byte((reg_data4 >> 13) & 0x1F);
    Curr_Time->curr_day = bcd_to_int_byte((reg_data4 >> 7) & 0x3F);
    Curr_Time->curr_week = reg_data4 & 0x7F;

    Curr_Time->curr_hour = bcd_to_int_byte((reg_data5 >> 14) & 0x3F);
    Curr_Time->curr_minute = bcd_to_int_byte((reg_data5 >> 7) & 0x7F);
    Curr_Time->curr_second = bcd_to_int_byte(reg_data5 & 0x7F);

    Curr_Time->curr_daysec = reg_data6 & 0x1FFFF;  // Assuming binary
    Curr_Time->curr_subsec = reg_data7 & 0xFFFFFF; // Assuming binary
}

/**
 * @brief 读取B码时间
 */
void read_bm_time(In_BmTime *Bm_Time)
{
    uint32_t reg0 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG0);
    uint32_t reg1 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG1);
    uint32_t reg2 = Xil_In32(SoftTimer_BASEADDR + SoftTimer_REG2);

    Bm_Time->bm_year = 2000 + bcd_to_int_byte((reg0 >> 10) & 0xFF);
    uint16_t bcd_bm_yd = reg0 & 0x03FF;
    uint8_t bm_yd_r_d0 = bcd_bm_yd & 0xF;
    uint8_t bm_yd_r_d1 = (bcd_bm_yd >> 4) & 0xF;
    uint8_t bm_yd_r_d2 = (bcd_bm_yd >> 8) & 0x3;
    Bm_Time->bm_yearday = (uint16_t)bm_yd_r_d2 * 100 + (uint16_t)bm_yd_r_d1 * 10 + bm_yd_r_d0;

    Bm_Time->bm_hour = bcd_to_int_byte((reg1 >> 14) & 0x3F);
    Bm_Time->bm_minute = bcd_to_int_byte((reg1 >> 7) & 0x7F);
    Bm_Time->bm_second = bcd_to_int_byte(reg1 & 0x7F);
    Bm_Time->bm_daysec = reg2 & 0x1FFFF; // Assuming binary
}