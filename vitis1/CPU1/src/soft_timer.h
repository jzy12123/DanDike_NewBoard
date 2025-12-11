#ifndef SOFT_TIMER_H_
#define SOFT_TIMER_H_

#include <stdint.h>
#include <stdbool.h>
#include "xil_io.h"      // 包含 Xil_Out32, Xil_In32
#include "xparameters.h" // 包含 XPAR_SOFT_TIMER_0_S_AXI_BASEADDR

// 软时钟IP核寄存器地址定义
#define SoftTimer_BASEADDR XPAR_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_BASEADDR
#define SoftTimer_REG0 0  // 写: 年份BCD[17:10], 年日BCD[9:0]  读: B码年日[9:0], B码年[17:10]
#define SoftTimer_REG1 4  // 写: 月BCD[17:13], 日BCD[12:7], 星期独热码[6:0] 读: B码秒[6:0], B码分[13:7], B码时[19:14]
#define SoftTimer_REG2 8  // 写: 时BCD[19:14], 分BCD[13:7], 秒BCD[6:0]  读: B码日秒[16:0]
#define SoftTimer_REG3 12 // 写: wr_date[0]  读: 当前年BCD[17:10], 当前年日BCD[9:0]
#define SoftTimer_REG4 16 // 写: wr_time[0]  读: 当前星期独热码[6:0], 当前月日BCD[12:7], 当前月BCD[17:13]
#define SoftTimer_REG5 20 // 写: bm_encode_en[0] 读: 当前秒BCD[6:0], 当前分BCD[13:7], 当前时BCD[19:14]
#define SoftTimer_REG6 24 // 写: bm_decode_en[0] 读: 当前日秒BCD[16:0] (或二进制)
#define SoftTimer_REG7 28 // 写: pps_clr_en[0]   读: 亚秒值[23:0] (通常二进制)
#define SoftTimer_REG15 60 // 定时器控制寄存器
// REG15 位定义
#define STIMER_ALARM_EN_MASK 0x80000000   // [31] 使能
#define STIMER_ALARM_HOUR_MASK 0x3F000000 // [29:24] 时 (BCD)
#define STIMER_ALARM_HOUR_SHIFT 24
#define STIMER_ALARM_MIN_MASK 0x007F0000 // [22:16] 分 (BCD)
#define STIMER_ALARM_MIN_SHIFT 16
#define STIMER_ALARM_SEC_MASK 0x00007F00 // [14:8] 秒 (BCD)
#define STIMER_ALARM_SEC_SHIFT 8
#define STIMER_RDSERIAL_EN_MASK 0x00000001 // [0] 读故障使能 (必须保留)

// 结构体定义：用于写入软时钟的时间数据
typedef struct
{
    uint16_t year;  // 年份         slv_reg0:[17:10] BCD码
    uint16_t month; // 月份         slv_reg1:[17:13] BCD码
    uint16_t day;   // 月日期         slv_reg1:[12:7]BCD码
    uint16_t week;  // 星期         slv_reg1:[6:0]独热码（0~6）周一至周日

    uint16_t year_day; // 年内第几天    slv_reg0:[9:0]  BCD码
    uint16_t hour;     // 小时         slv_reg2:[19:14] BCD码
    uint16_t min;      // 分钟         slv_reg2:[13:7]  BCD码
    uint16_t sec;      // 秒           slv_reg2:[6:0]   BCD码


    bool bm_encode_en; // 容许b码输出  slv_reg5：[0]
    bool bm_decode_en; // 容许b码输入   slv_reg6：[0]
    bool pps_clr_en;   // 亚秒PPS清零   slv_reg7：[0]
} Out_RealTime;

typedef struct
{
    uint16_t curr_yearday; // 当前年天数   slv_reg3[9:0]   BCD码
    uint16_t curr_year;    // 当前年       slv_reg3[17:10] BCD码
    uint16_t curr_week;    // 当前星期      slv_reg4[6:0] 独热码（0~6）周一至周日
    uint16_t curr_day;     // 当前月日期   slv_reg4[12:7] BCD码
    uint16_t curr_month;   // 当前月份     slv_reg4[17:13]BCD码
    uint16_t curr_second;  // 当前秒       slv_reg5[6:0]  BCD码
    uint16_t curr_minute;  // 当前分       slv_reg5[13:7] BCD码
    uint16_t curr_hour;    // 当前时       slv_reg5[19:14]BCD码
    uint32_t curr_daysec;  // 当前日秒     slv_reg6[16:0] BCD码
    uint32_t curr_subsec;  // 亚秒         slv_reg7[23:0] BCD码
} In_CurrTime;

// B码时间结构体 (如果需要读取)
typedef struct
{
    uint16_t bm_yearday;
    uint16_t bm_year;
    uint16_t bm_second;
    uint16_t bm_minute;
    uint16_t bm_hour;
    uint32_t bm_daysec;
} In_BmTime;

// [新增] 声明全局影子寄存器 (在 StateSequence.c 中定义)
// 所有操作 REG15 的代码都必须维护这个变量
extern uint32_t g_SoftTimer_Reg15_Shadow;

// 函数声明
uint8_t int_to_bcd_byte(uint8_t value);
uint8_t bcd_to_int_byte(uint8_t bcd_value);
int calculate_year_day(int month, int day, int year);
int calculate_weekday_iso(int y, int m, int d); // 1=Monday, ..., 7=Sunday

void write_soft_timer(Out_RealTime *time_data);
void read_current_time(In_CurrTime *Curr_Time);
void read_bm_time(In_BmTime *Bm_Time);
double time_diff_seconds(const In_CurrTime *end_time, const In_CurrTime *start_time);
#endif /* SOFT_TIMER_H_ */
