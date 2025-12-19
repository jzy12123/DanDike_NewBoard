#ifndef WAVE_RECORD_H
#define WAVE_RECORD_H

#include "xil_types.h"
#include "stdbool.h"
#include "Communications_Protocol.h"
#include "Amplifier_Switch.h"
#include "soft_timer.h"

// 录波配置
#define REC_SHARE_BASE 0x3C000000 // 共享内存起始地址
#define REC_TOTAL_SIZE 0x04000000 // 总空间 64MB
#define CFG_SIZE_LIMIT 4096       // 预留 4KB 给 CFG 文件
#define COMTRADE_FRAME_SIZE 24     // 4(Seq) + 4(Time) + 16(8路模拟量) =  24 字节
// CFG 文件存放地址：64MB 空间的最后 4KB
#define REC_CFG_ADDR (REC_SHARE_BASE + REC_TOTAL_SIZE - CFG_SIZE_LIMIT)
// DAT 数据最大允许大小
#define REC_DAT_MAX_SIZE (REC_TOTAL_SIZE - CFG_SIZE_LIMIT)
// 录波控制结构体
typedef struct
{
    volatile bool isRecording;   // 录波开关 (加 volatile)
    volatile bool isPendingSave; // 【新增】标记是否需要生成文件 (加 volatile)
    bool isFirstBlock;     // 【新增】标记是否是启动后的第一块数据
    u64 startTimeUs;       // 【新增】记录启动时刻的微秒时间戳
    u32 recordedBytes;     // 已记录字节数
    u32 maxRecordBytes;    // 目标字节数 (受 REC_DAT_MAX_SIZE 限制)
    u32 sampleSequence;    // 采样点序号
    u32 writeAddrOffset;   // 当前写入偏移
    char startTimeStr[32]; // 录波启动时间字符串 (用于CFG)
    // 上报所需信息
    char recFrom[32];           // 录波来源 (如 "SetTaskStateSequence")
    int targetDurationMs;       // 设定时长
    In_CurrTime startTimestamp; // 启动时的详细时间 (用于生成文件名)
    u32 cfgFileSize;            // CFG文件大小 (用于上报)
} WaveRecord_Ctrl_t;

extern WaveRecord_Ctrl_t g_WaveRecordCtrl;

// 函数声明
void WaveRecord_Init(void);
void WaveRecord_Start(u32 duration_ms, const char *source);
void WaveRecord_Stop(void);
void WaveRecord_Process(u16 *pRawData, int points_count);
void Generate_Comtrade_CFG(void);
// 任务轮询 (仅用于监控结束和上报 TaskEvent)
void WaveRecordTask_Check(void);

// 【新增】设置软时钟闹钟 (供 Cmd 调用)
int WaveRecord_EnableAlarm(const char *startTimeStr);

// 【新增】定时中断响应函数 (供 SoftTimer_AlarmHandler 调用)
void WaveRecord_OnAlarmIRQ(void);

// 【新增】DI中断响应函数 (供 debounce_timer_handler 调用)
void WaveRecord_OnDIIRQ(uint32_t current_val);

#endif