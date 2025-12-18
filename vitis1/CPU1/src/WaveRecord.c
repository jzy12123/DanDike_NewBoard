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

    g_WaveRecordCtrl.startTimeUs = Get_Current_Time_US(); //  记录精确的启动时刻
    g_WaveRecordCtrl.isFirstBlock = true; // 标记这是第一块
    g_WaveRecordCtrl.isRecording = true;  // 开启录波开关
    xil_printf("CPU1: SetTaskWaveRecord Started. Target: %u bytes\r\n", g_WaveRecordCtrl.maxRecordBytes);
}

void WaveRecord_Stop(void)
{
    if (g_WaveRecordCtrl.isRecording)
    {
        g_WaveRecordCtrl.isRecording = false;

        // 录波结束时立即生成 CFG 文件
        Generate_Comtrade_CFG();

        xil_printf("CPU1: SetTaskWaveRecord Stopped. Total: %u bytes. CFG generated at 0x%X\r\n", g_WaveRecordCtrl.recordedBytes, REC_CFG_ADDR);
    }
}

/**
 * @brief 处理一块数据 (500ms) 进行录波存储
 * 包含首块数据的“无效历史数据”剔除功能
 */
void WaveRecord_Process(u16 *pRawData, int points_count)
{
    // 1. 基础检查
    if (!g_WaveRecordCtrl.isRecording)
        return;

    // 2. 指针与计数器准备
    // 默认情况下，处理整个 Buffer
    u16 *pValidData = pRawData;      // 有效数据的起始指针
    int valid_points = points_count; // 本次需要处理的有效点数

    // =================================================================
    // 【核心逻辑】如果是启动后的第一块数据，计算并剔除启动前的无效波形
    // =================================================================
    if (g_WaveRecordCtrl.isFirstBlock)
    {
        // 计算有效时长：(当前中断时刻 - 点击启动时刻)
        // 假设 Block 刚传完触发中断，那么从“启动”到“中断”这段时间才是有效录波
        long long time_diff_us = (long long)(g_LastDmaIrqTime_us - g_WaveRecordCtrl.startTimeUs);

        // 异常保护：防止时间倒挂
        if (time_diff_us < 0)
            time_diff_us = 0;

        // 计算本块数据的总时长 (us)
        u64 block_duration_us = (u64)points_count * 1000000 / FS_RATE;

        // 如果有效时间超过了块长（说明启动指令是很久以前发的），则保留全块
        if (time_diff_us > block_duration_us)
        {
            time_diff_us = block_duration_us;
        }

        // 计算需要保留的点数 = 有效时长 * 采样率
        int keep_samples = (int)((double)time_diff_us * FS_RATE / 1000000.0);

        // 计算需要剔除的点数 = 总点数 - 保留点数
        int discard_samples = points_count - keep_samples;

        // 边界修正
        if (discard_samples < 0)
            discard_samples = 0;
        if (discard_samples >= points_count)
            discard_samples = points_count;

        // 应用偏移：剔除头部数据
        if (discard_samples > 0)
        {
            // 指针后移：注意 pRawData 是交错存储，偏移量 = 点数 * 通道数
            pValidData += (discard_samples * CHN_NUM);
            valid_points -= discard_samples;

            // 调试打印 (可选)
            xil_printf("CPU1: First Block Trimmed. Discarded: %d, Kept: %d\r\n", discard_samples, valid_points);
        }

        // 清除标志，后续的数据块将全部记录
        g_WaveRecordCtrl.isFirstBlock = false;
    }

    // =================================================================
    // 常规存储逻辑 (使用处理过的 valid_points 和 pValidData)
    // =================================================================

    // 再次检查容量（防止刚才计算完刚好满了）
    if (g_WaveRecordCtrl.recordedBytes >= g_WaveRecordCtrl.maxRecordBytes)
    {
        WaveRecord_Stop();
        return;
    }

    // 获取 DDR 写入地址
    u32 *pDest = (u32 *)(UINTPTR)(REC_SHARE_BASE + g_WaveRecordCtrl.writeAddrOffset);
    u32 current_block_size = 0;

    for (int i = 0; i < valid_points; i++)
    {
        // 1. 序号 (4B)
        *pDest++ = g_WaveRecordCtrl.sampleSequence;

        // 2. 时间 (4B) - 使用 u64 优化除法
        // Time = (Seq * 1000000) / FS
        u64 time_us_64 = ((u64)g_WaveRecordCtrl.sampleSequence * 1000000) / FS_RATE;
        *pDest++ = (u32)time_us_64;

        // 序号递增
        g_WaveRecordCtrl.sampleSequence++;

        // 3. 8通道模拟量 (16B)
        // 使用偏移后的 pValidData 指针
        u16 *pSamp = &pValidData[i * CHN_NUM];

        // 32位合并写入优化
        *pDest++ = (u32)pSamp[0] | ((u32)pSamp[1] << 16);
        *pDest++ = (u32)pSamp[2] | ((u32)pSamp[3] << 16);
        *pDest++ = (u32)pSamp[4] | ((u32)pSamp[5] << 16);
        *pDest++ = (u32)pSamp[6] | ((u32)pSamp[7] << 16);

        // 累加帧大小 (固定24字节)
        current_block_size += COMTRADE_FRAME_SIZE;

        // 循环内检查：防止单次循环写超限 (针对内存边缘情况)
        if (g_WaveRecordCtrl.writeAddrOffset + current_block_size >= REC_DAT_MAX_SIZE ||
            g_WaveRecordCtrl.recordedBytes + current_block_size >= g_WaveRecordCtrl.maxRecordBytes)
        {
            break;
        }
    }

    // 循环结束后统一更新全局状态
    g_WaveRecordCtrl.writeAddrOffset += current_block_size;
    g_WaveRecordCtrl.recordedBytes += current_block_size;

    // 刷 Cache (确保数据写入物理 DDR)
    Xil_DCacheFlushRange(REC_SHARE_BASE + g_WaveRecordCtrl.writeAddrOffset - current_block_size,
                         current_block_size);

    // 检查是否录满
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