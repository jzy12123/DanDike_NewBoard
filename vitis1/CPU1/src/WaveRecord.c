#include "WaveRecord.h"
#include "ADDA.h" // 获取 FS_RATE, CHN_NUM, AD_Correct, setACS 等
#include "xil_cache.h"
#include "xil_printf.h"
#include <string.h>
#include <stdio.h>

WaveRecord_Ctrl_t g_WaveRecordCtrl;

void WaveRecord_Init(void)
{
    memset(&g_WaveRecordCtrl, 0, sizeof(WaveRecord_Ctrl_t));
    g_WaveRecordCtrl.isRecording = false;
    g_WaveRecordCtrl.isPendingSave = false;
}

/**
 * @brief 上报录波启动 (3.2.1 WaveRecordStarted)
 */
static void Report_WaveRecordStart(void)
{
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "Report");
    cJSON_AddStringToObject(report, "FunCode", "WaveRecordStarted");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "RecFrom", g_WaveRecordCtrl.recFrom);
    cJSON_AddNumberToObject(data, "RecSamp", FS_RATE); // 51200

    // RecDuration: 设定时长，如果无限长用 -1
    int duration = (g_WaveRecordCtrl.targetDurationMs <= 0) ? -1 : g_WaveRecordCtrl.targetDurationMs;
    cJSON_AddNumberToObject(data, "RecDuration", duration);

    cJSON_AddItemToObject(report, "Data", data);

    // 发送消息
    char *string = cJSON_PrintUnformatted(report);
    if (string)
    {
        size_t len = strlen(string);
        char *finalStr = malloc(len + 3);
        if (finalStr)
        {
            snprintf(finalStr, len + 3, "|%s|", string);
            MsgQue_write(finalStr, strlen(finalStr));
            free(finalStr);
        }
        free(string);
    }
    cJSON_Delete(report);
}

/**
 * @brief 上报文件生成 (3.2.2 WaveRecordFileCreated)
 */
static void Report_WaveRecordFileCreated(void)
{
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "Report");
    cJSON_AddStringToObject(report, "FunCode", "WaveRecordFileCreated");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "RecFrom", g_WaveRecordCtrl.recFrom);
    cJSON_AddNumberToObject(data, "RecSamp", FS_RATE);

    // 实际录波时长 = 点数 * 1000 / FS
    // 或者直接用 recordedBytes 计算
    double actual_duration = (double)g_WaveRecordCtrl.sampleSequence * 1000.0 / FS_RATE;
    cJSON_AddNumberToObject(data, "RecDuration", (int)actual_duration);

    // 生成文件名 YYYYMMDD_HHNNSS_ZZZ
    char fileNameBase[64];
    In_CurrTime *t = &g_WaveRecordCtrl.startTimestamp;
    // subsec 是 0.1us 单位 (10MHz计数)，转毫秒需要 / 10000
    int ms = (int)(t->curr_subsec / 10000);

    sprintf(fileNameBase, "%04u%02u%02u_%02u%02u%02u_%03u",
            t->curr_year, t->curr_month, t->curr_day,
            t->curr_hour, t->curr_minute, t->curr_second, ms);

    cJSON *files = cJSON_CreateArray();
    char nameBuf[80];
    sprintf(nameBuf, "%s.cfg", fileNameBase);
    cJSON_AddItemToArray(files, cJSON_CreateString(nameBuf));
    sprintf(nameBuf, "%s.dat", fileNameBase);
    cJSON_AddItemToArray(files, cJSON_CreateString(nameBuf));

    cJSON_AddItemToObject(data, "WaveFile", files);

    // FilePath, FileSizeDat, FileSizeCfg 按要求不输出或输出空
    // 如果想输出 Size，可以使用 g_WaveRecordCtrl.recordedBytes 和 g_WaveRecordCtrl.cfgFileSize

    cJSON_AddItemToObject(report, "Data", data);

    // 发送消息
    char *string = cJSON_PrintUnformatted(report);
    if (string)
    {
        size_t len = strlen(string);
        char *finalStr = malloc(len + 3);
        if (finalStr)
        {
            snprintf(finalStr, len + 3, "|%s|", string);
            MsgQue_write(finalStr, strlen(finalStr));
            free(finalStr);
        }
        free(string);
    }
    cJSON_Delete(report);
}

void WaveRecord_Start(u32 duration_ms, const char *source)
{
    g_WaveRecordCtrl.sampleSequence = 0;
    g_WaveRecordCtrl.recordedBytes = 0;
    g_WaveRecordCtrl.writeAddrOffset = 0;

    // 1. 记录来源和目标时长
    if (source)
    {
        strncpy(g_WaveRecordCtrl.recFrom, source, 31);
    }
    else
    {
        strcpy(g_WaveRecordCtrl.recFrom, "Unknown");
    }
    g_WaveRecordCtrl.targetDurationMs = duration_ms;

    // 2. 记录启动时间 (保存到结构体用于生成文件名)
    read_current_time(&g_WaveRecordCtrl.startTimestamp);

    // 生成 CFG 用的时间字符串 (保持 COMTRADE 格式)
    In_CurrTime *curr = &g_WaveRecordCtrl.startTimestamp;
    sprintf(g_WaveRecordCtrl.startTimeStr, "%04u/%02u/%02u,%02u:%02u:%02u.%06u",
            curr->curr_year, curr->curr_month, curr->curr_day,
            curr->curr_hour, curr->curr_minute, curr->curr_second,
            (unsigned int)curr->curr_subsec);

    // 3. 计算最大字节数
    if (duration_ms <= 0)
    {
        g_WaveRecordCtrl.maxRecordBytes = REC_DAT_MAX_SIZE;
    }
    else
    {
        u64 bytes = (u64)duration_ms * FS_RATE / 1000 * COMTRADE_FRAME_SIZE;
        if (bytes > REC_DAT_MAX_SIZE)
            bytes = REC_DAT_MAX_SIZE;
        g_WaveRecordCtrl.maxRecordBytes = (u32)bytes;
    }

    // 清除标志
    g_WaveRecordCtrl.isPendingSave = false;
    g_WaveRecordCtrl.isRecording = true;

    // 4. 精确记录时间戳用于裁切
    g_WaveRecordCtrl.startTimeUs = Get_Current_Time_US();
    g_WaveRecordCtrl.isFirstBlock = true;

    xil_printf("CPU1: WaveRecord Started (%s)...\r\n", g_WaveRecordCtrl.recFrom);

    // 【新增】立即上报 WaveRecordStarted
    Report_WaveRecordStart();
}

void WaveRecord_Stop(void)
{
    if (g_WaveRecordCtrl.isRecording)
    {
        g_WaveRecordCtrl.isRecording = false;  // 立即停止接收数据
        g_WaveRecordCtrl.isPendingSave = true; // 标记需要生成文件
    }
}

/**
 * @brief 处理一块数据 (500ms) 进行录波存储
 * 包含首块数据的“无效历史数据”剔除功能
 */
void WaveRecord_Process(u16 *pRawData, int points_count)
{
    // 1. 检查是否有挂起的保存任务
    if (g_WaveRecordCtrl.isPendingSave)
    {
        Generate_Comtrade_CFG();
        // 生成完文件后，立即上报 WaveRecordFileCreated
        Report_WaveRecordFileCreated();
        g_WaveRecordCtrl.isPendingSave = false;
    }

    // 2. 如果没在录波，直接返回
    if (!g_WaveRecordCtrl.isRecording)
        return;

    // 3. 指针与计数器准备
    // 默认情况下，处理整个 Buffer
    u16 *pValidData = pRawData;      // 有效数据的起始指针
    int valid_points = points_count; // 本次需要处理的有效点数

    // 【核心逻辑】如果是启动后的第一块数据，计算并剔除启动前的无效波形
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
    Xil_DCacheFlushRange(REC_SHARE_BASE + g_WaveRecordCtrl.writeAddrOffset - current_block_size, current_block_size);

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

    // 9. 记录 CFG 大小
    g_WaveRecordCtrl.cfgFileSize = len;

    // 打印日志
    xil_printf("CPU1: SetTaskWaveRecord Stopped. Total: %u bytes. CFG generated at 0x%X\r\n", g_WaveRecordCtrl.recordedBytes, REC_CFG_ADDR);
}