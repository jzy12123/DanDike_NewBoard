/*
 * ReplayWave.c - 波形回放模块实现
 *
 * 数据通路：共享DDR(.dat) → AX+B变换 → tx_buffer → AXI DMA → FIFO → DA输出
 * 驱动方式：FIFO prog_empty中断 → ReplayWave_FeedNext() 喂下一个周波
 */

#include "ReplayWave.h"
#include "ADDA.h"
#include "Amplifier_Switch.h"
#include "WaveRecord.h"
#include "Communications_Protocol.h"
#include "Msg_Que.h"
#include "cJSON.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "soft_timer.h"

/* ================================================================
 *  全局变量
 * ================================================================ */
ReplayWave_Config_t g_ReplayConfig;
ReplayWave_Runtime_t g_ReplayRuntime;

/* 用于动态格式化具体的错误信息返回给 JSON */
static char g_ReplayErrMsg[128] = {0};

/* 用于记录上一次满幅缩放结果的缓存 (按硬件通道索引 0-7 存储 A_final) */
typedef struct
{
    bool valid;
    char waveFile[REPLAY_WAVE_NAME_LEN];
    double a_final_cache[8];
} ReplayWave_History_t;

static ReplayWave_History_t g_ReplayHistory = {0};

/* FIFO prog_empty 中断号 (来自 xparameters.h) */
#define REPLAY_FIFO_INTR_ID XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_ADDA_AXIS_DATA_FIFO_1_PROG_EMPTY_INTR

/* 外部引用 */
extern XIntc AxiIntc_BareMetal;
extern XAxiDma axidma;
extern u16 *tx_buffer_ptr;
extern u16 enable;
extern u32 Wave_Range[8];
extern u32 g_do_output_state;
extern void OnOff_Write_Continuous(u32 data);

/* ================================================================
 *  辅助函数：获取当前时间字符串
 *  复用 ADDA.c 中的 Get_Current_Time_US() 和 soft_timer 的时间读取
 * ================================================================ */
static void Get_TimeStr(char *buf, int bufLen)
{
    In_CurrTime ct;
    read_current_time(&ct);

    /* 软时钟寄存器直接给出整数值 (非BCD) */
    snprintf(buf, bufLen, "%04u-%02u-%02u %02u:%02u:%02u.%06u", ct.curr_year,
             ct.curr_month, ct.curr_day, ct.curr_hour, ct.curr_minute,
             ct.curr_second,
             (unsigned int)(ct.curr_subsec / 10)); /* 0.1us计数 → us */
}

/* 获取当前时间的微秒时间戳 (日内微秒, 用于DO延时计时) */
static u64 Get_TimestampUs(void)
{
    return Get_Current_Time_US();
}

/* ================================================================
 *  辅助函数：发送JSON消息
 * ================================================================ */
static void Send_JSON(cJSON *json)
{
    char *str = cJSON_PrintUnformatted(json);
    if (str)
    {
        MsgQue_write(str, strlen(str));
        free(str);
    }
    cJSON_Delete(json);
}

/* ================================================================
 *  COMTRADE CFG 解析器
 *
 *  解析CFG字符串，提取模拟通道信息、采样率、触发时间等
 *  CFG格式：COMTRADE 1999标准，行分隔符为 \n
 * ================================================================ */

/* 从 "dd/mm/yyyy,hh:mm:ss.ssssss" 提取日内微秒 */
static u64 Parse_Comtrade_TimeUs(const char *dateStr, const char *timeStr)
{
    int hh = 0, mm = 0, ss = 0, us = 0;
    /* 解析 "hh:mm:ss.ssssss" */
    if (timeStr)
    {
        sscanf(timeStr, "%d:%d:%d.%d", &hh, &mm, &ss, &us);
        /* 处理不同精度: 3位→ms, 6位→us */
        int len = 0;
        const char *dot = strchr(timeStr, '.');
        if (dot)
            len = strlen(dot + 1);
        if (len == 3)
            us *= 1000; /* ms → us */
        else if (len < 3)
            us *= 1000;
    }
    return (u64)hh * 3600000000ULL + (u64)mm * 60000000ULL +
           (u64)ss * 1000000ULL + (u64)us;
}

static const char *ReplayWave_ParseCfg(const char *cfgStr)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;

    /* 【详细报错 1】：检查CFG字符串是否为空 */
    if (!cfgStr || cfgStr[0] == '\0')
    {
        return "CfgParseError: CFG string is completely empty";
    }

    /* 创建可修改的副本 */
    int cfgLen = strlen(cfgStr);
    char *cfgCopy = (char *)malloc(cfgLen + 1);
    if (!cfgCopy)
        return "CfgParseError: Malloc failed (Out of memory)";
    strcpy(cfgCopy, cfgStr);

    /* 替换 \r 为空格 */
    char *p = cfgCopy;
    while (*p)
    {
        if (*p == '\r')
            *p = ' ';
        p++;
    }

    /* ============================================================
     * 逐行解析 — 使用 strchr 手动分行，避免 strtok 嵌套问题
     * ============================================================ */
    int lineNum = 0;
    int analogParsed = 0;
    int digitalParsed = 0;
    u64 startTimeUs = 0, trigTimeUs = 0;
    char startDateStr[32] = "", startTimeStr_[32] = "";
    char trigDateStr[32] = "", trigTimeStr_[32] = "";

    char *lineStart = cfgCopy;
    while (lineStart && *lineStart)
    {
        lineNum++;

        /* 找到行尾 */
        char *lineEnd = strchr(lineStart, '\n');
        if (lineEnd)
        {
            *lineEnd = '\0'; /* 截断为一行 */
        }

        /* 跳过行首空格 */
        char *line = lineStart;
        while (*line == ' ')
            line++;

        /* 推进到下一行 (在处理当前行之前就计算好) */
        lineStart = lineEnd ? (lineEnd + 1) : NULL;

        /* 跳过空行 */
        if (*line == '\0')
            continue;

        if (lineNum == 1)
        {
            /* Line 1: station_name,rec_dev_id,rev_year - 跳过 */
        }
        else if (lineNum == 2)
        {
            /* Line 2: TT,##A,##D */
            int tt = 0;
            char aBuf[8] = "", dBuf[8] = "";
            sscanf(line, "%d,%[^,],%s", &tt, aBuf, dBuf);
            cfg->analogCount = atoi(aBuf);  /* "8A" → 8 */
            cfg->digitalCount = atoi(dBuf); /* "0D" → 0 */

            /* 【详细报错 2】：检查模拟通道数量是否合法 */
            if (cfg->analogCount <= 0 || cfg->analogCount > REPLAY_MAX_FILE_CHANNELS)
            {
                snprintf(g_ReplayErrMsg, sizeof(g_ReplayErrMsg),
                         "CfgParseError: Invalid analog count (%d). Max is %d.",
                         cfg->analogCount, REPLAY_MAX_FILE_CHANNELS);
                free(cfgCopy);
                return g_ReplayErrMsg;
            }
            cfg->digitalGroups = (cfg->digitalCount + 15) / 16;
        }
        else if (lineNum >= 3 && analogParsed < cfg->analogCount)
        {
            /* ==============================================
             * 模拟通道行: An,ch_id,ph,ccbm,uu,a,b,skew,min,max,...
             * 注意: ph 和 ccbm 可能为空 (连续逗号 ,,)
             * 使用 strchr 手动分割，正确处理空字段
             * ============================================== */
            ReplayCfgAnalog_t *ch = &cfg->cfgAnalog[analogParsed];
            memset(ch, 0, sizeof(*ch));

            char lineCopy[256];
            strncpy(lineCopy, line, sizeof(lineCopy) - 1);
            lineCopy[255] = '\0';

            int field = 0;
            char *fieldStart = lineCopy;
            while (fieldStart)
            {
                /* 找逗号 */
                char *comma = strchr(fieldStart, ',');
                if (comma)
                    *comma = '\0';

                /* 跳过字段首尾空格 */
                char *val = fieldStart;
                while (*val == ' ')
                    val++;

                switch (field)
                {
                case 0:
                    ch->index = atoi(val);
                    break;
                case 1:
                    strncpy(ch->name, val, sizeof(ch->name) - 1);
                    break;
                case 2:
                    strncpy(ch->phase, val, sizeof(ch->phase) - 1);
                    break;
                /* case 3: ccbm - skip */
                case 4:
                    strncpy(ch->unit, val, sizeof(ch->unit) - 1);
                    break;
                case 5:
                    ch->a = atof(val);
                    break;
                case 6:
                    ch->b = atof(val);
                    break;
                /* case 7: skew - skip */
                case 8:
                    ch->minVal = (int16_t)atoi(val);
                    break;
                case 9:
                    ch->maxVal = (int16_t)atoi(val);
                    break;
                }
                field++;

                /* 推进到下一个字段 */
                fieldStart = comma ? (comma + 1) : NULL;
            }
            analogParsed++;
        }
        else if (analogParsed >= cfg->analogCount &&
                 digitalParsed < cfg->digitalCount)
        {
            /* 数字通道行 - 跳过 */
            digitalParsed++;
        }
        else
        {
            /* 后续公共行 */
            int publicLine = lineNum - 2 - cfg->analogCount - cfg->digitalCount;

            switch (publicLine)
            {
            case 1: /* 频率 - skip */
                break;
            case 2: /* nrates */
                break;
            case 3:
            {
                /* 采样率, 末尾样本号 (兼容带小数点的格式) */
                char rateBuf[32] = "";
                char endBuf[32] = "";
                /* 读到逗号为止算作rateBuf，逗号后面算作endBuf */
                if (sscanf(line, "%[^,],%s", rateBuf, endBuf) == 2)
                {
                    /* 用 atof 转 double 后再转 int，可以完美兼容 51200 和 51200.000000 */
                    cfg->fileSampleRate = (int)atof(rateBuf);
                    /* 样本数通常是整数，直接 atoi 即可 */
                    cfg->totalSamples = (u32)atoi(endBuf);
                }
                break;
            }
            case 4:
            {
                /* 首个数据点时间: yyyy/mm/dd,hh:mm:ss.ssssss */
                char *comma = strchr(line, ',');
                if (comma)
                {
                    *comma = '\0';
                    strncpy(startDateStr, line, 31);
                    strncpy(startTimeStr_, comma + 1, 31);
                    startTimeUs = Parse_Comtrade_TimeUs(startDateStr, startTimeStr_);
                }
                break;
            }
            case 5:
            {
                /* 触发时间 */
                char *comma = strchr(line, ',');
                if (comma)
                {
                    *comma = '\0';
                    strncpy(trigDateStr, line, 31);
                    strncpy(trigTimeStr_, comma + 1, 31);
                    trigTimeUs = Parse_Comtrade_TimeUs(trigDateStr, trigTimeStr_);
                }
                break;
            }
            case 6:
            {
                /* 文件类型: BINARY/ASCII */
                if (strstr(line, "ASCII"))
                {
                    free(cfgCopy);
                    return "CfgParseError: ASCII format not supported. Please use BINARY."; /* 不支持ASCII */
                }
                break;
            }
                /* case 7: timemult - skip */
            }
        }
    }

    free(cfgCopy);

    /* ============================================================
     * 【详细报错 3】：最终完整性核查
     * ============================================================ */
    if (lineNum < 2)
    {
        return "CfgParseError: CFG content is too short (Missing basic lines)";
    }

    if (analogParsed < cfg->analogCount)
    {
        snprintf(g_ReplayErrMsg, sizeof(g_ReplayErrMsg),
                 "CfgParseError: Expected %d analog channels, but only parsed %d",
                 cfg->analogCount, analogParsed);
        return g_ReplayErrMsg;
    }

    if (cfg->totalSamples == 0)
    {
        return "CfgParseError: TotalSamples is 0 (Check sample rate line format)";
    }

    if (cfg->fileSampleRate == 0)
        cfg->fileSampleRate = REPLAY_SAMPLE_RATE;

    /* 打印解析结果 */
    xil_printf("ReplayWave: ParseCfg OK: %d analog, %d digital, rate=%d, samples=%u\r\n",
               cfg->analogCount, cfg->digitalCount,
               cfg->fileSampleRate, (unsigned int)cfg->totalSamples);
    for (int i = 0; i < cfg->analogCount; i++)
    {
        printf("  Ch%d [%s] unit=%s a=%.6f b=%.6f min=%d max=%d\r\n",
               cfg->cfgAnalog[i].index, cfg->cfgAnalog[i].name,
               cfg->cfgAnalog[i].unit,
               cfg->cfgAnalog[i].a, cfg->cfgAnalog[i].b,
               (int)cfg->cfgAnalog[i].minVal, (int)cfg->cfgAnalog[i].maxVal);
    }

    /* 计算每条记录的字节数 */
    /* BINARY: 4(序号) + 4(时间戳μs) + 2*analog + 2*digital_groups */
    cfg->recordSize = 4 + 4 + cfg->analogCount * 2 + cfg->digitalGroups * 2;

    /* 计算触发点采样索引 */
    if (trigTimeUs > startTimeUs)
    {
        u64 diffUs = trigTimeUs - startTimeUs;
        cfg->rawTrigSample = (u32)(diffUs * cfg->fileSampleRate / 1000000ULL);
    }
    else
    {
        cfg->rawTrigSample = 0;
    }

    /* 计算区域边界 */
    cfg->firstCycleEnd = REPLAY_CYCLE_LEN;

    /* 【修复相位跳变】：将 lastCycleStart 严格向下对齐到周波边界 */
    u32 alignedTotal = (cfg->totalSamples / REPLAY_CYCLE_LEN) * REPLAY_CYCLE_LEN;
    if (alignedTotal > REPLAY_CYCLE_LEN)
    {
        cfg->lastCycleStart = alignedTotal - REPLAY_CYCLE_LEN;
    }
    else
    {
        cfg->lastCycleStart = 0;
    }

    return 0;
}

/* ================================================================
 *  硬件通道索引计算
 *  Type='U', Chn=1→0(UA), 2→1(UB), 3→2(UC), 4→3(UX)
 *  Type='I', Chn=1→4(IA), 2→5(IB), 3→6(IC), 4→7(IX)
 * ================================================================ */
static int Calc_HwIndex(char type, int chn)
{
    int base = (type == 'I' || type == 'i') ? 4 : 0;
    return base + (chn - 1);
}

/* ================================================================
 *  获取通道对应的当前功放量程满量程值 (物理值)
 *  Wave_Range[i] 存储的是量程档位索引
 * ================================================================ */
static double Get_ChannelFullScale(int hwIndex)
{
    u32 range = (u32)Wave_Range[hwIndex];
    if (hwIndex < 4)
    {
        /* 电压通道 */
        switch (range)
        {
        case 0:
            return 1.876;
        case 1:
            return 3.25;
        case 2:
            return 6.5;
        default:
            return 6.5;
        }
    }
    else
    {
        /* 电流通道 */
        switch (range)
        {
        case 0:
            return 0.2;
        case 1:
            return 1.0;
        case 2:
            return 5.0;
        default:
            return 5.0;
        }
    }
}

/* ================================================================
 *  构建通道映射 + 预计算AX+B系数
 * ================================================================ */
static const char *ReplayWave_BuildMapping(cJSON *mapArray)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    cfg->channelMapCount = 0;

    int count = cJSON_GetArraySize(mapArray);
    if (count > REPLAY_MAX_MAP_CHANNELS)
        count = REPLAY_MAX_MAP_CHANNELS;

    for (int i = 0; i < count; i++)
    {
        cJSON *item = cJSON_GetArrayItem(mapArray, i);
        ReplayChannelMap_t *m = &cfg->channelMap[i];
        memset(m, 0, sizeof(*m));

        m->line = cJSON_GetObjectItem(item, "Line") ? cJSON_GetObjectItem(item, "Line")->valueint : 1;
        m->chn = cJSON_GetObjectItem(item, "Chn") ? cJSON_GetObjectItem(item, "Chn")->valueint : 1;

        cJSON *typeItem = cJSON_GetObjectItem(item, "Type");
        m->type = (typeItem && typeItem->valuestring) ? typeItem->valuestring[0] : 'U';

        m->mapChn = cJSON_GetObjectItem(item, "MapChn") ? cJSON_GetObjectItem(item, "MapChn")->valueint : 1;
        m->ratio = cJSON_GetObjectItem(item, "Ratio") ? cJSON_GetObjectItem(item, "Ratio")->valuedouble : 1.0;
        m->delayMS = cJSON_GetObjectItem(item, "DelayMS") ? cJSON_GetObjectItem(item, "DelayMS")->valuedouble : 0.0;

        /* 验证文件通道号有效 */
        if (m->mapChn < 1 || m->mapChn > cfg->analogCount)
        {
            snprintf(g_ReplayErrMsg, sizeof(g_ReplayErrMsg),
                     "MapParseError: Invalid MapChn=%d (Max AnalogCount is %d)",
                     m->mapChn, cfg->analogCount);
            return g_ReplayErrMsg;
        }

        /* 计算硬件索引 */
        m->hwIndex = Calc_HwIndex(m->type, m->chn);
        if (m->hwIndex < 0 || m->hwIndex > 7)
        {
            snprintf(g_ReplayErrMsg, sizeof(g_ReplayErrMsg),
                     "MapParseError: Invalid HwChannel (Type='%c', Chn=%d)",
                     m->type, m->chn);
            return g_ReplayErrMsg;
        }

        /* 延时采样点数 */
        m->delaySamples = (int)(m->delayMS * REPLAY_SAMPLE_RATE / 1000.0);

        /* 查找CFG中的AX+B系数 (mapChn是1-based) */
        ReplayCfgAnalog_t *cfgCh = &cfg->cfgAnalog[m->mapChn - 1];

        /* 获取当前量程满量程值 */
        double fullScale = Get_ChannelFullScale(m->hwIndex);

        /*
         * 变换链 (与 ADDA.c Write_Wave_to_Wave_NewData 公式保持一致):
         *   physical = a_cfg * raw + b_cfg     (COMTRADE标准)
         *   physical_scaled = physical * ratio  (用户额外缩放)
         *   normalized = physical_scaled / fullScale  (归一化到 [-1, +1])
         *   dac_u16 = normalized * 32768 + 32767      (映射到 [0, 65535], 中值32767)
         *
         * 预计算 (将 raw → dac_u16 合并):
         *   dac_u16 = raw * (a_cfg * ratio / fullScale * 32768)
         *           + (b_cfg * ratio / fullScale * 32768)
         *           + 32767
         *         = raw * A_final + B_final + 32767
         *
         * 实际使用时: dacCode = raw * A_final + B_final (有符号浮点)
         *            dacVal  = clamp(dacCode + 32767, 0, 65535)
         */
        double fullScalePeak = fullScale * 1.41421356;
        m->A_final = cfgCh->a * m->ratio / fullScalePeak * 32767.0;
        m->B_final = cfgCh->b * m->ratio / fullScalePeak * 32767.0;

        cfg->channelMapCount++;
    }

    return NULL; /* 成功 */
}

/* ================================================================
 *  扫描 .dat 实际数据峰值
 *
 *  遍历所有采样点，对每个映射通道找出 rawMin/rawMax
 *  结果存入 channelMap[i].rawActualMin / rawActualMax
 * ================================================================ */
static void ReplayWave_ScanPeak(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    u8 *datBase = (u8 *)REPLAY_DAT_ADDR;

    /* 初始化 */
    for (int m = 0; m < cfg->channelMapCount; m++)
    {
        cfg->channelMap[m].rawActualMin = 0;
        cfg->channelMap[m].rawActualMax = 0;
    }

    xil_printf("ReplayWave: ScanPeak scanning %u samples...\r\n",
               (unsigned int)cfg->totalSamples);

    /* 遍历所有采样点 */
    for (u32 s = 0; s < cfg->totalSamples; s++)
    {
        u8 *pRecord = datBase + s * cfg->recordSize;
        /* 跳过: 4字节序号 + 4字节时间戳 → 模拟通道数据 */
        int16_t *pAnalog = (int16_t *)(pRecord + 8);

        for (int m = 0; m < cfg->channelMapCount; m++)
        {
            ReplayChannelMap_t *map = &cfg->channelMap[m];
            int16_t rawVal = pAnalog[map->mapChn - 1];

            if (rawVal < map->rawActualMin)
                map->rawActualMin = rawVal;
            if (rawVal > map->rawActualMax)
                map->rawActualMax = rawVal;
        }
    }

    /* 打印扫描结果 */
    for (int m = 0; m < cfg->channelMapCount; m++)
    {
        ReplayChannelMap_t *map = &cfg->channelMap[m];
        ReplayCfgAnalog_t *cfgCh = &cfg->cfgAnalog[map->mapChn - 1];
        xil_printf("  [DEBUG] Ch%d[%s] → hw%c%d: rawMin=%d rawMax=%d\r\n",
                   cfgCh->index, cfgCh->name,
                   map->type, map->chn,
                   map->rawActualMin, map->rawActualMax);
    }
    xil_printf("ReplayWave: ScanPeak done.\r\n");
}

/* ================================================================
 *  量程检查: 用实际扫描峰值检查是否超出当前功放量程
 *  超出 → 返回 "OutDevRange" 错误 (阻断回放)
 *
 *  注意: 如果 rawPeak 已接近满幅 (≥95%), 说明数据已被前一次缩放过,
 *        此时 CFG 中的 a 系数是原始值，不能直接用于计算物理峰值。
 *        跳过该通道 (前一次已验证安全性)。
 * ================================================================ */
static const char *ReplayWave_CheckRange(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;

    bool historyMatch = (g_ReplayHistory.valid &&
                         strcmp(g_ReplayHistory.waveFile, cfg->waveFile) == 0);

    for (int i = 0; i < cfg->channelMapCount; i++)
    {
        ReplayChannelMap_t *m = &cfg->channelMap[i];
        ReplayCfgAnalog_t *cfgCh = &cfg->cfgAnalog[m->mapChn - 1];

        /* 判断数据是否已被缩放 (rawPeak >= 95% × 32767) */
        int16_t absMin = (m->rawActualMin == -32768) ? 32767 : (int16_t)(-m->rawActualMin);
        int16_t absMax = m->rawActualMax;
        int16_t rawPeak = (absMin > absMax) ? absMin : absMax;

        /* 只有波形文件名相同，并且数据接近满幅，才认为是被前一次缩放过的安全数据 */
        if (historyMatch && rawPeak >= 31129)
        {
            printf("  [DEBUG] Ch%d[%s] SKIP range check (matched history, rawPeak=%d)\r\n",
                   cfgCh->index, cfgCh->name, rawPeak);
            continue;
        }

        /* 计算物理值极限 (基于实际扫描的rawMin/rawMax) */
        double physMax = fabs(cfgCh->a * m->rawActualMax + cfgCh->b) * m->ratio;
        double physMin = fabs(cfgCh->a * m->rawActualMin + cfgCh->b) * m->ratio;
        double physPeak = (physMax > physMin) ? physMax : physMin;

        double fullScale = Get_ChannelFullScale(m->hwIndex);
        // 放大器量程 fullScale 给出的是有效值 (RMS)，
        // 物理硬件能够输出的最大峰值是其 1.414 倍。
        double fullScalePeak = fullScale * 1.41421356;

        /* 增加 1.2 倍的允许余量 */
        double maxAllowablePeak = fullScalePeak * 1.2;

        printf("  [DEBUG] Ch%d[%s] physPeak=%.4f maxAllowablePeak=%.3f (120%% limit) %s\r\n", cfgCh->index, cfgCh->name, physPeak, maxAllowablePeak, (physPeak > maxAllowablePeak) ? "EXCEED!" : "OK");

        if (physPeak > maxAllowablePeak)
        {
            snprintf(g_ReplayErrMsg, sizeof(g_ReplayErrMsg),
                     "OutDevRange: Ch%d[%s] Peak=%.2f > Limit=%.2f",
                     cfgCh->index, cfgCh->name, physPeak, maxAllowablePeak);
            return g_ReplayErrMsg;
        }
    }
    return NULL;
}

/* ================================================================
 *  满幅缩放: 就地覆写 .dat 中的 raw 数据，使其尽量充满 ±32767
 *
 *  对每个映射通道:
 *    1) 计算 scaleF = 32767 / max(|rawMin|, |rawMax|)
 *    2) 若 rawPeak >= 32767*0.95 → 跳过 (已接近满幅)
 *    3) 遍历所有采样点: new_raw = clamp(raw * scaleF, -32768, 32767)
 *    4) 补偿CFG系数: a_new = a_old / scaleF
 *    5) 重算 A_final
 *
 *  注意: 会覆写共享DDR中的 .dat 数据！
 *        如果Linux未重新写入数据，下次调用时 rawPeak 已接近32767，会自动跳过
 * ================================================================ */
static void ReplayWave_RescaleData(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    u8 *datBase = (u8 *)REPLAY_DAT_ADDR;

    xil_printf("ReplayWave: RescaleData starting...\r\n");

    bool historyMatch = (g_ReplayHistory.valid &&
                         strcmp(g_ReplayHistory.waveFile, cfg->waveFile) == 0);

    for (int m = 0; m < cfg->channelMapCount; m++)
    {
        ReplayChannelMap_t *map = &cfg->channelMap[m];
        ReplayCfgAnalog_t *cfgCh = &cfg->cfgAnalog[map->mapChn - 1];

        int16_t absMin = (map->rawActualMin == -32768) ? 32767 : (int16_t)(-map->rawActualMin);
        int16_t absMax = map->rawActualMax;
        int16_t rawPeak = (absMin > absMax) ? absMin : absMax;

        /* 跳过条件: rawPeak 已接近满幅 (≥95% × 32767)
         * 说明数据已被前一次缩放过。优先使用历史记录恢复A_final */
        if (rawPeak >= 31129)
        {                            /* 32767 * 0.95 ≈ 31129 */
            map->scaleApplied = 1.0; /* 本次不做缩放 */

            if (historyMatch)
            {
                /* 命中历史记录，直接使用上一次的 A_final */
                map->A_final = g_ReplayHistory.a_final_cache[map->hwIndex];
                printf("  [DEBUG] Ch%d[%s] SKIP rescale (matched history), restored A_final=%.6f\r\n",
                       cfgCh->index, cfgCh->name, map->A_final);
            }
            else
            {
                /* 未命中历史(通常不会发生)，回退到原始估算逻辑 */
                int16_t cfgAbsMin = (cfgCh->minVal == -32768) ? 32767 : (int16_t)(-cfgCh->minVal);
                int16_t cfgAbsMax = cfgCh->maxVal;
                int16_t origRawPeak = (cfgAbsMin > cfgAbsMax) ? cfgAbsMin : cfgAbsMax;

                if (origRawPeak > 0 && origRawPeak < rawPeak)
                {
                    double prevScaleF = (double)rawPeak / (double)origRawPeak;
                    cfgCh->a = cfgCh->a / prevScaleF;
                    double fullScale = Get_ChannelFullScale(map->hwIndex);
                    double fullScalePeak = fullScale * 1.41421356;
                    map->A_final = cfgCh->a * map->ratio / fullScalePeak * 32767.0;
                    printf("  [DEBUG] Ch%d[%s] SKIP rescale (no history), estimated prevScaleF=%.4f A_final=%.6f\r\n",
                           cfgCh->index, cfgCh->name, prevScaleF, map->A_final);
                }
                else
                {
                    xil_printf("  [DEBUG] Ch%d[%s] SKIP rescale (rawPeak=%d, already >=95%%)\r\n",
                               cfgCh->index, cfgCh->name, rawPeak);
                }
            }
            continue;
        }

        if (rawPeak == 0)
        {
            map->scaleApplied = 1.0;
            xil_printf("  [DEBUG] Ch%d[%s] SKIP rescale (rawPeak=0, no data)\r\n",
                       cfgCh->index, cfgCh->name);
            continue;
        }

        double scaleF = 32767.0 / (double)rawPeak;
        map->scaleApplied = scaleF;
        double a_old = cfgCh->a;

        printf("  [DEBUG] Ch%d[%s] rawPeak=%d scaleF=%.4f\r\n",
               cfgCh->index, cfgCh->name, rawPeak, scaleF);

        /* 遍历所有采样点，就地缩放该通道的 raw 值 */
        int chOffset = (map->mapChn - 1); /* 通道在记录中的偏移 (int16单位) */
        for (u32 s = 0; s < cfg->totalSamples; s++)
        {
            int16_t *pVal = (int16_t *)(datBase + s * cfg->recordSize + 8) + chOffset;
            int32_t scaled = (int32_t)((double)(*pVal) * scaleF);
            /* 钳位 */
            if (scaled > 32767)
                scaled = 32767;
            if (scaled < -32768)
                scaled = -32768;
            *pVal = (int16_t)scaled;
        }

        /* 补偿CFG系数: a_new = a_old / scaleF */
        cfgCh->a = a_old / scaleF;

        /* 重算 A_final (B_final 不受 raw 缩放影响) */
        double fullScale = Get_ChannelFullScale(map->hwIndex);
        double fullScalePeak = fullScale * 1.41421356;
        map->A_final = cfgCh->a * map->ratio / fullScalePeak * 32767.0;

        printf("  [DEBUG] Ch%d[%s] a: %.6f -> %.6f, A_final=%.6f\r\n",
               cfgCh->index, cfgCh->name, a_old, cfgCh->a, map->A_final);
    }

    /* 记录本次结果到历史缓存 */
    g_ReplayHistory.valid = true;
    strncpy(g_ReplayHistory.waveFile, cfg->waveFile, REPLAY_WAVE_NAME_LEN - 1);
    for (int m = 0; m < cfg->channelMapCount; m++)
    {
        ReplayChannelMap_t *map = &cfg->channelMap[m];
        g_ReplayHistory.a_final_cache[map->hwIndex] = map->A_final;
    }

    /* 刷回 D-Cache，确保覆写的数据对 DMA 可见 */
    Xil_DCacheFlushRange(REPLAY_DAT_ADDR, cfg->datFileSize);

    xil_printf("ReplayWave: RescaleData done. D-Cache flushed.\r\n");
}

/* ================================================================
 *  计算区域边界
 * ================================================================ */
static void ReplayWave_CalcRegions(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;

    /* 中段循环边界 */
    if (cfg->repeatMiddle.repeatCount > 0)
    {
        if (cfg->repeatMiddle.beginMS < 0)
        {
            /* 使用原始触发点 */
            cfg->repeatMiddle.startSample = cfg->rawTrigSample;
        }
        else
        {
            /* 从 PreEndTime 开始的偏移 */
            u32 offsetSamples = (u32)(cfg->repeatMiddle.beginMS * REPLAY_SAMPLE_RATE / 1000.0);
            cfg->repeatMiddle.startSample = cfg->firstCycleEnd + offsetSamples;
        }
        /* 【修复相位跳变】：将中段起点严格向下对齐到周波边界，保证无缝切入 */
        cfg->repeatMiddle.startSample = (cfg->repeatMiddle.startSample / REPLAY_CYCLE_LEN) * REPLAY_CYCLE_LEN;
        u32 lenSamples = (u32)(cfg->repeatMiddle.lengthMS * REPLAY_SAMPLE_RATE / 1000.0);
        /* 对齐到周波边界 */
        lenSamples = ((lenSamples + REPLAY_CYCLE_LEN - 1) / REPLAY_CYCLE_LEN) * REPLAY_CYCLE_LEN;
        if (lenSamples == 0)
            lenSamples = REPLAY_CYCLE_LEN;
        cfg->repeatMiddle.endSample = cfg->repeatMiddle.startSample + lenSamples;

        /* 钳位 */
        if (cfg->repeatMiddle.endSample > cfg->lastCycleStart)
            cfg->repeatMiddle.endSample = cfg->lastCycleStart;
        if (cfg->repeatMiddle.startSample >= cfg->repeatMiddle.endSample)
            cfg->repeatMiddle.repeatCount = 0; /* 无效区间，禁用 */
    }
}

/* ================================================================
 *  解析 DO 配置
 * ================================================================ */
static void ReplayWave_ParseDO(cJSON *doArray)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    cfg->doConfigCount = 0;

    if (!doArray)
        return;
    int count = cJSON_GetArraySize(doArray);
    if (count > REPLAY_MAX_DO_CONFIG)
        count = REPLAY_MAX_DO_CONFIG;

    for (int i = 0; i < count; i++)
    {
        cJSON *item = cJSON_GetArrayItem(doArray, i);
        ReplayDO_Config_t *d = &cfg->doConfig[i];
        memset(d, 0, sizeof(*d));

        d->chn = cJSON_GetObjectItem(item, "Chn") ? cJSON_GetObjectItem(item, "Chn")->valueint : 0;

        cJSON *modeItem = cJSON_GetObjectItem(item, "Mode");
        if (modeItem && modeItem->valuestring)
        {
            if (strcmp(modeItem->valuestring, "Keep") == 0)
                d->mode = DO_MODE_KEEP;
            else if (strcmp(modeItem->valuestring, "Turn") == 0)
                d->mode = DO_MODE_TURN;
            else if (strcmp(modeItem->valuestring, "Breaker") == 0)
                d->mode = DO_MODE_BREAKER;
            else if (strcmp(modeItem->valuestring, "Map") == 0)
                d->mode = DO_MODE_MAP;
        }

        d->val = cJSON_GetObjectItem(item, "val") ? cJSON_GetObjectItem(item, "val")->valueint : 0;

        /* Turn 模式参数 */
        if (d->mode == DO_MODE_TURN)
        {
            cJSON *ref = cJSON_GetObjectItem(item, "TimeRef");
            if (ref && ref->valuestring && strcmp(ref->valuestring, "RepeatPrevEnd") == 0)
                d->timeRef = DO_TIMEREF_REPEATPREVEND;
            else
                d->timeRef = DO_TIMEREF_START;

            d->delayMS = cJSON_GetObjectItem(item, "DelayMS") ? cJSON_GetObjectItem(item, "DelayMS")->valuedouble : 0;
            d->holdMS = cJSON_GetObjectItem(item, "HoldMS") ? cJSON_GetObjectItem(item, "HoldMS")->valuedouble : -1;
        }

        /* Breaker 模式参数 */
        if (d->mode == DO_MODE_BREAKER)
        {
            d->diCSChn = cJSON_GetObjectItem(item, "DICSChn") ? cJSON_GetObjectItem(item, "DICSChn")->valueint : 0;
            d->diOSChn = cJSON_GetObjectItem(item, "DIOSChn") ? cJSON_GetObjectItem(item, "DIOSChn")->valueint : 0;
            d->breakerDelayMS = cJSON_GetObjectItem(item, "DelayMS") ? cJSON_GetObjectItem(item, "DelayMS")->valuedouble : 0;
        }

        /* Map 模式参数 */
        if (d->mode == DO_MODE_MAP)
        {
            d->mapChn = cJSON_GetObjectItem(item, "MapChn") ? cJSON_GetObjectItem(item, "MapChn")->valueint : 1;
        }

        cfg->doConfigCount++;
    }
}

/* ================================================================
 *  解析前导/中段/后导循环配置
 * ================================================================ */
static void ReplayWave_ParseRepeat(cJSON *data)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;

    /* 前导 */
    memset(&cfg->repeatPrev, 0, sizeof(cfg->repeatPrev));
    cJSON *prev = cJSON_GetObjectItem(data, "RepeatPrev");
    if (prev)
    {
        cfg->repeatPrev.repeatCount = cJSON_GetObjectItem(prev, "RepeatCount") ? cJSON_GetObjectItem(prev, "RepeatCount")->valueint : 0;

        cJSON *brk = cJSON_GetObjectItem(prev, "BreakCondition");
        if (brk && cfg->repeatPrev.repeatCount > 0)
        {
            cfg->repeatPrev.breakEnable = cJSON_IsTrue(cJSON_GetObjectItem(brk, "Enable"));

            cJSON *trigArr = cJSON_GetObjectItem(brk, "TrigDI");
            if (trigArr)
            {
                int tc = cJSON_GetArraySize(trigArr);
                if (tc > REPLAY_MAX_DI_TRIG)
                    tc = REPLAY_MAX_DI_TRIG;
                for (int i = 0; i < tc; i++)
                {
                    cJSON *t = cJSON_GetArrayItem(trigArr, i);
                    cfg->repeatPrev.trigDIs[i].chn = cJSON_GetObjectItem(t, "Chn") ? cJSON_GetObjectItem(t, "Chn")->valueint : 0;
                    cfg->repeatPrev.trigDIs[i].val = cJSON_GetObjectItem(t, "val") ? cJSON_GetObjectItem(t, "val")->valueint : -1;
                }
                cfg->repeatPrev.trigCount = tc;
            }

            cJSON *logic = cJSON_GetObjectItem(brk, "TrigLogic");
            if (logic && logic->valuestring)
                cfg->repeatPrev.trigLogicAnd = (strcmp(logic->valuestring, "And") == 0);
        }
    }

    /* 中段 */
    memset(&cfg->repeatMiddle, 0, sizeof(cfg->repeatMiddle));
    cJSON *mid = cJSON_GetObjectItem(data, "RepeatMiddle");
    if (mid)
    {
        cfg->repeatMiddle.repeatCount = cJSON_GetObjectItem(mid, "RepeatCount") ? cJSON_GetObjectItem(mid, "RepeatCount")->valueint : 0;
        cfg->repeatMiddle.beginMS = cJSON_GetObjectItem(mid, "BeginMS") ? cJSON_GetObjectItem(mid, "BeginMS")->valuedouble : -1.0;
        cfg->repeatMiddle.lengthMS = cJSON_GetObjectItem(mid, "LengthMS") ? cJSON_GetObjectItem(mid, "LengthMS")->valuedouble : 20.0;
    }

    /* 后导 */
    memset(&cfg->repeatBack, 0, sizeof(cfg->repeatBack));
    cJSON *back = cJSON_GetObjectItem(data, "RepeatBack");
    if (back)
    {
        cfg->repeatBack.repeatCount = cJSON_GetObjectItem(back, "RepeatCount") ? cJSON_GetObjectItem(back, "RepeatCount")->valueint : 0;
    }
}

/* ================================================================
 *  初始化
 * ================================================================ */
void ReplayWave_Init(void)
{
    memset(&g_ReplayConfig, 0, sizeof(g_ReplayConfig));
    memset(&g_ReplayRuntime, 0, sizeof(g_ReplayRuntime));
    xil_printf("CPU1: ReplayWave Module Initialized.\r\n");
}

/* ================================================================
 *  GetWaveReplayFileInfo 处理
 * ================================================================ */
void ReplayWave_HandleGetInfo(cJSON *root)
{
    /* 读取控制头 */
    Xil_DCacheInvalidateRange(REPLAY_SHM_BASE, REPLAY_HEADER_SIZE);
    volatile ReplayHeader_t *hdr = (volatile ReplayHeader_t *)REPLAY_SHM_BASE;

    bool fileExist = (hdr->magic == REPLAY_MAGIC && hdr->status == REPLAY_STATUS_READY);

    /* 构建回复 */
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "GetWaveReplayFileInfo");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *data = cJSON_CreateObject();

    char timeStr[32];
    Get_TimeStr(timeStr, sizeof(timeStr));
    cJSON_AddStringToObject(data, "DevTime", timeStr);
    cJSON_AddBoolToObject(data, "FileExist", fileExist);

    if (fileExist)
    {
        cJSON *fileInfo = cJSON_CreateObject();
        char nameBuf[REPLAY_WAVE_NAME_LEN];
        memcpy(nameBuf, (const char *)hdr->waveFileName, REPLAY_WAVE_NAME_LEN);
        nameBuf[REPLAY_WAVE_NAME_LEN - 1] = '\0';

        cJSON_AddStringToObject(fileInfo, "WaveFile", nameBuf);
        cJSON_AddNumberToObject(fileInfo, "FileSizeDat", hdr->datFileSize);
        cJSON_AddBoolToObject(fileInfo, "ParasCompleted", g_ReplayConfig.parasCompleted);
        cJSON_AddItemToObject(data, "FileInfo", fileInfo);
    }

    cJSON_AddItemToObject(reply, "Data", data);
    Send_JSON(reply);
}

/* ================================================================
 *  SetTaskWaveReplayParas 处理
 *  返回值: NULL=成功, 非NULL=错误信息字符串
 * ================================================================ */
const char *ReplayWave_HandleParas(cJSON *data)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;

    /* 如果回放正在运行，先停止 */
    if (g_ReplayRuntime.isRunning)
    {
        ReplayWave_Stop();
    }

    /* 重置配置 */
    memset(cfg, 0, sizeof(*cfg));

    /* ---- 1. 获取文件名 ---- */
    cJSON *wfItem = cJSON_GetObjectItem(data, "WaveFile");
    if (wfItem && wfItem->valuestring)
    {
        strncpy(cfg->waveFile, wfItem->valuestring, REPLAY_WAVE_NAME_LEN - 1);
    }

    /* ---- 2. 检查文件是否存在 ---- */
    Xil_DCacheInvalidateRange(REPLAY_SHM_BASE, REPLAY_HEADER_SIZE);
    volatile ReplayHeader_t *hdr = (volatile ReplayHeader_t *)REPLAY_SHM_BASE;

    if (hdr->magic != REPLAY_MAGIC || hdr->status != REPLAY_STATUS_READY)
    {
        return "WaveFileNotExist";
    }
    cfg->datFileSize = hdr->datFileSize;

    /* ---- 3. 失效整个DAT数据区的D-Cache (确保能读到Linux写入的数据) ---- */
    Xil_DCacheInvalidateRange(REPLAY_DAT_ADDR, cfg->datFileSize);

    /* ---- 4. 解析CFG ---- */
    cJSON *cfgItem = cJSON_GetObjectItem(data, "Cfg");
    if (!cfgItem || !cJSON_IsString(cfgItem) || !cfgItem->valuestring)
    {
        return "CfgParseError: Missing 'Cfg' node in JSON data";
    }
    /* 直接接收具体的错误字符串并向上抛出 */
    const char *parseErr = ReplayWave_ParseCfg(cfgItem->valuestring);
    if (parseErr != NULL)
    {
        return parseErr;
    }

    /* ---- 5. 验证dat文件大小与CFG一致 ---- */
    u32 expectedSize = cfg->totalSamples * cfg->recordSize;
    if (cfg->datFileSize < expectedSize)
    {
        xil_printf("ReplayWave: DAT size mismatch: file=%u, expected=%u\r\n",
                   cfg->datFileSize, expectedSize);
        snprintf(g_ReplayErrMsg, sizeof(g_ReplayErrMsg),
                 "DatParseError: Size mismatch. File has %ld bytes, CFG expects %ld bytes",
                 cfg->datFileSize, expectedSize);
        return g_ReplayErrMsg;
    }

    /* ---- 6. 解析通道映射 ---- */
    cJSON *mapArray = cJSON_GetObjectItem(data, "Map");
    if (!mapArray || cJSON_GetArraySize(mapArray) == 0)
    {
        xil_printf("ReplayWave: ERROR Map is empty, no channel mapping.\r\n");
        return "NoChannelMap";
    }
    const char *mapErr = ReplayWave_BuildMapping(mapArray);
    if (mapErr)
        return mapErr;

    /* ---- 7. 扫描实际数据峰值 ---- */
    ReplayWave_ScanPeak();

    /* ---- 8. 量程检查 (用实际峰值，超出当前量程则报错) ---- */
    const char *rangeErr = ReplayWave_CheckRange();
    if (rangeErr)
        return rangeErr;

    /* ---- 9. 满幅缩放 (就地覆写.dat, 调整a系数) ---- */
    ReplayWave_RescaleData();

    /* ---- 10. 解析循环配置 ---- */
    ReplayWave_ParseRepeat(data);

    /* ---- 11. 计算区域边界 ---- */
    ReplayWave_CalcRegions();

    /* ---- 12. 解析DO配置 ---- */
    cJSON *doArray = cJSON_GetObjectItem(data, "DO");
    ReplayWave_ParseDO(doArray);

    /* ---- 13. 解析录波配置 ---- */
    cJSON *recRange = cJSON_GetObjectItem(data, "RecRange");
    cfg->recConfig.recRange = recRange ? recRange->valueint : 0;
    cfg->recConfig.recSamp = REPLAY_SAMPLE_RATE;

    /* ---- 14. 标记预处理完成 ---- */
    cfg->parasCompleted = true;

    xil_printf("ReplayWave: Paras OK. analog=%d, samples=%u, recordSize=%u, maps=%d\r\n",
               cfg->analogCount, cfg->totalSamples, cfg->recordSize, cfg->channelMapCount);

    return NULL; /* 成功 */
}

/* ================================================================
 *  核心数据变换: 从.dat读取一个DMA块并转换到tx_buffer
 *
 *  fileSampleStart: 文件中的起始采样点索引
 * ================================================================ */
static void read_and_transform_block(u32 fileSampleStart)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    u8 *datBase = (u8 *)REPLAY_DAT_ADDR;
    u32 *pTxBuf = (u32 *)tx_buffer_ptr;

    for (int i = 0; i < REPLAY_CYCLE_LEN; i++)
    {
        u32 sampleIdx = fileSampleStart + i;

        /* 8通道DAC值初始化为中值 (0输出, 与ADDA.c一致: 32767) */
        u16 dacVals[8] = {32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767};

        /* 对每个映射的通道进行AX+B变换 */
        for (int m = 0; m < cfg->channelMapCount; m++)
        {
            ReplayChannelMap_t *map = &cfg->channelMap[m];

            /* 处理通道延时 */
            int effectiveIdx = (int)sampleIdx - map->delaySamples;
            if (effectiveIdx < 0)
            {
                /* 用首周波尾部数据补充 (保证波形连续) */
                effectiveIdx = REPLAY_CYCLE_LEN + effectiveIdx;
                if (effectiveIdx < 0)
                    effectiveIdx = 0;
            }

            /* 钳位到有效范围 */
            if ((u32)effectiveIdx >= cfg->totalSamples)
            {
                effectiveIdx = (int)(cfg->totalSamples - 1);
            }

            /* 定位到.dat文件中的记录 */
            u8 *pRecord = datBase + (u32)effectiveIdx * cfg->recordSize;

            /* 跳过: 4字节序号 + 4字节时间戳 → 到达模拟通道数据 */
            int16_t *pAnalog = (int16_t *)(pRecord + 8);

            /* 读取映射的文件通道原始值 (mapChn 是1-based) */
            int16_t rawVal = pAnalog[map->mapChn - 1];

            /* AX+B → 有符号浮点 (与 ADDA.c 公式一致: val * 32768 + 32767) */
            int32_t dacCode = (int32_t)(rawVal * map->A_final + map->B_final);

            /* 转为无符号并钳位 (中值32767, 与 ADDA.c Write_Wave_to_Wave_NewData 一致) */
            int32_t dacU16 = dacCode + 32767;
            if (dacU16 > 65535)
                dacU16 = 65535;
            if (dacU16 < 0)
                dacU16 = 0;
            dacVals[map->hwIndex] = (u16)dacU16;
        }

        /* 打包为128bit格式 (与BRAM/FIFO格式一致) */
        /* Word[0] = UB|UA, Word[1] = UX|UC, Word[2] = IB|IA, Word[3] = IX|IC */
        int dst = i * 4;                                        /* 每个采样点4个u32 */
        pTxBuf[dst + 0] = ((u32)dacVals[1] << 16) | dacVals[0]; /* UB|UA */
        pTxBuf[dst + 1] = ((u32)dacVals[3] << 16) | dacVals[2]; /* UX|UC */
        pTxBuf[dst + 2] = ((u32)dacVals[5] << 16) | dacVals[4]; /* IB|IA */
        pTxBuf[dst + 3] = ((u32)dacVals[7] << 16) | dacVals[6]; /* IX|IC */
    }
}

/* ================================================================
 *  读取.dat中某采样点的数字通道值
 *  返回: 第 digChn (1-based) 个数字通道的值 (0 或 1)
 * ================================================================ */
static int Read_DigitalChannel(u32 sampleIdx, int digChn)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    if (digChn < 1 || digChn > cfg->digitalCount)
        return 0;
    if (sampleIdx >= cfg->totalSamples)
        return 0;

    u8 *datBase = (u8 *)REPLAY_DAT_ADDR;
    u8 *pRecord = datBase + sampleIdx * cfg->recordSize;

    /* 数字通道数据位于模拟通道之后 */
    u16 *pDigital = (u16 *)(pRecord + 8 + cfg->analogCount * 2);

    int groupIdx = (digChn - 1) / 16;
    int bitIdx = (digChn - 1) % 16;

    return (pDigital[groupIdx] >> bitIdx) & 1;
}

/* ================================================================
 *  区域状态机：前进到下一个DMA块
 *
 *  返回: 下一个DMA块应读取的文件起始采样点索引
 *        如果回放结束，设置 isFinished
 * ================================================================ */
static u32 Advance_And_GetSource(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;
    u32 src = 0;

    switch (rt->region)
    {

    case REPLAY_FIRST_CYCLE:
        src = 0; /* 读取文件第0~1023采样点 */
        /* 首周波输出后，转入下一区域 */
        if (cfg->repeatPrev.repeatCount > 0)
        {
            rt->region = REPLAY_PREV_LOOP;
            rt->regionRepeatLeft = cfg->repeatPrev.repeatCount;
        }
        else
        {
            rt->region = REPLAY_MAIN_PRE;
            rt->fileSamplePos = cfg->firstCycleEnd; /* 从第2周波开始 */
            /* 记录 PreEndTime */
            if (!rt->preEndTimeRecorded)
            {
                Get_TimeStr(rt->preEndTimeStr, sizeof(rt->preEndTimeStr));
                rt->preEndTimeRecorded = true;
                rt->preEndTimestampUs = Get_TimestampUs();
                rt->reportPending = true;
                strcpy(rt->reportResult, "Doing");
            }
        }
        break;

    case REPLAY_PREV_LOOP:
        src = 0; /* 重复首周波 */
        rt->regionRepeatLeft--;

        /* 检查是否被DI中断提前退出 (标志由 OnDIChange 设置) */
        if (rt->prevBreakTriggered || rt->regionRepeatLeft <= 0)
        {
            rt->region = REPLAY_MAIN_PRE;
            rt->fileSamplePos = cfg->firstCycleEnd;
            if (!rt->preEndTimeRecorded)
            {
                Get_TimeStr(rt->preEndTimeStr, sizeof(rt->preEndTimeStr));
                rt->preEndTimeRecorded = true;
                rt->preEndTimestampUs = Get_TimestampUs();
                rt->reportPending = true;
                strcpy(rt->reportResult, "Doing");
            }
        }
        break;

    case REPLAY_MAIN_PRE:
        src = rt->fileSamplePos;
        rt->fileSamplePos += REPLAY_CYCLE_LEN;

        /* 检查是否到达原始触发点 → 记录时间 */
        if (!rt->rawTrigTimeRecorded &&
            src <= cfg->rawTrigSample &&
            rt->fileSamplePos > cfg->rawTrigSample)
        {
            Get_TimeStr(rt->rawTrigTimeStr, sizeof(rt->rawTrigTimeStr));
            rt->rawTrigTimeRecorded = true;
            rt->reportPending = true;
            strcpy(rt->reportResult, "Doing");
        }

        /* 检查是否到达中段循环起点 */
        if (cfg->repeatMiddle.repeatCount > 0 &&
            rt->fileSamplePos >= cfg->repeatMiddle.startSample)
        {
            rt->region = REPLAY_MIDDLE_LOOP;
            rt->middleLoopPos = cfg->repeatMiddle.startSample;
            rt->regionRepeatLeft = cfg->repeatMiddle.repeatCount;
        }
        /* 或直接到达后段 */
        else if (rt->fileSamplePos >= cfg->lastCycleStart)
        {
            if (cfg->repeatBack.repeatCount > 0)
            {
                rt->region = REPLAY_BACK_LOOP;
                rt->regionRepeatLeft = cfg->repeatBack.repeatCount;
                /* 记录 BackStartTime */
                if (!rt->backStartTimeRecorded)
                {
                    Get_TimeStr(rt->backStartTimeStr, sizeof(rt->backStartTimeStr));
                    rt->backStartTimeRecorded = true;
                    rt->reportPending = true;
                    strcpy(rt->reportResult, "Doing");
                }
            }
            else
            {
                rt->region = REPLAY_HOLDING;
            }
        }
        break;

    case REPLAY_MIDDLE_LOOP:
        src = rt->middleLoopPos;
        rt->middleLoopPos += REPLAY_CYCLE_LEN;

        /* 检查触发点 */
        if (!rt->rawTrigTimeRecorded &&
            src <= cfg->rawTrigSample &&
            rt->middleLoopPos > cfg->rawTrigSample)
        {
            Get_TimeStr(rt->rawTrigTimeStr, sizeof(rt->rawTrigTimeStr));
            rt->rawTrigTimeRecorded = true;
            rt->reportPending = true;
            strcpy(rt->reportResult, "Doing");
        }

        /* 检查是否到达中段循环末尾 */
        if (rt->middleLoopPos >= cfg->repeatMiddle.endSample)
        {
            rt->regionRepeatLeft--;
            if (rt->regionRepeatLeft > 0)
            {
                /* 再来一轮 */
                rt->middleLoopPos = cfg->repeatMiddle.startSample;
            }
            else
            {
                /* 中段循环结束 → 主数据后段 */
                rt->region = REPLAY_MAIN_POST;
                rt->fileSamplePos = cfg->repeatMiddle.endSample;
            }
        }
        break;

    case REPLAY_MAIN_POST:
        src = rt->fileSamplePos;
        rt->fileSamplePos += REPLAY_CYCLE_LEN;

        /* 检查触发点 */
        if (!rt->rawTrigTimeRecorded &&
            src <= cfg->rawTrigSample &&
            rt->fileSamplePos > cfg->rawTrigSample)
        {
            Get_TimeStr(rt->rawTrigTimeStr, sizeof(rt->rawTrigTimeStr));
            rt->rawTrigTimeRecorded = true;
            rt->reportPending = true;
            strcpy(rt->reportResult, "Doing");
        }

        /* 检查是否到达尾周波 */
        if (rt->fileSamplePos >= cfg->lastCycleStart)
        {
            if (cfg->repeatBack.repeatCount > 0)
            {
                rt->region = REPLAY_BACK_LOOP;
                rt->regionRepeatLeft = cfg->repeatBack.repeatCount;
                /* 记录 BackStartTime */
                if (!rt->backStartTimeRecorded)
                {
                    Get_TimeStr(rt->backStartTimeStr, sizeof(rt->backStartTimeStr));
                    rt->backStartTimeRecorded = true;
                    rt->reportPending = true;
                    strcpy(rt->reportResult, "Doing");
                }
            }
            else
            {
                /* 无后导，直接结束 (但先输出最后一个周波) */
                if (rt->fileSamplePos >= cfg->totalSamples)
                {
                    rt->region = REPLAY_HOLDING;
                }
            }
        }
        break;

    case REPLAY_BACK_LOOP:
        src = cfg->lastCycleStart; /* 重复尾周波 */
        rt->regionRepeatLeft--;
        if (rt->regionRepeatLeft <= 0)
        {
            rt->region = REPLAY_HOLDING;
        }
        break;

    case REPLAY_HOLDING:
        /* 持续输出最后一个周波 */
        src = cfg->lastCycleStart;
        /* Success 只上报一次 */
        if (!rt->holdingReported)
        {
            rt->holdingReported = true;
            rt->reportPending = true;
            strcpy(rt->reportResult, "Success");
        }
        break;

    default:
        rt->region = REPLAY_HOLDING;
        src = cfg->lastCycleStart;
        break;
    }

    rt->totalBlocksPlayed++;
    return src;
}

/* ================================================================
 *  DO 输出初始化 (回放启动时调用)
 * ================================================================ */
/* DO 控制辅助: 设置单个DO通道 */
static void Replay_SetDO(int chn, int val)
{
    if (chn < 1 || chn > 8)
        return;
    int bit_position = 23 + chn;
    if (val)
    {
        g_do_output_state |= (1U << bit_position);
    }
    else
    {
        g_do_output_state &= ~(1U << bit_position);
    }
    OnOff_Write_Continuous(g_do_output_state);
}

/* DO 控制辅助: 读取单个DO通道当前值 */
static int Replay_ReadDO(int chn)
{
    if (chn < 1 || chn > 8)
        return 0;
    int bit_position = 23 + chn;
    return (g_do_output_state >> bit_position) & 1;
}

static void Init_DO_Outputs(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;

    for (int i = 0; i < cfg->doConfigCount; i++)
    {
        ReplayDO_Config_t *d = &cfg->doConfig[i];
        d->turnTriggered = false;
        d->turnRestored = false;
        d->prevDI_CS = 0;
        d->prevDI_OS = 0;

        int initVal = d->val;
        if (initVal == -1)
        {
            /* 保持回放前状态 — 读取当前DO值 */
            initVal = Replay_ReadDO(d->chn);
        }
        else if (initVal == -2)
        {
            /* 翻转回放前状态 */
            initVal = Replay_ReadDO(d->chn) ? 0 : 1;
        }

        d->currentVal = initVal;
        /* 写入DO (Keep/Turn/Breaker的初始值) */
        if (d->mode != DO_MODE_MAP)
        {
            Replay_SetDO(d->chn, d->currentVal);
        }
    }
}

/* ================================================================
 *  DO 输出更新 (主循环中调用)
 * ================================================================ */
static void Update_DO_Outputs(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;

    if (!rt->isRunning)
        return;

    u64 nowUs = Get_TimestampUs();

    for (int i = 0; i < cfg->doConfigCount; i++)
    {
        ReplayDO_Config_t *d = &cfg->doConfig[i];

        if (d->mode == DO_MODE_TURN && !d->turnTriggered)
        {
            /* 计算参考时间 */
            u64 refUs = rt->startTimestampUs;
            if (d->timeRef == DO_TIMEREF_REPEATPREVEND && rt->preEndTimeRecorded)
            {
                refUs = rt->preEndTimestampUs;
            }

            u64 turnUs = refUs + (u64)(d->delayMS * 1000.0);
            if (nowUs >= turnUs)
            {
                /* 翻转 */
                d->currentVal = d->currentVal ? 0 : 1;
                Replay_SetDO(d->chn, d->currentVal);
                d->turnTriggered = true;
            }
        }

        if (d->mode == DO_MODE_TURN && d->turnTriggered && !d->turnRestored && d->holdMS > 0)
        {
            u64 refUs = rt->startTimestampUs;
            if (d->timeRef == DO_TIMEREF_REPEATPREVEND && rt->preEndTimeRecorded)
            {
                refUs = rt->preEndTimestampUs;
            }
            u64 restoreUs = refUs + (u64)((d->delayMS + d->holdMS) * 1000.0);
            if (nowUs >= restoreUs)
            {
                d->currentVal = d->currentVal ? 0 : 1;
                Replay_SetDO(d->chn, d->currentVal);
                d->turnRestored = true;
            }
        }

        if (d->mode == DO_MODE_MAP && rt->isRunning)
        {
            /* 从当前播放位置读取数字通道值 */
            u32 curSample = rt->fileSamplePos > 0 ? rt->fileSamplePos - REPLAY_CYCLE_LEN : 0;
            int digVal = Read_DigitalChannel(curSample, d->mapChn);
            if (digVal != d->currentVal)
            {
                d->currentVal = digVal;
                Replay_SetDO(d->chn, d->currentVal);
            }
        }
    }
}

/* ================================================================
 *  DI 变化回调: 用于前导退出判断 + Breaker DO 模式
 * ================================================================ */
void ReplayWave_OnDIChange(u32 diCurrentVal)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;

    if (!rt->isRunning)
        return;

    /* ---- 前导循环 DI 提前退出检查 ---- */
    if (rt->region == REPLAY_PREV_LOOP &&
        cfg->repeatPrev.breakEnable &&
        !rt->prevBreakTriggered)
    {

        bool condMet = cfg->repeatPrev.trigLogicAnd; /* AND起始为true, OR起始为false */

        for (int i = 0; i < cfg->repeatPrev.trigCount; i++)
        {
            ReplayDI_Trig_t *trig = &cfg->repeatPrev.trigDIs[i];
            if (trig->val == -1)
                continue; /* 忽略 */

            int diVal = (diCurrentVal >> (trig->chn - 1)) & 1;
            bool match = (diVal == trig->val);

            if (cfg->repeatPrev.trigLogicAnd)
            {
                condMet = condMet && match;
            }
            else
            {
                condMet = condMet || match;
            }
        }

        if (condMet)
        {
            rt->prevBreakTriggered = true;
            /* 记录 DITrigTime */
            if (!rt->diTrigTimeRecorded)
            {
                Get_TimeStr(rt->diTrigTimeStr, sizeof(rt->diTrigTimeStr));
                rt->diTrigTimeRecorded = true;
            }
            // xil_printf("ReplayWave: [DEBUG] Early break triggered by DI! (val=0x%02X)\r\n", diCurrentVal);
        }
    }

    /* ---- Breaker DO 模式: 检测DI上升沿 ---- */
    for (int i = 0; i < cfg->doConfigCount; i++)
    {
        ReplayDO_Config_t *d = &cfg->doConfig[i];
        if (d->mode != DO_MODE_BREAKER)
            continue;

        /* 合闸DI上升沿检测 */
        if (d->diCSChn > 0)
        {
            u32 csNow = (diCurrentVal >> (d->diCSChn - 1)) & 1;
            if (csNow && !d->prevDI_CS)
            {
                /* 上升沿 → 合闸 (置1) */
                if (d->currentVal != 1)
                {
                    d->currentVal = 1;
                    Replay_SetDO(d->chn, 1); /* TODO: 加延时支持 */
                }
            }
            d->prevDI_CS = csNow;
        }

        /* 分闸DI上升沿检测 */
        if (d->diOSChn > 0)
        {
            u32 osNow = (diCurrentVal >> (d->diOSChn - 1)) & 1;
            if (osNow && !d->prevDI_OS)
            {
                /* 上升沿 → 分闸 (置0) */
                if (d->currentVal != 0)
                {
                    d->currentVal = 0;
                    Replay_SetDO(d->chn, 0); /* TODO: 加延时支持 */
                }
            }
            d->prevDI_OS = osNow;
        }
    }
}

/* ================================================================
 *  FIFO prog_empty 中断回调 — 播放引擎核心
 *  由 underflow_handler() 调用
 * ================================================================ */
void ReplayWave_FeedNext(void)
{
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;
    if (!rt->isRunning)
        return;

    /* 1. 获取下一个DMA块的源文件位置 */
    u32 srcSample = Advance_And_GetSource();

    /* 2. 如果回放已停止 (手动Stop), 不再喂数据 */
    if (!rt->isRunning)
        return;

    /* 3. 从.dat读取并AX+B变换到tx_buffer */
    read_and_transform_block(srcSample);

    /* 4. 刷新Cache并启动DMA搬运到FIFO */
    Xil_DCacheFlushRange((UINTPTR)tx_buffer_ptr, REPLAY_DMA_BLOCK_BYTES);
    XAxiDma_SimpleTransfer(&axidma, (UINTPTR)tx_buffer_ptr,
                           REPLAY_DMA_BLOCK_BYTES, XAXIDMA_DMA_TO_DEVICE);
}

/* ================================================================
 * 实际启动回放
 * ================================================================ */
static void ReplayWave_DoStart(void)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;

    xil_printf("ReplayWave: Preparing hardware for playback...\r\n");

    /* 1. 软件状态及时间戳初始化*/
    Get_TimeStr(rt->startTimeStr, sizeof(rt->startTimeStr));
    rt->startTimestampUs = Get_TimestampUs();
    rt->startTimeRecorded = true;
    rt->region = REPLAY_FIRST_CYCLE;
    rt->fileSamplePos = 0;
    rt->isRunning = true;
    rt->isWaiting = false;

    /* 2. 初始化DO输出 */
    Init_DO_Outputs();

    /* 3. 在切模式前预填充 FIFO */
    /* 预填充第1个DMA块 (首周波) */
    u32 src1 = Advance_And_GetSource();
    read_and_transform_block(src1);
    Xil_DCacheFlushRange((UINTPTR)tx_buffer_ptr, REPLAY_DMA_BLOCK_BYTES);

    /* 启动DMA搬运：把首周波数据推入 FIFO */
    XAxiDma_SimpleTransfer(&axidma, (UINTPTR)tx_buffer_ptr,
                           REPLAY_DMA_BLOCK_BYTES, XAXIDMA_DMA_TO_DEVICE);

    /* 必须死等搬运完成！确保 FIFO 内部已经有了正确的起点数据 (通常是 0x8000) */
    while (XAxiDma_Busy(&axidma, XAXIDMA_DMA_TO_DEVICE))
        ;

    /* 预填充第2个DMA块 (双缓冲保证连续性) */
    u32 src2 = Advance_And_GetSource();
    if (rt->isRunning)
    {
        read_and_transform_block(src2);
        Xil_DCacheFlushRange((UINTPTR)tx_buffer_ptr, REPLAY_DMA_BLOCK_BYTES);
        XAxiDma_SimpleTransfer(&axidma, (UINTPTR)tx_buffer_ptr,
                               REPLAY_DMA_BLOCK_BYTES, XAXIDMA_DMA_TO_DEVICE);
        while (XAxiDma_Busy(&axidma, XAXIDMA_DMA_TO_DEVICE))
            ;
    }

    /* 4. 配置硬件参数寄存器 (先写辅助参数，不切主开关) */
    Xil_Out32(dac_whole_base_addr + 4, (u32)(REPLAY_FREQ_DIV) << 16);
    Xil_Out32(dac_whole_base_addr + 8, (u32)(0xFF) << 16);

    /* 5. 提前开启中断 */
    /* 此时 FIFO 满了，不会立即触发 underflow，等硬件切模式后消耗了数据才会触发 */
    XIntc_Enable(&AxiIntc_BareMetal, REPLAY_FIFO_INTR_ID);

    /* 6. 最后拨动模式开关 (bit 16) */
    Xil_Out32(dac_whole_base_addr + 0, 0x00010000U);

    /* 7. 功放开启移至最后 */
    /* 确保 DAC 硬件输出已经稳定在回放数据的起始电平后，再合上功放 */
    {
        float replay_amp[8] = {100, 100, 100, 100, 100, 100, 100, 100};
        power_amplifier_control(replay_amp, Wave_Range, PID_OFF, POWAMP_ON);
        xil_printf("ReplayWave: Power amplifier enabled (Safe Boot).\r\n");
    }

    /* 8. 录波及状态上报 */
    if (cfg->recConfig.recRange == 2)
    {
        WaveRecord_Start(0, "SetTaskWaveReplayStart");
        rt->recordingStarted = true;
    }

    rt->reportPending = true;
    strcpy(rt->reportResult, "Doing");

    xil_printf("ReplayWave: Playback started. DA DMA mode enabled.\r\n");
}

/* ================================================================
 *  设置软时钟闹钟 (定时启动)
 *  @param startTimeStr 格式: "2026-05-08 10:35:00.000"
 *  @return 0=成功, -1=参数错误
 * ================================================================ */
static int ReplayWave_EnableAlarm(const char *startTimeStr)
{
    if (!startTimeStr || strlen(startTimeStr) < 19)
        return -1;

    /* 解析时分秒 (位置: [11:12] [14:15] [17:18]) */
    char h_str[3] = {startTimeStr[11], startTimeStr[12], '\0'};
    char m_str[3] = {startTimeStr[14], startTimeStr[15], '\0'};
    char s_str[3] = {startTimeStr[17], startTimeStr[18], '\0'};
    int hour = atoi(h_str);
    int min = atoi(m_str);
    int sec = atoi(s_str);

    xil_printf("ReplayWave: Scheduling Alarm at %02d:%02d:%02d\r\n", hour, min, sec);

    uint32_t bcd_h = ((hour / 10) << 4) | (hour % 10);
    uint32_t bcd_m = ((min / 10) << 4) | (min % 10);
    uint32_t bcd_s = ((sec / 10) << 4) | (sec % 10);

    /* 配置闹钟寄存器 (保留 RdSerial 使能位) */
    g_SoftTimer_Reg15_Shadow &= STIMER_RDSERIAL_EN_MASK;
    g_SoftTimer_Reg15_Shadow |= STIMER_ALARM_EN_MASK;
    g_SoftTimer_Reg15_Shadow |= (bcd_h << STIMER_ALARM_HOUR_SHIFT);
    g_SoftTimer_Reg15_Shadow |= (bcd_m << STIMER_ALARM_MIN_SHIFT);
    g_SoftTimer_Reg15_Shadow |= (bcd_s << STIMER_ALARM_SEC_SHIFT);

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG15, g_SoftTimer_Reg15_Shadow);

    return 0;
}

/* ================================================================
 *  闹钟中断回调 (由 SoftTimer_AlarmHandler 分发调用)
 * ================================================================ */
void ReplayWave_OnAlarmIRQ(void)
{
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;

    if (!rt->isWaiting)
        return; /* 不是我们的闹钟 */

    xil_printf("CPU1: [IRQ] ReplayWave Triggered by Alarm!\r\n");
    ReplayWave_DoStart();
}

/* ================================================================
 *  SetTaskWaveReplayStart 处理
 *  返回值: NULL=成功, 非NULL=错误信息
 * ================================================================ */
const char *ReplayWave_HandleStart(cJSON *data)
{
    ReplayWave_Config_t *cfg = &g_ReplayConfig;
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;

    if (!cfg->parasCompleted)
    {
        return "ParasNotCompleted";
    }

    /* 如果已在运行，先停止 */
    if (rt->isRunning)
    {
        ReplayWave_Stop();
    }

    /* 重置运行时状态 */
    memset(rt, 0, sizeof(*rt));

    /* 检查启动模式 */
    int startMode = 0;
    cJSON *modeItem = cJSON_GetObjectItem(data, "StartMode");
    if (modeItem)
        startMode = modeItem->valueint;

    if (startMode == 1)
    {
        /* ---- 定时启动 ---- */
        cJSON *timeItem = cJSON_GetObjectItem(data, "StartTime");
        if (!timeItem || !timeItem->valuestring)
        {
            return "StartTimeMissing";
        }

        /* 设置闹钟 */
        if (ReplayWave_EnableAlarm(timeItem->valuestring) != 0)
        {
            return "StartTimeInvalid";
        }

        /* 进入等待状态 (main.c 互斥会保护 DAC) */
        rt->isWaiting = true;
        xil_printf("ReplayWave: Waiting for timed start...\r\n");
        return NULL; /* 返回 Success，等待闹钟中断 */
    }

    /* ---- 立即启动 ---- */
    ReplayWave_DoStart();
    return NULL;
}

/* ================================================================
 *  停止回放
 * ================================================================ */
void ReplayWave_Stop(void)
{
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;

    /* 禁用FIFO prog_empty中断 */
    XIntc_Disable(&AxiIntc_BareMetal, REPLAY_FIFO_INTR_ID);

    /* 关闭DMA模式 */
    Xil_Out32(dac_whole_base_addr + 0, 0x00000000U);

    /* 停止录波 */
    if (rt->recordingStarted)
    {
        WaveRecord_Stop();
        rt->recordingStarted = false;
    }

    rt->isRunning = false;

    /* 如果在等待定时启动，取消闹钟 */
    if (rt->isWaiting)
    {
        g_SoftTimer_Reg15_Shadow &= ~STIMER_ALARM_EN_MASK;
        Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG15, g_SoftTimer_Reg15_Shadow);
        xil_printf("ReplayWave: Alarm cancelled.\r\n");
    }
    rt->isWaiting = false;

    xil_printf("ReplayWave: Playback stopped.\r\n");
}

/* ================================================================
 *  主循环检查与TaskEvent上报
 * ================================================================ */
void ReplayWave_CheckAndReport(void)
{
    ReplayWave_Runtime_t *rt = &g_ReplayRuntime;
    ReplayWave_Config_t *cfg = &g_ReplayConfig;

    /* 更新DO输出 (Turn模式计时, Map模式更新) */
    if (rt->isRunning)
    {
        Update_DO_Outputs();

        /* RecRange=1: 前导结束后启动录波，必须确保未进入后导或结束状态 */
        if (cfg->recConfig.recRange == 1 && !rt->recordingStarted && rt->preEndTimeRecorded && !rt->holdingReported && !rt->backStartTimeRecorded)
        {
            WaveRecord_Start(0, "SetTaskWaveReplayStart");
            rt->recordingStarted = true;
        }
    }

    /* 检查是否有待上报的TaskEvent */
    if (!rt->reportPending)
        return;
    rt->reportPending = false;

    /* 构建 TaskEvent */
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "TaskEvent");
    cJSON_AddStringToObject(report, "FunCode", "SetTaskWaveReplayStart");
    cJSON_AddStringToObject(report, "Result", rt->reportResult);

    cJSON *rdata = cJSON_CreateObject();

    if (rt->startTimeRecorded)
        cJSON_AddStringToObject(rdata, "StartTime", rt->startTimeStr);
    if (rt->diTrigTimeRecorded)
        cJSON_AddStringToObject(rdata, "DITrigTime", rt->diTrigTimeStr);
    else
        cJSON_AddStringToObject(rdata, "DITrigTime", "");
    if (rt->preEndTimeRecorded)
        cJSON_AddStringToObject(rdata, "PreEndTime", rt->preEndTimeStr);
    if (rt->rawTrigTimeRecorded)
        cJSON_AddStringToObject(rdata, "RawTrigTime", rt->rawTrigTimeStr);
    if (rt->backStartTimeRecorded)
        cJSON_AddStringToObject(rdata, "BackStartTime", rt->backStartTimeStr);

    cJSON_AddItemToObject(report, "Data", rdata);
    Send_JSON(report);

    /* 回放完成后的一次性处理 (停录波、打印日志) */
    /* 如果是 RecRange == 1，遇到后导起点 (backStartTimeRecorded) 即可提前停录 */
    bool shouldStopRecord = (rt->holdingReported) || (cfg->recConfig.recRange == 1 && rt->backStartTimeRecorded);

    if (shouldStopRecord && rt->recordingStarted)
    {
        WaveRecord_Stop();
        rt->recordingStarted = false;
        xil_printf("ReplayWave: Playback finished or entered BackLoop. Stopping record.\r\n");
    }
}
