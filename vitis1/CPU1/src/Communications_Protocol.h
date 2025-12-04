#ifndef COMMUNICATIONS_PROTOCOL_H
#define COMMUNICATIONS_PROTOCOL_H

#include "string.h"
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include <stdbool.h>
#include <string.h>
#include "ADDA.h"
#include "cJSON.h"
#include "Msg_que.h"
#include "Rc64.h"
#include <math.h>
#include "Amplifier_Switch.h"
#include "Timer_sync.h"
#include "power_pulse.h"
#include "soft_timer.h"
#include "StateSequence.h"

#define JSON_ADDR 0x3AC00000

#define UDP_ADDRESS 0XFFFF0000 // UDP回报结构体地址
#define UDP_MEM_SIZE 0xFE00

#define LinesAC 2		 // 装置支持的AC线路数
#define ChnsAC 4		 // AC每线路支持的通道数 ABCX
#define ChnsDCS 4		 // 直流源通道数
#define ChnsDCM 4		 // 直流表通道数
#define ChnsBI 8		 // 开入通道数
#define ChnsBO 8		 // 开出通道数
#define HarmNumberMax 32 // n+1，最大支持到31次，考虑1次备用（基波），所以定义结构体时该值为实际谐波次数+1.
#define ChnsDI 32
#define ChnsDO 32
#define DisoeMsgNum 10

/******************************************************************************************************
 * UDP结构体
 ******************************************************************************************************/
typedef enum
{
	DeviceState = 100,
	BaseDataAC = 101,
	HarmData = 102,
	InterHarmData = 103,
	BaseDataDCS = 301,
	BaseDataDCM = 401,
	DI = 501,
	DO = 502,
	InnerBattery = 2,
	VMData = 110
} Return_DataType;

// 回报使能状态
typedef struct
{
	bool DevState;
	bool BaseDataAC;
	bool HarmData;
	bool InterHarmData;
	bool BaseDataDCS;
	bool BaseDataDCM;
	bool DI;
	bool DO;
	bool InnerBattery;
	bool VMData; // DK51D专用数据上报
} ReportEnableStatus;
extern ReportEnableStatus reportStatus;

// 100=DevState, 装置状态（16字节）
// 字节顺序：bACMeterMode, bClosedLoop, nStatusFund, bHarmRunning, Reserved4..Reserved15
typedef struct
{
	uint8_t bACMeterMode;  // 1字节 // 0=交流源状态;1=交流表状态
	uint8_t bClosedLoop;   // 1字节 // 0=开环状态;1=闭环状态
	uint8_t nStatusFund;   // 1字节 // (原bACRunning) 基波: 0=停止;1=运行;2=暂停
	uint8_t nStatusHarm;   // 1字节 // (原bHarmRunning) 谐波: 0=停止;1=运行;2=暂停
	uint8_t nStatusInharm; // 1字节 // 间谐波: 0=停止;1=运行;2=暂停
	uint8_t bAutoRange;	   // 1字节 // 0=手动档;1=自动档
	uint8_t Reserved6;
	uint8_t Reserved7;
	uint8_t Reserved8;
	uint8_t Reserved9;
	uint8_t Reserved10;
	uint8_t Reserved11;
	uint8_t Reserved12;
	uint8_t Reserved13;
	uint8_t Reserved14;
	uint8_t Reserved15;
} DevState;
extern DevState devState;

// 101=BaseDataAC，交流源/交流表基础数据
// 交流线路——基本量                 ChnsAC*12*8+24 => 4通道=408
typedef struct
{
	double ur[ChnsAC]; // U档位[ChnsAC]
	double ir[ChnsAC]; // I档位[ChnsAC]

	volatile double u[ChnsAC];	  // U[ChnsAC]	//总有效值
	volatile double i[ChnsAC];	  // I_[ChnsAC]	//总有效值
	volatile double phu[ChnsAC];  // phu[ChnsAC]	//基波有效值
	volatile double phi[ChnsAC];  // phi[ChnsAC]	//基波有效值
	volatile double p[ChnsAC];	  // p[ChnsAC]	//总有效值
	volatile double q[ChnsAC];	  // q[ChnsAC]	//总有效值
	volatile double pf[ChnsAC];	  // pf[ChnsAC]	//总有效值
	volatile double f[ChnsAC];	  // f[ChnsAC]	//基波有效值
	volatile double thdu[ChnsAC]; // thdu[ChnsAC]	//总有效值
	volatile double thdi[ChnsAC]; // thdi[ChnsAC]	//总有效值

	volatile double totalP; // 当前线路的总有功
	volatile double totalQ;
	volatile double totalPF;
} LineAC;
extern LineAC lineAC;

// 102=HarmData，谐波数据
// 交流通道谐波（下标0=直流，1=基波，2..=谐波），
// u/i：对谐波（索引>=2）按含量（百分数）表示；P/Q：按幅值表示
// 大小估算：HarmNumberMax*6*8+16
typedef struct
{
	double u[HarmNumberMax];   // u[HrNo], 0=DC,1=Base,2+=Harm
	double i[HarmNumberMax];   // i[HrNo]
	double phu[HarmNumberMax]; // phu[HrNo]
	double phi[HarmNumberMax]; // phi[HrNo]
	double p[HarmNumberMax];   // p[HrNo]
	double q[HarmNumberMax];   // q[HrNo]

	double totalP; // 当前通道的总有功
	double totalQ;
} Harm;
// 交流线路——谐波
typedef struct
{
	Harm harm[ChnsAC]; // harm[ChnsAC]
} LineHarm;
extern LineHarm lineHarm;

// 501=DI,开入实时状态
// ChnsDI * 1 => 32
typedef struct
{
	uint8_t v; // 开关量状态, 0=分,1=合
} SingleDI;
typedef struct
{
	SingleDI DI[ChnsDI]; // 最大32个开入通道
} LineDI;
extern LineDI lineDI;

// 502=DO,开出实时状态
// ChnsDI * 1 => 32
typedef struct
{
	uint8_t v; // 开关量状态, 0=分,1=合
} SingleDO;
typedef struct
{
	SingleDO DO[ChnsDO]; // 最大32个开出
} LineDO;
extern LineDO lineDO;

#define MAXPAYLOAD (2 * sizeof(u32) + sizeof(DevState)) + (2 * sizeof(u32) + sizeof(LineAC)) + (2 * sizeof(u32) + sizeof(LineHarm)) + (2 * sizeof(u32) + sizeof(LineDI)) + (2 * sizeof(u32) + sizeof(LineDO)) + sizeof(u32) // 数据区+帧尾区									  // 帧

typedef struct
{
	// 帧头区
	char syncHeader[4]; // D1D2D3D4
	u32 dataLength1;	// 数据长度
	u32 dataLength2;	// 数据长度重复,冗余校验

	u8 versionInfo; // 1
	u8 reserved[3]; // 00 00 00
	// 数据区
	char payload[MAXPAYLOAD]; // 帧尾区也放进payload
} UDPPacket;

/*****************************************************************************************************
 * JSON结构体
 ******************************************************************************************************/
// 使用一个结构体来将FunCode与相应的处理函数映射起来
typedef void (*FunCodeHandler)(cJSON *data);
typedef struct
{
	const char *funCode;
	FunCodeHandler handler;
} FunCodeMap;

// 3.1.1直流源设置
typedef struct
{
	int Chn;
	float UR;
	float U;
	float URipple;
	float IR;
	float I_;
	float IRipple;
} SetDCS_Vals;
typedef struct
{
	bool ClosedLoop;
	SetDCS_Vals Vals[ChnsDCS];
} SetDCS;

// 3.2.1直流表设置
typedef struct
{
	int Chn;
	float UR;
	float IR;
} SetDCM_Vals;
typedef struct
{
	bool ClosedLoop;
	SetDCM_Vals Vals[ChnsDCM];
} SetDCM;

// 3.3.1交流源设置
typedef struct
{
	int Line;
	int Chn;
	float F;
	float UR;
	float U;
	float PhU;
	float IR;
	float I_;
	float PhI;
} SetACS_Vals;
typedef struct
{
	bool ClosedLoop;
	SetACS_Vals Vals[LinesAC * ChnsAC];
} SetACS;
extern SetACS setACS;

// 3.3.2交流表设置
typedef struct
{
	int Line;
	int Chn;
	float UR;
	float IR;
} SetACM_Vals;
typedef struct
{
	SetACM_Vals Vals[LinesAC * ChnsAC];
} SetACM;

// 3.3.3谐波输出设置
typedef struct
{
	int Line;
	int Chn;
	int HN;
	float U;
	float PhU;
	float I_;
	float PhI;
} SetHarm_Vals;
typedef struct
{
	SetHarm_Vals Vals[LinesAC * ChnsAC * HarmNumberMax];
} SetHarm;

// 3.4 开出设置
typedef struct
{
	int Chn;
	bool val;
} SetDO_Vals;
typedef struct
{
	SetDO_Vals Vals[ChnsBO];
} SetDO;
extern SetDO setDO;

// 定义JSON回报结构体
typedef struct
{
	char FunCode[32];
	char Result[16];
	bool hasClosedLoop; // 指示是否包含ClosedLoop字段
	bool ClosedLoop;
} ReplyData;

extern volatile bool udp_data_changed_flag; // UDP数据变化标志位
extern volatile bool dac_parameters_updated_by_command;
extern const char FPGA_Ver_Full[];
extern const char ARM_Ver_Full[];

extern uint8_t paused_bClosedLoop_state;
extern uint8_t target_powamp_enable_state_after_pause;

extern uint32_t g_do_output_state;
extern int g_harm_number_thd; // 新增: 用于计算THD的谐波次数
// 中文注释: 定义专用于中断服务程序（ISR）安全读取的“影子”变量。
// volatile关键字确保每次访问都直接读写内存，防止编译器优化。
extern volatile double g_safe_total_p_for_isr;
extern volatile double g_safe_total_q_for_isr;

typedef struct
{
	int HN;
	float U;
	float PhU;
	float I_;
	float PhI;
} Struct_Seq_Harm;

#define MAX_SEQ_HARMS 32
typedef struct
{
	int Line;
	int Chn;
	float U;
	float PhU;
	float I_;
	float PhI;
	float F;
	float UR; // 档位
	float IR; // 档位
	int HarmCount;
	Struct_Seq_Harm Harms[MAX_SEQ_HARMS];
} Struct_Seq_AC;

typedef struct
{
	int Chn;
	int Val;
} Struct_Seq_DO;

typedef struct
{
	int Chn;
	int Val;
} Struct_Seq_TrigDI;

typedef struct
{
	int MaxDuration;
	int JumpTo;
	int TrigLogic;
	int TrigDICount;
	Struct_Seq_TrigDI TrigDIs[8]; // 最大8个触发条件
	int ACCount;
	Struct_Seq_AC ACs[8]; // 最大8个通道
	int DOCount;
	Struct_Seq_DO DOs[8]; // 最大8个开出
} Struct_Seq_Step;

#define MAX_SEQ_STEPS 50
typedef struct
{
	int StartMode;
	char StartTime[32];
	int RepeatCount;
	int RecStartState;
	int RecMS;
	int RecSamp;
	int StepCount;
	Struct_Seq_Step Steps[MAX_SEQ_STEPS];
} Struct_StateSequence;
extern Struct_StateSequence g_StateSequenceTask; // 状态序列全局变量
/******************************************************************************************************
 * 函数申明
 ******************************************************************************************************/
const char *get_version_string(const char *full_version_str);
void ReportUDP_Structure(ReportEnableStatus ReportStatus);
size_t calculate_dynamic_payload_size(ReportEnableStatus ReportStatus);
void write_UDP_to_shared_memory(UINTPTR base_addr, void *data, size_t size);
void initDevState(DevState *devState);
void initLineAC(LineAC *lineAC);
void initLineHarm(LineHarm *lineHarm);
void initLineDI(LineDI *lineDI);
void initLineDO(LineDO *lineDO);
void init_JsonUdp(void);

int Parse_JsonCommand(char *buffer);
void write_reply_to_shared_memory(ReplyData *replyData);
void handle_GetFunCodeList(cJSON *data);
void handle_GetDevBaseInfo(cJSON *data);
void handle_GetDevState(cJSON *data);
void handle_SetReportEnable(cJSON *data);
void handle_SetDCS(cJSON *data);
void handle_SetDCM(cJSON *data);
void handle_SetACS(cJSON *data);
void handle_SetACM(cJSON *data);
void handle_SetHarm(cJSON *data);
void handle_SetDO(cJSON *data);
void handle_StopDCS(cJSON *data);
void handle_SetACStatus(cJSON *data);
void handle_SetCalibrateAC(cJSON *data);
void handle_WriteCalibrateAC(cJSON *data);
void handle_RestoreCalibrateDefault(cJSON *data);
void handle_SetSysTimeSyncMode(cJSON *data);
void handle_SetTaskEnergyTest(cJSON *data);
void handle_TerminateRunningTask(cJSON *data);
void handle_StateSequence(cJSON *data);
void handle_SetDevState(cJSON *data);  
// 主动上报
void report_protection_event(u8 ProectFault);
void check_and_report_energy_test_status(void);

#endif
