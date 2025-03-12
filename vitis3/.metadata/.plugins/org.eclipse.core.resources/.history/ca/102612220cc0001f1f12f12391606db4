#ifndef MY_KISSFT_H
#define MY_KISSFT_H

#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "xil_io.h"
#include "ADDA.h"

// 定义采样率和采样点数
#define SAMPLE_RATE 			(double)(51200.0*(sample_points/1024.0))     // 采样频率
#define MAX_AMPLITUDE 			16384.0       // 数据最大值范围


#define N_APPROX 				(int)(780.0 * (sample_points / 1024.0))                // 估计基波频率的点数
#define D_RANGE_MAX 			(int)(1138.0 * (sample_points / 1024.0))            // 最大步长对应45Hz的点数
#define THRESHOLD_PERCENTAGE 	0.01    // 谐波百分比阈值

// 复数结构体定义
struct compx {
    float real;
    float imag;
};
#define ChnsAC  		4  					//AC每线路支持的通道数 ABCX
#define HarmNumberMax  	32
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
}LINEAC;

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
}HARM;

typedef struct __attribute__((aligned(8)))
{
	LINEAC LineAC;
	HARM  harm[ChnsAC];  		  //harm[ChnsAC]
}AnalyzeAC;

double FindFrequence(const int WaveData[], int data_length);
void AnalyzeWaveform();


#endif


