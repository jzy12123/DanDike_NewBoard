/*
 * ReplayWave.h - 波形回放模块
 *
 * 功能：从共享DDR读取COMTRADE .dat数据，
 *       通过AX+B变换和通道映射，经DMA→FIFO→DA IP核输出。
 *       支持前导/中段/后导循环、DO控制、同时录波。
 *
 * 数据通路：共享DDR(.dat) → CPU1计算AX+B → tx_buffer → AXI DMA → FIFO → DA输出
 */

#ifndef REPLAY_WAVE_H
#define REPLAY_WAVE_H

#include "stdbool.h"
#include "xil_types.h"
#include "xintc.h"

/* 前向声明 cJSON，避免循环依赖 */
struct cJSON;
typedef struct cJSON cJSON;

/* ================================================================
 *  共享内存地址定义 (与 linux/ReplayWave.c 一致)
 * ================================================================ */
#define REPLAY_SHM_BASE 0x34C00000U
#define REPLAY_SHM_SIZE 0x05000000U /* 80MB */
#define REPLAY_HEADER_SIZE 64U
#define REPLAY_DAT_ADDR (REPLAY_SHM_BASE + REPLAY_HEADER_SIZE) /* 0x34C00040 \
                                                                */
#define REPLAY_DAT_MAX_SIZE (REPLAY_SHM_SIZE - REPLAY_HEADER_SIZE)

/* 控制头 Magic */
#define REPLAY_MAGIC 0x52504C59U /* "RPLY" */

/* 控制头状态 */
#define REPLAY_STATUS_IDLE 0U
#define REPLAY_STATUS_WRITING 1U
#define REPLAY_STATUS_READY 2U

/* ================================================================
 *  回放常量
 * ================================================================ */
#define REPLAY_SAMPLE_RATE 51200                       /* 固定回放采样率 */
#define REPLAY_CYCLE_LEN 1024                          /* 每周波采样点 @51200Hz/50Hz */
#define REPLAY_DMA_BLOCK_BYTES (REPLAY_CYCLE_LEN * 16) /* 16384 字节/DMA块 */
#define REPLAY_FREQ_DIV 1953                           /* 100MHz / 51200 ≈ 1953 */

#define REPLAY_MAX_FILE_CHANNELS 32 /* CFG最大模拟通道数 */
#define REPLAY_MAX_DIGITAL_GROUPS 4 /* CFG最大数字通道组(每组16位) */
#define REPLAY_MAX_MAP_CHANNELS 8   /* 最大硬件映射通道 */
#define REPLAY_MAX_DO_CONFIG 8      /* 最大DO配置数 */
#define REPLAY_MAX_DI_TRIG 8        /* 前导退出最大DI触发数 */
#define REPLAY_WAVE_NAME_LEN 64

/* ================================================================
 *  共享内存控制头 (与Linux端结构体完全一致)
 * ================================================================ */
typedef struct __attribute__((packed))
{
    u32 magic;                               /* [0-3]   0x52504C59 */
    u32 datFileSize;                         /* [4-7]   .dat文件字节数 */
    u32 status;                              /* [8-11]  状态 */
    u32 reserved;                            /* [12-15] 保留 */
    char waveFileName[REPLAY_WAVE_NAME_LEN]; /* [16-79] 文件名(不含路径扩展名) */
} ReplayHeader_t;

/* ================================================================
 *  CFG 解析: 每个模拟通道信息
 * ================================================================ */
typedef struct
{
    int index;      /* 通道序号 (1-based) */
    char name[16];  /* 通道名称, 如 "IA" */
    char phase[4];  /* 相别 A/B/C/N */
    char unit[8];   /* 单位 V/A/kV/kA */
    double a;       /* COMTRADE乘数 */
    double b;       /* COMTRADE偏移 */
    int16_t minVal; /* 最小原始码值 */
    int16_t maxVal; /* 最大原始码值 */
} ReplayCfgAnalog_t;

/* ================================================================
 *  通道映射 (Map[] 数组中的每个条目)
 * ================================================================ */
typedef struct
{
    int line;       /* 硬件线路 (1-based, 当前固定=1) */
    int chn;        /* 硬件通道号 (1-based, 1~4) */
    char type;      /* 'U' 电压 或 'I' 电流 */
    int mapChn;     /* 映射的文件通道号 (1-based) */
    double ratio;   /* 额外幅值比例系数 */
    double delayMS; /* 通道延时 (ms) */

    /* ---- 以下为预计算字段 ---- */
    int hwIndex;      /* 硬件通道索引 0~7 (UA=0,UB=1,...IX=7) */
    int delaySamples; /* 延时对应采样点数 */
    double A_final;   /* 预计算: raw * A_final + B_final → 有符号DAC码 */
    double B_final;

    /* ---- 数据扫描与满幅缩放 ---- */
    int16_t rawActualMin; /* 扫描得到的实际最小原始值 */
    int16_t rawActualMax; /* 扫描得到的实际最大原始值 */
    double scaleApplied;  /* 已施加的缩放系数 (1.0=未缩放) */
} ReplayChannelMap_t;

/* ================================================================
 *  DO 配置
 * ================================================================ */
#define DO_MODE_KEEP 0
#define DO_MODE_TURN 1
#define DO_MODE_BREAKER 2
#define DO_MODE_MAP 3

/* Turn 模式时间参考 */
#define DO_TIMEREF_START 0
#define DO_TIMEREF_REPEATPREVEND 1

typedef struct
{
    int chn;  /* DO通道 (1-based) */
    int mode; /* DO_MODE_xxx */
    int val;  /* 初始值: 0/1/-1/-2 */

    /* Turn 模式专用 */
    int timeRef;    /* DO_TIMEREF_xxx */
    double delayMS; /* 翻转延时 (ms) */
    double holdMS;  /* 保持时长, <=0 不翻转回来 */

    /* Breaker 模式专用 */
    int diCSChn;           /* 合闸DI通道号 (1-based) */
    int diOSChn;           /* 分闸DI通道号 (1-based) */
    double breakerDelayMS; /* 断路器动作延时 (ms) */

    /* Map 模式专用 */
    int mapChn; /* 文件数字通道号 (1-based) */

    /* ---- 运行时状态 ---- */
    int currentVal;     /* 当前DO实际输出值 */
    bool turnTriggered; /* Turn: 已翻转 */
    bool turnRestored;  /* Turn: 已恢复 */
    u32 prevDI_CS;      /* Breaker: 上次合闸DI状态 */
    u32 prevDI_OS;      /* Breaker: 上次分闸DI状态 */
    u64 actualTurnUs;   /* Turn: 记录第一次翻转的实际精确微秒时间 */
} ReplayDO_Config_t;

/* ================================================================
 *  DI 触发条件 (前导提前退出)
 * ================================================================ */
typedef struct
{
    int chn; /* DI通道号 (1-based) */
    int val; /* -1=忽略, 0=低, 1=高 */
} ReplayDI_Trig_t;

/* ================================================================
 *  循环配置
 * ================================================================ */
/* 前导循环 */
typedef struct
{
    int repeatCount;  /* 循环次数, <1 不启用 */
    bool breakEnable; /* 是否允许DI提前退出 */
    int trigCount;    /* DI触发条件数量 */
    ReplayDI_Trig_t trigDIs[REPLAY_MAX_DI_TRIG];
    bool trigLogicAnd; /* true=AND, false=OR */
} ReplayRepeatPrev_t;

/* 中段循环 */
typedef struct
{
    int repeatCount; /* 循环次数, <1 不启用 */
    double beginMS;  /* 起始时刻 (<0 用原始触发点) */
    double lengthMS; /* 循环段长度 (ms) */
    u32 startSample; /* 预计算: 起始采样点 */
    u32 endSample;   /* 预计算: 结束采样点 */
} ReplayRepeatMiddle_t;

/* 后导循环 */
typedef struct
{
    int repeatCount; /* 循环次数, <1 不启用 */
} ReplayRepeatBack_t;

/* ================================================================
 *  录波配置
 * ================================================================ */
typedef struct
{
    int recRange; /* 0=不录, 1=除前后导, 2=全部 */
    int recSamp;  /* 录波采样率 (固定51200) */
} ReplayRecConfig_t;

/* ================================================================
 *  总配置结构体 (SetTaskWaveReplayParas 的产出)
 * ================================================================ */
typedef struct
{
    /* 文件信息 */
    char waveFile[REPLAY_WAVE_NAME_LEN];
    u32 datFileSize;

    /* CFG 解析结果 */
    int analogCount;    /* 模拟通道数 */
    int digitalCount;   /* 数字通道数 */
    int fileSampleRate; /* 文件采样率 */
    u32 totalSamples;   /* 文件总采样点数 */
    u32 recordSize;     /* 每条DAT记录的字节数 */
    u32 rawTrigSample;  /* 原始触发点在文件中的采样索引 */
    int digitalGroups;  /* 数字通道组数 = ceil(digitalCount/16) */
    ReplayCfgAnalog_t cfgAnalog[REPLAY_MAX_FILE_CHANNELS];

    /* 通道映射 */
    int channelMapCount;
    ReplayChannelMap_t channelMap[REPLAY_MAX_MAP_CHANNELS];

    /* DO 配置 */
    int doConfigCount;
    ReplayDO_Config_t doConfig[REPLAY_MAX_DO_CONFIG];

    /* 循环配置 */
    ReplayRepeatPrev_t repeatPrev;
    ReplayRepeatMiddle_t repeatMiddle;
    ReplayRepeatBack_t repeatBack;

    /* 录波配置 */
    ReplayRecConfig_t recConfig;

    /* 区域边界 (预计算, 采样点单位) */
    u32 firstCycleEnd;  /* = REPLAY_CYCLE_LEN */
    u32 lastCycleStart; /* = totalSamples - REPLAY_CYCLE_LEN */

    /* 预处理完成标志 */
    bool parasCompleted;
} ReplayWave_Config_t;

/* ================================================================
 *  区域状态枚举 (回放状态机)
 * ================================================================ */
typedef enum
{
    REPLAY_IDLE = 0,    /* 空闲 */
    REPLAY_WAITING,     /* 等待定时启动 */
    REPLAY_FIRST_CYCLE, /* 输出首周波 */
    REPLAY_PREV_LOOP,   /* 前导循环 (重复首周波) */
    REPLAY_MAIN_PRE,    /* 主数据前段 → 中段起点 */
    REPLAY_MIDDLE_LOOP, /* 中段循环 */
    REPLAY_MAIN_POST,   /* 主数据后段 → 尾周波前 */
    REPLAY_BACK_LOOP,   /* 后导循环 (重复尾周波) */
    REPLAY_HOLDING,     /* 保持末值输出 */
} ReplayRegion_t;

/* ================================================================
 *  运行时状态 (SetTaskWaveReplayStart 后)
 * ================================================================ */
typedef struct
{
    volatile bool isRunning;
    volatile bool isWaiting;       /* 定时启动等待 */
    volatile bool isFinished;      /* 回放完成待上报 */
    volatile bool holdingReported; /* HOLDING状态已上报Success (防止重复) */

    /* 区域管理 */
    ReplayRegion_t region;
    u32 fileSamplePos;       /* 当前文件读取位置 */
    int regionRepeatLeft;    /* 当前区域剩余循环次数 */
    bool prevBreakTriggered; /* 前导DI提前退出标志 */
    u32 middleLoopPos;       /* 中段循环内部位置 */

    /* 已输出的总DMA块计数 (用于DO时序) */
    u32 totalBlocksPlayed;

    /* 时间记录 (TaskEvent上报用) */
    char startTimeStr[32];
    char diTrigTimeStr[32];
    char preEndTimeStr[32];
    char rawTrigTimeStr[32];
    char backStartTimeStr[32];
    bool startTimeRecorded;
    bool preEndTimeRecorded;
    bool rawTrigTimeRecorded;
    bool backStartTimeRecorded;
    bool diTrigTimeRecorded;

    /* DO时序基准 (微秒) */
    u64 startTimestampUs;
    u64 preEndTimestampUs;

    /* 上报控制 */
    volatile bool reportPending; /* 有新TaskEvent需要上报 */
    char reportResult[16];       /* "Doing" / "Success" / "Failure" */

    /* 录波已启动标志 */
    bool recordingStarted;
} ReplayWave_Runtime_t;

/* ================================================================
 *  全局变量
 * ================================================================ */
extern ReplayWave_Config_t g_ReplayConfig;
extern ReplayWave_Runtime_t g_ReplayRuntime;

/* ================================================================
 *  函数声明
 * ================================================================ */

/* 初始化 */
void ReplayWave_Init(void);

/* JSON 指令处理 (供 Communications_Protocol.c 中的 handler 调用) */
void ReplayWave_HandleGetInfo(cJSON *root);
const char *ReplayWave_HandleParas(cJSON *data);
const char *ReplayWave_HandleStart(cJSON *data);

/* 播放引擎 */
void ReplayWave_FeedNext(void);       /* FIFO prog_empty 中断回调 */
void ReplayWave_Stop(void);           /* 停止回放 */
void ReplayWave_CheckAndReport(void); /* 主循环: 检查并上报 TaskEvent */

/* DI 状态变化回调 (Breaker模式/前导退出用) */
void ReplayWave_OnDIChange(u32 diCurrentVal);

/* 软时钟闹钟中断回调 (定时启动用) */
void ReplayWave_OnAlarmIRQ(void);

#endif /* REPLAY_WAVE_H */
