#ifndef ADDA_H
#define ADDA_H

#include "stdio.h"
#include <stdint.h>
#include "xparameters.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "stdbool.h"
#include "sleep.h"
#include "xscutimer.h" //定时器中断的函数声明
#include "xstatus.h"
#include "xplatform_info.h"
#include "xaxidma.h"
#include <math.h>
#include "xbram_hw.h"

#include "Amplifier_Switch.h"
#include "Communications_Protocol.h"
#include "PID.h"
/*
 * 定义
 */
// 共享内存
#define RTOS_base_addr 0x21000000
#define Share_addr 0x3C000000

// 定时器
#define TIMER_DEVICE_ID XPAR_XSCUTIMER_0_DEVICE_ID // 定时器ID
#define TIMER_IRPT_INTR XPAR_SCUTIMER_INTR         // 定时器中断ID
#define TIMER_LOAD_VALUE 0x9EC969D                 // 定时器装载0.5s

// 软中断
#define CPU0_ID XSCUGIC_SPI_CPU0_MASK // CPU0 ID
#define CPU1_ID XSCUGIC_SPI_CPU1_MASK // CPU1 ID

// dma中断
#define dac_whole_base_addr XPAR_AC_8_CHANNEL_0_ADDA_DAC_WHOLE_0_BASEADDR
#define adc_whole_base_addr XPAR_AC_8_CHANNEL_0_ADDA_ADC_WHOLE_0_BASEADDR
#define DMA_DEV_ID XPAR_AXIDMA_0_DEVICE_ID
#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID

// dma_adc
#define DMA_RX_INTR_ID XPAR_FABRIC_AC_8_CHANNEL_0_ADDA_AXI_DMA_0_S2MM_INTROUT_INTR // DMA中断号
#define sample_points 256                                                          // 一个周期采样256、1024、2048
#define AD_SAMP_CYCLE_NUMBER 16                                                    // AD采样周期数
#define FIFO_DEPTH 1024                                                            // FIFO深度1024

// dma_dac
#define DMA_TX_INTR_ID XPAR_FABRIC_AC_8_CHANNEL_0_ADDA_AXI_DMA_0_MM2S_INTROUT_INTR // DMA中断号
#define Underflow_INTR_ID 65U                                                      // FIFO空中断
#define DDR_BASE_ADDR RTOS_base_addr                                               // RTOS_base_addr:0x21000000
#define MEM_BASE_ADDR (DDR_BASE_ADDR + 0xD800000)                                  // MEM_BASE_ADDR：0x2E800000				//rtos内存的一半
#define TX_BUFFER_BASE MEM_BASE_ADDR                                               // TX_BUFFER_BASE：0x2E800000
#define RX_BUFFER_BASE (TX_BUFFER_BASE + 0x00400000)                               // RX_BUFFER_BASE：0x2EC00000
#define RESET_TIMEOUT_COUNTER 10000                                                // 复位时间
#define DATA_LEN 1024                                                              // 波形采样长度
#define Data_Width 65535

// bram
#define BRAM_DATA_BYTE 4 // BRAM数据字节个数
#define CHANNL_MAX 8
#define MAX_HARMONICS 32 // 最大谐波次数

// 定义ADC状态
typedef enum
{
    ADC_STATE_IDLE,
    ADC_STATE_SAMPLING
} ADC_Process_State;
// 定义PID状态枚举
typedef enum
{
    PID_OFF = 0,
    PID_ON = 1
} PID_STATE;
/*
 * 变量
 */
// 波形修改参数
extern uint16_t Wave_NewData[8][DATA_LEN];
extern float Phase_shift[8]; // 8路波形相位偏移 单位度
extern uint16_t enable;      // 使能通道输出
extern float Wave_Frequency;
extern float Wave_Amplitude[8];
extern u32 Wave_Range[8];

extern int dma_rx_8[8][sample_points];
extern XAxiDma axidma; // XAxiDma实例
extern XScuGic intc;   // 中断控制器的实例
extern XScuTimer Timer;

extern volatile u8 Timer_Flag; // 定时器完成标志

extern bool AdcFinish_Flag;            // ADC采样完成标志
extern volatile u8 Current_DDR_Region; // 当前正在写入的DDR区域(0:Share_addr_1, 1:Share_addr_2)

extern int numHarmonics[CHANNL_MAX];                      // 每个通道有几个谐波
extern float harmonics[CHANNL_MAX][MAX_HARMONICS];        // 每个通道每次谐波的幅值
extern float harmonics_phases[CHANNL_MAX][MAX_HARMONICS]; // 每个通道每次谐波的相位

// 校正系数数组，用于将DAC输出的电压或电流值转换为实际的物理量。
// [channel][range]
// channels: 0=UA, 1=UB, 2=UC, 3=UX, 4=IA, 5=IB, 6=IC, 7=IX
// ranges for voltage: 0=6.5V (0xC2), 1=3.25V (0xD4), 2=1.876V (0xA0)
// ranges for current: 0=5A (0xC2), 1=1A (0x92), 2=0.2A (0xA4)
extern double DA_Correct_100[8][3];
extern double DA_Correct_20[8][3];

// 相位校准参数数组，存放每个通道每个量程的相位校准值（单位：度）
extern double DA_CorrectPhase_100[8][3];

// AD校准参数数组，针对不同通道和量程
// [通道][量程]
// 通道: 0=UA, 1=UB, 2=UC, 3=UX, 4=IA, 5=IB, 6=IC, 7=IX
// 电压量程: 0=6.5V, 1=3.25V, 2=1.876V
// 电流量程: 0=5A, 1=1A, 2=0.2A
extern double AD_Correct[8][3];
// 函数
void sync_dma_buffer(UINTPTR addr, size_t size, int direction);
int SafeDmaTransfer(XAxiDma *AxiDmaInstPtr, UINTPTR BuffAddr, u32 Length, int Direction);
void dma_dac_init();
void dds_dac_init();
void Adc_Start_OneBulk(int SamplePoints, int SampleFrequency);
void Adc_Start(int SamplePoints, int SampleFrequency, int SamplingPeriodNumber);
// adc
int code_to_real(u16 x);
void Adc_Data_processing();

void rx_intr_handler(void *callback);
// dma_dac
void tx_intr_handler(void *callback);
void underflow_handler();

// 定时器
int timer_init(XScuTimer *timer_ptr);
void timer_intr_handler(void *CallBackRef);

// 中断初始化
int setup_intr_system(XScuGic *int_ins_ptr, XAxiDma *axidma_ptr, XScuTimer *timer_ptr,
                      u16 rx_intr_id, u16 tx_intr_id, u16 underflow_id, u16 Timer_id);

void start_dma_dac();
// dds_dac
void str_wr_bram(PID_STATE pid_state);
void changePhase(uint16_t NewData[], int Array_Length, float Phase_Degress);
void Write_Wave_to_Wave_NewData();
void Copy_Wave_to_tx_buffer_ptr();

double sumHarmonics(float harmonics[], int numHarmonics);
void addHarmonics(uint16_t NewData[], int Array_length, float Base_Phase_Degrees, int numHarmonics, float harmonics[], float harmonics_phases[]);

// 功放
// 获取DA输出参数
int get_voltage_range_index(unsigned char range_code);
int get_current_range_index(unsigned char range_code);

// 通过档位获得参数
int get_voltage_index_by_value(float voltage);
int get_current_index_by_value(float current);

#endif
