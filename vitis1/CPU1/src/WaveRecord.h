#ifndef WAVE_RECORD_H
#define WAVE_RECORD_H

#include "xil_types.h"
#include "stdbool.h"

// 录波配置
#define REC_SHARE_BASE 0x3C000000 // 共享内存起始地址
#define REC_MAX_SIZE 0x04000000   // 64MB
#define COMTRADE_FRAME_SIZE 24    // 4(Seq) + 4(Time) + 16(Data)

// 录波控制结构体
typedef struct
{
    bool isRecording;    // 录波开关
    u32 recordedBytes;   // 已记录字节数
    u32 maxRecordBytes;  // 目标字节数
    u32 sampleSequence;  // 采样点序号
    u32 writeAddrOffset; // 当前写入偏移
    u32 startTimeMs;     // 启动时间 (可选)
} WaveRecord_Ctrl_t;

extern WaveRecord_Ctrl_t g_WaveRecordCtrl;

// 函数声明
void WaveRecord_Init(void);
void WaveRecord_Start(u32 duration_ms);
void WaveRecord_Stop(void);
void WaveRecord_Process(u16 *pRawData, int points_count);
void Generate_Comtrade_CFG(void);

#endif