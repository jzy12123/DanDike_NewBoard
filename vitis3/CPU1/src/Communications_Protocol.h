#ifndef COMMUNICATIONS_PROTOCOL_H
#define COMMUNICATIONS_PROTOCOL_H

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include <stdbool.h>
#include <string.h>
#include "ADDA.h"
#include "cJSON.h"
#include "Msg_que.h"
#include <math.h>

#define JSON_ADDR			0x3AC00000

#define	UDP_ADDRESS		0XFFFF0000			//UDP回报结构体地址
#define UDP_MEM_SIZE	0xFE00


#define LinesAC 		2  					//装置支持的AC线路数
#define ChnsAC  		4  					//AC每线路支持的通道数 ABCX
#define ChnsDCS  		4					//直流源通道数
#define ChnsDCM  		4					//直流表通道数
#define ChnsBI   		8					//开入通道数
#define ChnsBO			8					//开出通道数
#define HarmNumberMax  	32  				//n+1，最大支持到31次，考虑1次备用（基波），所以定义结构体时该值为实际谐波次数+1.
#define ChnsDI			32
#define ChnsDO			32
#define DisoeMsgNum		10
/******************************************************************************************************
 * UDP结构体
 ******************************************************************************************************/
typedef enum{
	BaseDataAC		= 101,
	HarmData		= 102,
	InterHarmData	= 103,
	BaseDataDCS		= 301,
	BaseDataDCM		= 401,
	DI				= 501,
	DISOE			= 510,
	DO				= 502,
	InnerBattery	= 2,
	VMData			= 110
}Return_DataType;

//回报使能状态
typedef struct {
    bool BaseDataAC;
    bool HarmData;
    bool InterHarmData;
    bool BaseDataDCS;
    bool BaseDataDCM;
    bool DI;
    bool DISOE;
    bool DO;
    bool InnerBattery;
    bool VMData;  // DK51D专用数据上报
} ReportEnableStatus;
extern ReportEnableStatus reportStatus;



//101=BaseDataAC，交流源/交流表基础数据
//交流线路——基本量                 ChnsAC*12*8+24 => 4通道=408
typedef struct  __attribute__((aligned(8)))
{
	double ur[ChnsAC];    //U档位[ChnsAC]
	double ir[ChnsAC];    //I档位[ChnsAC]

	double u[ChnsAC];     //U[ChnsAC]
	double i[ChnsAC];     //I[ChnsAC]
	double phu[ChnsAC];   //phu[ChnsAC]
	double phi[ChnsAC];   //phi[ChnsAC]
	double p[ChnsAC];     //p[ChnsAC]
	double q[ChnsAC];     //q[ChnsAC]
	double pf[ChnsAC];    //pf[ChnsAC]
	double f[ChnsAC];     //f[ChnsAC]
	double thdu[ChnsAC];  //thdu[ChnsAC]
	double thdi[ChnsAC];  //thdi[ChnsAC]

	double totalP;        //当前线路的总有功
	double totalQ;
	double totalPF;
}LineAC;



//102=HarmData，谐波数据
//交流通道谐波       HarmNumberMax*6*8+16  => HarmNumberMax为32时=1552
typedef struct __attribute__((aligned(8)))
{
	double u[HarmNumberMax];      //u[HrNo]
	double i[HarmNumberMax];      //i[HrNo]
	double phu[HarmNumberMax];    //phu[HrNo]
	double phi[HarmNumberMax];    //phi[HrNo]
	double p[HarmNumberMax];      //p[HrNo]
	double q[HarmNumberMax];      //q[HrNo]

	double totalP;        		  //当前通道的总有功
	double totalQ;
}Harm;
//交流线路——谐波              ChnsAC*1552  => 6208
typedef struct __attribute__((aligned(8)))
{
	Harm  harm[ChnsAC];  		  //harm[ChnsAC]
}LineHarm;

//501=DI,开入实时状态
//ChnsDI * 1 => 32
typedef struct
{
	uint8_t v;         			//开关量状态, 0=分,1=合
}SingleDI;
typedef struct
{
	SingleDI DI[ChnsDI];         			//最大32个开入通道
}LineDI;

//502=DO,开出实时状态
//ChnsDI * 1 => 32
typedef struct
{
	uint8_t v;         			//开关量状态, 0=分,1=合
}SingleDO;
typedef struct
{
	SingleDO DO[ChnsDO];         			//最大32个开出
}LineDO;

//510=DISOE, 开入量事件记录
//10 * （1+1+2+4）=> 80
typedef struct
{
	uint8_t   Chn;		//通道号,从1开始;
	uint8_t   Val;		//0=分,1=合
	uint16_t  MS;		//毫秒,0~1000
	uint32_t  TIME;		//从1970年经过的秒数
}SingleDisoe;
typedef struct
{
	SingleDisoe DISOE[DisoeMsgNum];
}LineDisoe;


#define MAXPAYLOAD		8 * 5 + sizeof(LineAC)+sizeof(LineHarm)+sizeof(LineDI)+sizeof(LineDO)+sizeof(LineDisoe) + 4
typedef struct __attribute__((aligned(4))){
	//帧头区
    char syncHeader[4];			// D1D2D3D4
    u32 dataLength1;			// Length of LineAC
    u32 dataLength2;			// Length of LineHarm
    u8 versionInfo;				// 1
    u8 reserved[3];				// 00 00 00
    //数据区
    char payload[MAXPAYLOAD];	//帧尾区也放进payload
    //帧尾区
//    char endFooter[4];			// E1E2E3E4
} UDPPacket;



/*****************************************************************************************************
 * JSON结构体
 ******************************************************************************************************/
//使用一个结构体来将FunCode与相应的处理函数映射起来
typedef void (*FunCodeHandler) (cJSON *data);
typedef struct {
    const char *funCode;
    FunCodeHandler handler;
} FunCodeMap;


//3.1.1直流源设置
typedef struct
{
	int Chn;
	float UR;
	float U;
	float URipple;
	float IR;
	float I_;
	float IRipple;
}SetDCS_Vals;
typedef struct
{
	bool ClosedLoop;
	SetDCS_Vals Vals[ChnsDCS];
}SetDCS;

//3.2.1直流表设置
typedef struct
{
	int Chn;
	float UR;
	float IR;
}SetDCM_Vals;
typedef struct
{
	bool ClosedLoop;
	SetDCM_Vals Vals[ChnsDCM];
}SetDCM;

//3.3.1交流源设置
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
}SetACS_Vals;
typedef struct
{
	bool ClosedLoop;
	SetACS_Vals Vals[LinesAC * ChnsAC];
}SetACS;


//3.3.2交流表设置
typedef struct
{
	int Line;
	int Chn;
	float UR;
	float IR;
}SetACM_Vals;
typedef struct
{
	SetACM_Vals Vals[LinesAC * ChnsAC];
}SetACM;


//3.3.3谐波输出设置
typedef struct
{
	int Line;
	int Chn;
	int HR;
	float U;
	float PhU;
	float I_;
	float PhI;
}SetHarm_Vals;
typedef struct
{
	SetHarm_Vals Vals[LinesAC * ChnsAC * HarmNumberMax];
}SetHarm;

//3.4 开出设置
typedef struct
{

	int Chn;
	bool val;
}SetDO_Vals;
typedef struct
{
	SetDO_Vals Vals[ChnsBO];
}SetDO;


// 定义JSON回报结构体
typedef struct {
    char FunCode[32];
    char Result[16];
    bool hasClosedLoop; // 指示是否包含ClosedLoop字段
    bool ClosedLoop;
} ReplyData;


extern LineAC lineAC;
extern LineHarm lineHarm;
extern LineDI lineDI;
extern LineDO lineDO;
extern LineDisoe lineDisoe;
/******************************************************************************************************
 * 函数申明
 ******************************************************************************************************/
void ReportUDP_Structure(ReportEnableStatus ReportStatus);
size_t calculate_dynamic_payload_size(ReportEnableStatus ReportStatus);
void write_UDP_to_shared_memory(UINTPTR base_addr, void* data, size_t size);
void initLineAC(LineAC *lineAC);
void initLineHarm(LineHarm *lineHarm);
void initLineDI(LineDI *lineDI);
void initLineDO(LineDO *lineDO);
void initLineDisoe(LineDisoe *lineDisoe);

int Parse_JsonCommand(char *buffer);
void write_command_to_shared_memory();
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
void handle_SetInterHarm(cJSON *data);
void handle_SetDO(cJSON *data);
void handle_StopDCS(cJSON *data);
void handle_StopAC(cJSON *data);
void handle_ClearHarm(cJSON *data);
void handle_ClearInterHarm(cJSON *data);
#endif
