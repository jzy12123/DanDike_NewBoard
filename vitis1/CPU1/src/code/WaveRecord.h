// [code/WaveRecord.h]

#ifndef WAVE_RECORD_H
#define WAVE_RECORD_H

#include "xil_types.h"
#include "stdbool.h"
#include "Communications_Protocol.h"
#include "Amplifier_Switch.h"
#include "soft_timer.h"

// ================= 录波内存布局配置 =================
// 总空间 80MB (0x05000000)
#define REC_SHARE_BASE 0x3B000000
#define REC_TOTAL_SIZE 0x05000000

// 分为两段，每段 40MB (0x02800000)
#define REC_SECTION_SIZE 0x02800000

// 第一段定义
#define REC_SEC1_START REC_SHARE_BASE
#define REC_SEC1_END (REC_SEC1_START + REC_SECTION_SIZE)
#define REC_SEC1_CFG (REC_SEC1_END - 4096) // CFG 放在第一段末尾 4KB

// 第二段定义
#define REC_SEC2_START (REC_SHARE_BASE + REC_SECTION_SIZE)
#define REC_SEC2_END (REC_SEC2_START + REC_SECTION_SIZE)
#define REC_SEC2_CFG (REC_SEC2_END - 4096) // CFG 放在第二段末尾 4KB

// 30秒数据的字节数限制 (51200 * 24 * 30 = 36,864,000 字节)
// 预留一些余量，比如严格按30秒切分
#define REC_CHUNK_TIME_MS 30000

// COMTRADE 帧大小
#define COMTRADE_FRAME_SIZE 24

// 录波控制结构体
typedef struct
{
    // --- 状态标志 ---
    volatile bool isRecording;
    volatile bool isPendingStartReport; // 启动报告挂起
    volatile bool isStopping;           // 【新增】软停止请求标志
    volatile bool isTaskFinished;       // 任务整体完成标志 (用于发 TaskEvent Success)

    // --- 双缓冲控制 ---
    volatile u8 currentSection;       // 当前正在写入的段 (1 或 2)
    volatile u8 pendingReportSection; // 待上报的段 (0=无, 1=Sec1, 2=Sec2)

    // --- 实时运行参数 ---
    bool isFirstBlock;
    u64 startTimeUs;    // 整体任务启动的微秒时间戳 (用于裁切第一块)
    u32 sampleSequence; // 全局采样点序号 (连续累加)

    u32 bytesWrittenInSection; // 当前段已写入字节数
    u32 bytesLimitPerSection;  // 每段的目标字节数 (30s)

    // --- 任务参数 ---
    char recFrom[32];
    int totalTargetDurationMs; // 总目标时长 (-1 为无限)
    u32 totalRecordedBytes;    // 总记录字节数 (用于判断总时长)

    // --- 快照数据 (用于生成报告，避免竞争) ---
    // 当一段录满切换时，将该段的信息存入这里供主循环上报
    struct
    {
        char startTimeStr[32]; // 该段的起始时间字符串
        u32 dataSize;          // 该段的数据大小
        u32 cfgSize;           // 该段 CFG 大小
        u32 startSeq;          // 该段起始序号
        In_CurrTime startTime; // 该段起始时间结构体 (用于文件名)
    } snapshot;

    // 当前正在记录段的起始时间 (用于计算 snapshot)
    In_CurrTime currentSectionStartTime;

} WaveRecord_Ctrl_t;

extern WaveRecord_Ctrl_t g_WaveRecordCtrl;

// 函数声明
void WaveRecord_Init(void);
void WaveRecord_Stop(void);
void WaveRecord_Process(u16 *pRawData, int points_count);
void WaveRecordTask_Check(void);

// 启动函数
void WaveRecord_Start(u32 duration_ms, const char *source);
void WaveRecord_OnDIIRQ(uint32_t current_val);
void WaveRecord_OnAlarmIRQ(void);
int WaveRecord_EnableAlarm(const char *startTimeStr);

// 内部辅助
void Generate_Comtrade_CFG(u8 section); // 增加参数：生成哪一段的CFG

#endif /* WAVE_RECORD_H */