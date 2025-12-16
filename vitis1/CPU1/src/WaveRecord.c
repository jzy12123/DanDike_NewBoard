#include "WaveRecord.h"
#include "ADDA.h" // 获取 FS_RATE, CHN_NUM
#include "xil_cache.h"
#include "xil_printf.h"
#include <string.h>

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

    // 计算最大字节数 (RecMS <= 0 表示录直到满)
    if (duration_ms <= 0)
    {
        g_WaveRecordCtrl.maxRecordBytes = REC_MAX_SIZE;
    }
    else
    {
        u64 bytes = (u64)duration_ms * FS_RATE / 1000 * COMTRADE_FRAME_SIZE;
        if (bytes > REC_MAX_SIZE)
            bytes = REC_MAX_SIZE;
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
        Generate_Comtrade_CFG();
        xil_printf("CPU1: WaveRecord Stopped. Total: %u bytes\r\n", g_WaveRecordCtrl.recordedBytes);
        // 这里可以发送 Report.WaveRecordComplete
    }
}

/**
 * @brief 处理一块数据 (500ms) 进行录波存储
 */
void WaveRecord_Process(u16 *pRawData, int points_count)
{
    if (!g_WaveRecordCtrl.isRecording)
        return;

    // 检查剩余空间
    if (g_WaveRecordCtrl.recordedBytes >= g_WaveRecordCtrl.maxRecordBytes)
    {
        WaveRecord_Stop();
        return;
    }

    u32 *pDest = (u32 *)(UINTPTR)(REC_SHARE_BASE + g_WaveRecordCtrl.writeAddrOffset);
    u32 current_block_size = 0;

    for (int i = 0; i < points_count; i++)
    {
        // 1. 序号
        *pDest++ = g_WaveRecordCtrl.sampleSequence++;

        // 2. 时间 (us)
        // 简单计算: seq * 1000000 / 51200
        // 为避免浮点，可用 (seq * 19531) >> 10 (约等于 seq * 19.07, 需更精确)
        // 这里先用浮点保证精度
        u32 time_us = (u32)((double)g_WaveRecordCtrl.sampleSequence * 1000000.0 / FS_RATE);
        *pDest++ = time_us;

        // 3. 8通道数据 (合并为4个u32)
        // pRawData 排列: [Ch0][Ch1]...[Ch7]
        u16 *pSamp = &pRawData[i * CHN_NUM];
        *pDest++ = (u32)pSamp[0] | ((u32)pSamp[1] << 16);
        *pDest++ = (u32)pSamp[2] | ((u32)pSamp[3] << 16);
        *pDest++ = (u32)pSamp[4] | ((u32)pSamp[5] << 16);
        *pDest++ = (u32)pSamp[6] | ((u32)pSamp[7] << 16);

        current_block_size += COMTRADE_FRAME_SIZE;
        g_WaveRecordCtrl.writeAddrOffset += COMTRADE_FRAME_SIZE;

        if (g_WaveRecordCtrl.writeAddrOffset >= REC_MAX_SIZE ||
            g_WaveRecordCtrl.writeAddrOffset >= g_WaveRecordCtrl.maxRecordBytes)
        {
            break;
        }
    }

    g_WaveRecordCtrl.recordedBytes += current_block_size;

    // 刷新Cache到DDR
    Xil_DCacheFlushRange(REC_SHARE_BASE + g_WaveRecordCtrl.writeAddrOffset - current_block_size,
                         current_block_size);

    if (g_WaveRecordCtrl.recordedBytes >= g_WaveRecordCtrl.maxRecordBytes)
    {
        WaveRecord_Stop();
    }
}

void Generate_Comtrade_CFG(void)
{
    // 这里生成 ASCII 格式的 .cfg 文件内容并写入共享内存的特定区域
    // 供 Linux 读取。具体格式参考 Comtrade 标准。
    // 示例： "StationName,DevId,1999" ...
}