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
#include "Communications_Protocol.h"
#include "kiss_fft\kiss_fft.h"
// 定义采样率和采样点数
#define SAMPLE_RATE 			50*sample_points     // 采样频率
#define MAX_AMPLITUDE 			16384.0       // 数据最大值范围


#define D_RANGE_MIN 			(int)(780.0 * (sample_points / 1024.0))                // 估计基波频率的点数
#define D_RANGE_MAX 			(int)(1138.0 * (sample_points / 1024.0))            // 最大步长对应45Hz的点数
#define THRESHOLD_PERCENTAGE 	0.01    // 谐波百分比阈值




void AnalyzeWaveform(double harmonic_info[][3], int channel);
void AnalyzeWaveform_AcSource(double harmonic_info[][3], int channel, u32 ddr_addr,
                              int SampleFrequency, double fundamental_frequency);

#endif


