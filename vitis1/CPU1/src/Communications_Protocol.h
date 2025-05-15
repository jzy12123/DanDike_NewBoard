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
#include "Rc64.h"
#include <math.h>

#define JSON_ADDR 0x3AC00000

#define UDP_ADDRESS 0XFFFF0000 // UDP�ر��ṹ���ַ
#define UDP_MEM_SIZE 0xFE00

#define LinesAC 2		 // װ��֧�ֵ�AC��·��
#define ChnsAC 4		 // ACÿ��·֧�ֵ�ͨ���� ABCX
#define ChnsDCS 4		 // ֱ��Դͨ����
#define ChnsDCM 4		 // ֱ����ͨ����
#define ChnsBI 8		 // ����ͨ����
#define ChnsBO 8		 // ����ͨ����
#define HarmNumberMax 32 // n+1�����֧�ֵ�31�Σ�����1�α��ã������������Զ���ṹ��ʱ��ֵΪʵ��г������+1.
#define ChnsDI 32
#define ChnsDO 32
#define DisoeMsgNum 10
/******************************************************************************************************
 * UDP�ṹ��
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
	DISOE = 510,
	DO = 502,
	InnerBattery = 2,
	VMData = 110
} Return_DataType;

// �ر�ʹ��״̬
typedef struct
{
	bool DevState;
	bool BaseDataAC;
	bool HarmData;
	bool InterHarmData;
	bool BaseDataDCS;
	bool BaseDataDCM;
	bool DI;
	bool DISOE;
	bool DO;
	bool InnerBattery;
	bool VMData; // DK51Dר�������ϱ�
} ReportEnableStatus;
extern ReportEnableStatus reportStatus;

// 100=DevState, װ��״̬
typedef struct
{
	uint8_t bACMeterMode; // 1�ֽ� // 0=����Դ״̬;1=������״̬
	uint8_t bACRunning;	  // 0=ֹͣ״̬;1=����״̬
	uint8_t bClosedLoop;  // 0=����״̬;1=�ջ�״̬
	uint8_t Reserved3;
	uint8_t Reserved4;
	uint8_t Reserved5;
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

// 101=BaseDataAC������Դ/�������������
// ������·����������                 ChnsAC*12*8+24 => 4ͨ��=408
typedef struct
{
	double ur[ChnsAC]; // U��λ[ChnsAC]
	double ir[ChnsAC]; // I��λ[ChnsAC]

	volatile double u[ChnsAC];	  // U[ChnsAC]
	volatile double i[ChnsAC];	  // I[ChnsAC]
	volatile double phu[ChnsAC];  // phu[ChnsAC]
	volatile double phi[ChnsAC];  // phi[ChnsAC]
	volatile double p[ChnsAC];	  // p[ChnsAC]
	volatile double q[ChnsAC];	  // q[ChnsAC]
	volatile double pf[ChnsAC];	  // pf[ChnsAC]
	volatile double f[ChnsAC];	  // f[ChnsAC]
	volatile double thdu[ChnsAC]; // thdu[ChnsAC]
	volatile double thdi[ChnsAC]; // thdi[ChnsAC]

	volatile double totalP; // ��ǰ��·�����й�
	volatile double totalQ;
	volatile double totalPF;
} LineAC;
extern LineAC lineAC;

// 102=HarmData��г������
// ����ͨ��г��       HarmNumberMax*6*8+16  => HarmNumberMaxΪ32ʱ=1552
typedef struct
{
	double u[HarmNumberMax];   // u[HrNo]
	double i[HarmNumberMax];   // i[HrNo]
	double phu[HarmNumberMax]; // phu[HrNo]
	double phi[HarmNumberMax]; // phi[HrNo]
	double p[HarmNumberMax];   // p[HrNo]
	double q[HarmNumberMax];   // q[HrNo]

	double totalP; // ��ǰͨ�������й�
	double totalQ;
} Harm;
// ������·����г��              ChnsAC*1552  => 6208
typedef struct
{
	Harm harm[ChnsAC]; // harm[ChnsAC]
} LineHarm;
extern LineHarm lineHarm;

// 501=DI,����ʵʱ״̬
// ChnsDI * 1 => 32
typedef struct
{
	uint8_t v; // ������״̬, 0=��,1=��
} SingleDI;
typedef struct
{
	SingleDI DI[ChnsDI]; // ���32������ͨ��
} LineDI;
extern LineDI lineDI;

// 502=DO,����ʵʱ״̬
// ChnsDI * 1 => 32
typedef struct
{
	uint8_t v; // ������״̬, 0=��,1=��
} SingleDO;
typedef struct
{
	SingleDO DO[ChnsDO]; // ���32������
} LineDO;
extern LineDO lineDO;

// 510=DISOE, �������¼���¼
// 10 * ��1+1+2+4��=> 80
typedef struct
{
	uint8_t Chn;   // ͨ����,��1��ʼ;
	uint8_t Val;   // 0=��,1=��
	uint16_t MS;   // ����,0~1000
	uint32_t TIME; // ��1970�꾭��������
} SingleDisoe;
typedef struct
{
	SingleDisoe DISOE[DisoeMsgNum];
} LineDisoe;
extern LineDisoe lineDisoe;

#define MAXPAYLOAD (2 * sizeof(u32) + sizeof(DevState)) + (2 * sizeof(u32) + sizeof(LineAC)) + (2 * sizeof(u32) + sizeof(LineHarm)) + (2 * sizeof(u32) + sizeof(LineDI)) + (2 * sizeof(u32)) + (2 * sizeof(u32) + sizeof(LineDO)) + (2 * sizeof(u32) + sizeof(LineDisoe)) + sizeof(u32) // ������+֡β��
typedef struct
{
	// ֡ͷ��
	char syncHeader[4]; // D1D2D3D4
	u32 dataLength1;	// ���ݳ���
	u32 dataLength2;	// ���ݳ����ظ�,����У��

	u8 versionInfo; // 1
	u8 reserved[3]; // 00 00 00
	// ������
	char payload[MAXPAYLOAD]; // ֡β��Ҳ�Ž�payload
							  //    char endFooter[4];			// E1E2E3E4
} UDPPacket;

/*****************************************************************************************************
 * JSON�ṹ��
 ******************************************************************************************************/
// ʹ��һ���ṹ������FunCode����Ӧ�Ĵ�����ӳ������
typedef void (*FunCodeHandler)(cJSON *data);
typedef struct
{
	const char *funCode;
	FunCodeHandler handler;
} FunCodeMap;

// 3.1.1ֱ��Դ����
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

// 3.2.1ֱ��������
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

// 3.3.1����Դ����
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

// 3.3.2����������
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

// 3.3.3г���������
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

// 3.4 ��������
typedef struct
{

	int Chn;
	bool val;
} SetDO_Vals;
typedef struct
{
	SetDO_Vals Vals[ChnsBO];
} SetDO;

// ����JSON�ر��ṹ��
typedef struct
{
	char FunCode[32];
	char Result[16];
	bool hasClosedLoop; // ָʾ�Ƿ����ClosedLoop�ֶ�
	bool ClosedLoop;
} ReplyData;

extern const char FPGA_Ver_Full[];
extern const char ARM_Ver_Full[];
/******************************************************************************************************
 * ��������
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
void initLineDisoe(LineDisoe *lineDisoe);
void init_JsonUdp(void);

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
void handle_SetCalibrateAC(cJSON *data);
void handle_WriteCalibrateAC(cJSON *data);
// �����ϱ�
void report_protection_event(u8 ProectFault);
#endif
