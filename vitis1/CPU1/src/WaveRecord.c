#include "WaveRecord.h"
#include "ADDA.h" // 获取 FS_RATE, CHN_NUM, AD_Correct, setACS 等
#include "xil_cache.h"
#include "xil_printf.h"
#include "Communications_Protocol.h" // 获取 In_CurrTime, read_current_time
#include <string.h>
#include <stdio.h>

WaveRecord_Ctrl_t g_WaveRecordCtrl;

void WaveRecord_Init(void)
{
    memset(&g_WaveRecordCtrl, 0, sizeof(WaveRecord_Ctrl_t));
    g_WaveRecordCtrl.isRecording = false;
}

void WaveRecord_Start(u32 duration_ms)
{
    g_WaveRecordCtrl.sampleSequence = 0;
    g_WaveRecordCtrl.recordedBytes = 0;
    g_WaveRecordCtrl.writeAddrOffset = 0;

    // 记录启动时间用于 CFG 生成
    In_CurrTime curr;
    read_current_time(&curr);
    sprintf(g_WaveRecordCtrl.startTimeStr, "%04u/%02u/%02u,%02u:%02u:%02u.%06u",
            curr.curr_year, curr.curr_month, curr.curr_day,
            curr.curr_hour, curr.curr_minute, curr.curr_second,
            (unsigned int)curr.curr_subsec); // 假设subsec精度适配

    // 计算最大字节数 (RecMS <= 0 表示录直到满)
    if (duration_ms <= 0)
    {
        g_WaveRecordCtrl.maxRecordBytes = REC_DAT_MAX_SIZE;
    }
    else
    {
        u64 bytes = (u64)duration_ms * FS_RATE / 1000 * COMTRADE_FRAME_SIZE;
        // 必须限制在预留空间内，防止覆盖 CFG 区域
        if (bytes > REC_DAT_MAX_SIZE)
            bytes = REC_DAT_MAX_SIZE;
        g_WaveRecordCtrl.maxRecordBytes = (u32)bytes;
    }

    g_WaveRecordCtrl.isRecording = true;
    xil_printf("CPU1: WaveRecord Started. Target: %u bytes\r\n", g_WaveRecordCtrl.maxRecordBytes);
}

void WaveRecord_Stop(void)
{
    if (g_WaveRecordCtrl.isRecording)
    {
        g_WaveRecordCtrl.isRecording = false;

        // 录波结束时立即生成 CFG 文件
        Generate_Comtrade_CFG();

        xil_printf("CPU1: WaveRecord Stopped. Total: %u bytes. CFG generated at 0x%X\r\n", g_WaveRecordCtrl.recordedBytes, REC_CFG_ADDR);
    }
}

/**
 * @brief 处理一块数据 (500ms) 进行录波存储
 */
void WaveRecord_Process(u16 *pRawData, int points_count)
{
    if (!g_WaveRecordCtrl.isRecording)
        return;

    if (g_WaveRecordCtrl.recordedBytes >= g_WaveRecordCtrl.maxRecordBytes)
    {
        WaveRecord_Stop();
        return;
    }

    u32 *pDest = (u32 *)(UINTPTR)(REC_SHARE_BASE + g_WaveRecordCtrl.writeAddrOffset);
    u32 current_block_size = 0;

    for (int i = 0; i < points_count; i++)
    {
        // 1. 序号 (4B)
        *pDest++ = g_WaveRecordCtrl.sampleSequence; // 先写入，稍后递增

        // 2. 时间 (4B) - 【优化】使用 u64 整数运算代替 double
        // 避免在 25600 次循环里做浮点除法
        u64 time_us_64 = ((u64)g_WaveRecordCtrl.sampleSequence * 1000000) / FS_RATE;
        *pDest++ = (u32)time_us_64;

        // 序号递增放这里
        g_WaveRecordCtrl.sampleSequence++;

        // 3. 8通道模拟量 (16B)
        u16 *pSamp = &pRawData[i * CHN_NUM];
        *pDest++ = (u32)pSamp[0] | ((u32)pSamp[1] << 16);
        *pDest++ = (u32)pSamp[2] | ((u32)pSamp[3] << 16);
        *pDest++ = (u32)pSamp[4] | ((u32)pSamp[5] << 16);
        *pDest++ = (u32)pSamp[6] | ((u32)pSamp[7] << 16);

        current_block_size += COMTRADE_FRAME_SIZE; // 这里必须是 24！
    }

    // 循环结束后统一更新全局偏移
    g_WaveRecordCtrl.writeAddrOffset += current_block_size;
    g_WaveRecordCtrl.recordedBytes += current_block_size;

    // 刷 Cache
    Xil_DCacheFlushRange(REC_SHARE_BASE + g_WaveRecordCtrl.writeAddrOffset - current_block_size,
                         current_block_size);

    if (g_WaveRecordCtrl.recordedBytes >= g_WaveRecordCtrl.maxRecordBytes)
    {
        WaveRecord_Stop();
    }
}

/**
 * @brief 生成 Comtrade 配置文件 (.CFG) 并写入共享内存末尾
 */
void Generate_Comtrade_CFG(void)
{
    char *cfgBuf = (char *)(UINTPTR)REC_CFG_ADDR;
    int len = 0;

    // 1. Station Name, Device ID, RevYear (1999)
    len += sprintf(cfgBuf + len, "Zynq_Device,Unit01,1999\n");

    // 2. Channels: Total, Analog, Digital (8A, 0D)
    len += sprintf(cfgBuf + len, "8,8A,0D\n");

    // 3. Analog Channel Definitions
    // 物理顺序: IA(0), UA(1), IB(2), UB(3), IC(4), UC(5), IX(6), UX(7)
    // 对应 AD_Correct 索引:
    //   IA(0)->4, UA(1)->0, IB(2)->5, UB(3)->1, IC(4)->6, UC(5)->2, IX(6)->7, UX(7)->3
    //   规律: 偶数k(电流) -> idx=4+k/2; 奇数k(电压) -> idx=0+k/2
    const char *chnNames[] = {"IA", "UA", "IB", "UB", "IC", "UC", "IX", "UX"};
    const char *units[] = {"A", "V", "A", "V", "A", "V", "A", "V"};

    for (int k = 0; k < 8; k++)
    {
        int ad_correct_idx;
        double range_val;
        int logical_chn = k / 2; // 0,0, 1,1, 2,2, 3,3

        if (k % 2 == 0)
        { // Current Channel (IA, IB...)
            ad_correct_idx = 4 + logical_chn;
            range_val = setACS.Vals[logical_chn].IR;
        }
        else
        { // Voltage Channel (UA, UB...)
            ad_correct_idx = 0 + logical_chn;
            range_val = setACS.Vals[logical_chn].UR;
        }

        // 获取当前量程下的校准系数索引
        int range_idx;
        if (k % 2 == 0)
            range_idx = get_current_index_by_value(range_val);
        else
            range_idx = get_voltage_index_by_value(range_val);

        // 计算变换因子 a (Multiplier)
        // 公式: Physical = Raw * a + b
        // 已知: Raw_Max ~32768, Physical_Max = range_val
        // AD_Correct 存储的是满量程对应的 ADC 码值(约20000-30000)
        // 所以: a = range_val / ADConst_Correct[idx][range_idx]
        // 或者是 AD_Correct 数组 (如果已校准)
        double corrector = AD_Correct[ad_correct_idx][range_idx];
        if (corrector < 1.0)
            corrector = 20000.0; // 防止除零保护
        // 【关键修改】
        // 原始公式: a = range_val / corrector;
        // 问题: range_val 是 RMS 值，而 COMTRADE 需要还原瞬时峰值。
        // 修正: a = (range_val * 1.41421356) / corrector;
        // 这样 Raw_Max 就会映射到 Peak_Voltage (即 RMS * 1.414)
        double a = (range_val * 1.41421356) / corrector;
        double b = 0.0; // 偏移量通常为0

        // 格式: n,id,ph,cc,min,max,a,b,skew,min,max,primary,secondary,PS
        len += sprintf(cfgBuf + len, "%d,%s,,,%s,%f,%f,0.0,-32768,32767,1,1,P\n",
                       k + 1,       // Index
                       chnNames[k], // ID
                       units[k],    // Units (V/A)
                       a,           // Multiplier
                       b            // Offset
        );
    }

    // 4. Frequency
    len += sprintf(cfgBuf + len, "50.0\n");

    // 5. Sample Rates (1 rate)
    len += sprintf(cfgBuf + len, "1\n");
    len += sprintf(cfgBuf + len, "%d,%lu\n", FS_RATE, g_WaveRecordCtrl.sampleSequence);

    // 6. Dates (Start Time, Trigger Time)
    // 使用 Start 时记录的时间
    len += sprintf(cfgBuf + len, "%s\n", g_WaveRecordCtrl.startTimeStr);
    len += sprintf(cfgBuf + len, "%s\n", g_WaveRecordCtrl.startTimeStr);

    // 7. File Type & Orientation
    len += sprintf(cfgBuf + len, "BINARY\n");
    len += sprintf(cfgBuf + len, "1\n");

    // 8. 刷 Cache
    Xil_DCacheFlushRange((UINTPTR)cfgBuf, len + 1); // +1 包含结束符
}