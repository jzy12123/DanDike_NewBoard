
#include "My_kissFft.h"
#include <float.h> // For FLT_MAX
#define FILTER_ORDER 100
double b[FILTER_ORDER + 1] = {
    /* 这里填入MATLAB生成的b系数 100阶fir滤波器 */
    0.0009, 0.0010, 0.0010, 0.0011, 0.0012, 0.0013, 0.0015, 0.0017, 0.0019, 0.0021, 0.0024, 0.0027, 0.0030, 0.0034,
    0.0038, 0.0042, 0.0046, 0.0051, 0.0056, 0.0061, 0.0066, 0.0072, 0.0077, 0.0083, 0.0089, 0.0095, 0.0101, 0.0107,
    0.0114, 0.0120, 0.0126, 0.0132, 0.0138, 0.0144, 0.0149, 0.0155, 0.0160, 0.0165, 0.0170, 0.0175, 0.0179, 0.0183,
    0.0186, 0.0189, 0.0192, 0.0194, 0.0196, 0.0198, 0.0199, 0.0200, 0.0200, 0.0200, 0.0199, 0.0198, 0.0196, 0.0194,
    0.0192, 0.0189, 0.0186, 0.0183, 0.0179, 0.0175, 0.0170, 0.0165, 0.0160, 0.0155, 0.0149, 0.0144, 0.0138, 0.0132,
    0.0126, 0.0120, 0.0114, 0.0107, 0.0101, 0.0095, 0.0089, 0.0083, 0.0077, 0.0072, 0.0066, 0.0061, 0.0056, 0.0051,
    0.0046, 0.0042, 0.0038, 0.0034, 0.0030, 0.0027, 0.0024, 0.0021, 0.0019, 0.0017, 0.0015, 0.0013, 0.0012, 0.0011,
    0.0010, 0.0010, 0.0009};

// FIR 滤波函数
void fir_filter(int *input, double *output, int data_len, double *coeffs, int coeff_len)
{
    for (int i = 0; i < data_len; i++)
    {
        output[i] = 0.0;
        for (int j = 0; j <= coeff_len; j++)
        {
            if (i - j >= 0)
            {
                output[i] += input[i - j] * coeffs[j];
            }
        }
    }
}

#define MAX_N 4096 // 最大支持的点数

// 计算 M 矩阵
void CalculateM(double *wave_data, int data_len, double *M)
{
    double h[MAX_N + 1], b[MAX_N + 1], c[MAX_N + 1], d[MAX_N + 1];
    double u[MAX_N + 1], v[MAX_N + 1], temp_y[MAX_N + 1];

    // 自然边界条件
    M[0] = M[data_len - 1] = 0;

    // 计算 h、b、c 和 d
    for (int i = 0; i < data_len - 1; i++)
    {
        h[i] = 1.0; // 原始数据点等距（x[i+1] - x[i] = 1.0）
    }
    for (int i = 1; i < data_len - 1; i++)
    {
        b[i] = h[i] / (h[i] + h[i - 1]);
        c[i] = 1 - b[i];
        d[i] = 6 * ((wave_data[i + 1] - wave_data[i]) / h[i] - (wave_data[i] - wave_data[i - 1]) / h[i - 1]) /
               (h[i] + h[i - 1]);
    }

    // 用追赶法计算 M[i]
    d[1] -= c[1] * M[0];
    d[data_len - 2] -= b[data_len - 2] * M[data_len - 1];
    b[data_len - 2] = 0;
    c[1] = 0;

    for (int i = 1; i < data_len - 1; i++)
    {
        u[i] = 2 - c[i] * v[i - 1];
        v[i] = b[i] / u[i];
        temp_y[i] = (d[i] - c[i] * temp_y[i - 1]) / u[i];
    }
    for (int i = data_len - 2; i >= 1; i--)
    {
        M[i] = temp_y[i] - v[i] * M[i + 1];
    }
}

// 三次样条插值
double *interp1_spline(double *wave_data, int data_len, int factor, int *new_len)
{
    int interp_len = data_len * factor; // 插值后的数据长度
    *new_len = interp_len;

    // 分配插值结果数组
    double *interp_data = malloc(interp_len * sizeof(double));
    if (!interp_data)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // 计算 M 矩阵
    double M[MAX_N + 1];
    CalculateM(wave_data, data_len, M);

    // 插值计算
    for (int i = 0; i < interp_len; i++)
    {
        double pos = (double)i / factor; // 插值点对应的原始数据位置
        int k = (int)floor(pos);         // 找到 pos 所在的区间
        if (k >= data_len - 1)
        {
            k = data_len - 2; // 防止越界
        }

        double h = 1.0; // 原始数据点等距（x[k+1] - x[k] = 1.0）
        double p = k + 1 - pos;
        double q = pos - k;
        interp_data[i] = (p * p * p * M[k] + q * q * q * M[k + 1]) / (6 * h) +
                         (p * wave_data[k] + q * wave_data[k + 1]) / h -
                         h * (p * M[k] + q * M[k + 1]) / 6;
    }

    return interp_data;
}

// 插值函数：线性插值
double *interp1_linear(double *x, int x_len, int factor, int *new_len)
{
    int interp_len = x_len * factor;
    double *x_interp = malloc(interp_len * sizeof(double));
    for (int i = 0; i < interp_len; i++)
    {
        double pos = (double)i / factor;
        int low = (int)floor(pos);
        int high = low + 1;
        double weight = pos - low;

        if (high >= x_len)
            high = x_len - 1;
        x_interp[i] = (1 - weight) * x[low] + weight * x[high];
    }
    *new_len = interp_len;
    return x_interp;
}

// 插值函数：固定等间距插值
double *interp1_linear_fix(double *x, int x_len, int factor, int *new_len)
{
    // 插值后的点数
    int interp_len = x_len * factor;
    *new_len = interp_len;

    // 为插值结果分配内存
    double *x_interp = malloc(interp_len * sizeof(double));
    if (!x_interp)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    // 插值计算
    for (int i = 0; i < x_len - 1; i++)
    { // 遍历每两个点
        double x1 = x[i];
        double x2 = x[i + 1];

        for (int j = 0; j < factor; j++)
        { // 在每两个点间插值
            double fraction = (double)j / factor;
            x_interp[i * factor + j] = x1 + (x2 - x1) * fraction;
        }
    }

    // 最后一个点需要单独插入
    x_interp[(x_len - 1) * factor] = x[x_len - 1];

    return x_interp;
}

// 最大似然法估计基波频率（从第二个周期开始，估算10次频率并取平均）
double interp_most_like_find_frequency(double *wave_data, int d_range_min, int d_range_max, int fs, int data_len)
{
    const int interp_factor = 10; // 插值倍数
    int interp_len;
    double *interp_data = interp1_linear(wave_data, data_len, interp_factor, &interp_len);
    // 截取信号的第二个周期开始
    int segment_len = d_range_min * interp_factor; // 一个周期的长度
    int start_index = segment_len;                 // 从第二个周期开始
    int num_cycles = 1;                            // 估算1次频率
    double total_frequency = 0.0;

    // 确保有足够的插值数据用于估算10次
    if (start_index + num_cycles * segment_len > interp_len)
    {
        fprintf(stderr, "Error: Not enough data to estimate 10 cycles.\n");
        free(interp_data);
        return -1; // 返回错误
    }

    for (int cycle = 0; cycle < num_cycles; cycle++)
    {
        double *segment = malloc(segment_len * sizeof(double));
        memcpy(segment, interp_data + start_index + cycle * segment_len, segment_len * sizeof(double)); // 从当前周期截取

        double max_likelihood = -INFINITY;
        int best_d = 0;

        // 遍历d范围计算似然度
        for (int d = d_range_min * interp_factor; d <= d_range_max * interp_factor; d++)
        {
            double likelihood = 0.0;
            for (int i = 0; i < segment_len; i++)
            {
                if (start_index + cycle * segment_len + i + d >= interp_len)
                    break;
                double diff = interp_data[start_index + cycle * segment_len + i + d] - segment[i];
                likelihood -= diff * diff;
            }
            if (likelihood > max_likelihood)
            {
                max_likelihood = likelihood;
                best_d = d;
            }
        }

        // 估算当前周期的频率
        double d_T = (double)best_d / interp_factor;
        double f_est = fs / d_T;

        total_frequency += f_est; // 累加估算频率
        free(segment);
    }

    free(interp_data);

    // 返回平均频率
    return total_frequency / num_cycles;
}

// 生成Flat-Top窗函数
void generate_flat_top_window(double *window, int length)
{
    const double a0 = 0.18810;
    const double a1 = 0.36923;
    const double a2 = 0.28702;
    const double a3 = 0.13077;
    const double a4 = 0.02488;

    for (int n = 0; n < length; n++)
    {
        double cos1 = cos(2 * M_PI * n / (length - 1));
        double cos2 = cos(4 * M_PI * n / (length - 1));
        double cos3 = cos(6 * M_PI * n / (length - 1));
        double cos4 = cos(8 * M_PI * n / (length - 1));

        window[n] = a0 - a1 * cos1 + a2 * cos2 - a3 * cos3 + a4 * cos4;
    }
}
void perform_fft_with_window(
    int *data,                // 输入信号
    int data_len,             // 信号长度
    int L_window,             // 窗函数长度
    double *flat_top_window,  // Flat-top 窗函数
    double overlap_ratio,     // 重叠率
    int fft_length,           // FFT 长度
    int num_harmonics,        // 最大谐波数
    double fundamental_freq,  // 基波频率
    double Fs,                // 采样率
    double harmonic_info[][3] // 用于存储谐波信息的二维数组
)
{
    // Step size (overlap size)
    int step_size = (int)floor(L_window * overlap_ratio);

    // Number of windows
    int num_windows = (data_len - L_window) / step_size + 1;

    // Allocate memory for FFT results
    kiss_fft_cfg cfg = kiss_fft_alloc(fft_length, 0, NULL, NULL);
    kiss_fft_cpx *in = malloc(fft_length * sizeof(kiss_fft_cpx));
    kiss_fft_cpx *out = malloc(fft_length * sizeof(kiss_fft_cpx));
    double **fft_results_windowed = malloc(num_windows * sizeof(double *));
    for (int i = 0; i < num_windows; i++)
    {
        fft_results_windowed[i] = calloc(fft_length / 2, sizeof(double));
    }

    // Process each window
    for (int i = 0; i < num_windows; i++)
    {
        // Compute start and end indices for the current window
        int start_idx = i * step_size;

        // Apply the Flat-top window
        for (int j = 0; j < L_window; j++)
        {
            in[j].r = data[start_idx + j] * flat_top_window[j];
            in[j].i = 0.0; // Real input, imaginary part is 0
        }
        // Zero-pad the remaining FFT input 实际不起作用
        for (int j = L_window; j < fft_length; j++)
        {
            in[j].r = 0.0;
            in[j].i = 0.0;
        }

        // Perform FFT
        kiss_fft(cfg, in, out);

        // Compute single-sided amplitude spectrum
        for (int j = 0; j < fft_length / 2; j++)
        {
            double magnitude = sqrt(out[j].r * out[j].r + out[j].i * out[j].i) / L_window;
            fft_results_windowed[i][j] = magnitude;
        }
    }

    // Average the FFT results across all windows
    double *avg_fft_windowed = calloc(fft_length / 2, sizeof(double));
    for (int j = 0; j < fft_length / 2; j++)
    {
        for (int i = 0; i < num_windows; i++)
        {
            avg_fft_windowed[j] += fft_results_windowed[i][j];
        }
        avg_fft_windowed[j] /= num_windows;
    }

    // Amplitude calibration
    const double a0 = 0.18810; // Flat-top window calibration coefficient
    for (int j = 0; j < fft_length / 2; j++)
    {
        avg_fft_windowed[j] *= (2.0 / (L_window * a0));
    }

    // Frequency vector
    double *f = malloc(fft_length / 2 * sizeof(double));
    for (int j = 0; j < fft_length / 2; j++)
    {
        f[j] = Fs * j / fft_length;
    }

    // Find fundamental amplitude
    int fundamental_idx = 0;
    double min_diff = fabs(f[0] - fundamental_freq);
    for (int j = 1; j < fft_length / 2; j++)
    {
        double diff = fabs(f[j] - fundamental_freq);
        if (diff < min_diff)
        {
            min_diff = diff;
            fundamental_idx = j;
        }
    }
    //    double fundamental_amplitude = avg_fft_windowed[fundamental_idx];

    // Find harmonics
    for (int k = 0; k < num_harmonics; k++)
    {
        double harmonic_freq = (k + 1) * fundamental_freq;
        int harmonic_idx = 0;
        min_diff = fabs(f[0] - harmonic_freq);
        for (int j = 1; j < fft_length / 2; j++)
        {
            double diff = fabs(f[j] - harmonic_freq);
            if (diff < min_diff)
            {
                min_diff = diff;
                harmonic_idx = j;
            }
        }

        // Store harmonic information in the array
        harmonic_info[k][0] = harmonic_freq; // Harmonic frequency
        double harmonic_Amplitude = avg_fft_windowed[harmonic_idx];
        double harmonic_Phase = atan2(out[harmonic_idx].i, out[harmonic_idx].r) * 180.0 / M_PI;
        //        if(harmonic_Amplitude < 0.01){
        //        	harmonic_Amplitude = 0;
        //        	harmonic_Phase = 0;
        //        }
        harmonic_info[k][1] = harmonic_Amplitude; // Harmonic amplitude as percentage
        harmonic_info[k][2] = harmonic_Phase;     // Harmonic phase in degrees
    }

    // Free memory
    free(avg_fft_windowed);
    free(f);
    for (int i = 0; i < num_windows; i++)
    {
        free(fft_results_windowed[i]);
    }
    free(fft_results_windowed);
    free(in);
    free(out);
    free(cfg);
}

void AnalyzeWaveform(double harmonic_info[][3], int channel)
{
    int N = sample_points * AD_SAMP_CYCLE_NUMBER / 2; // 扩展后的数据长度
    int extended_data[N];

    // 读取数据
    int SecondHalfWave = AD_SAMP_CYCLE_NUMBER / 2; // 读取后半波形 调试
    for (int j = 0; j < AD_SAMP_CYCLE_NUMBER / 2; j++)
    {
        for (int i = 0; i < sample_points; i++)
        {

            extended_data[i + j * sample_points] = Xil_In32(SecondHalfWave * sample_points * CHANNL_MAX * 4 + Share_addr + channel * sample_points * 4 + j * sample_points * CHANNL_MAX * 4 + i * 4);
        }
    }
    //	//打印原始波形
    //	for(int i = 0;i < N;i++){
    //		printf("x=%d\n", extended_data[i]);
    //	}

    double filtered_data[N]; // 滤波后信号
    // 调用 FIR 滤波器
    fir_filter(extended_data, filtered_data, N, b, FILTER_ORDER);
    // 打印滤波波形
    //	for(int i = 0;i < N;i++){
    //		printf("x=%.1f\n", filtered_data[i]);
    //	}

    // 1 基波频率检测（使用最大似然法）
    double fundamental_frequency = interp_most_like_find_frequency(filtered_data, D_RANGE_MIN, D_RANGE_MAX, SAMPLE_RATE, N);
    // 输出找到的基波频率
    //    printf("Fundamental Frequency (via FindFrequence): %.4f Hz\n", *fundamental_frequency);

    // 2生成 Flat-Top 窗函数
    const int L_window = N / 2;
    const int fft_len = N / 2;
    double *flat_top_window = malloc(L_window * sizeof(double));
    generate_flat_top_window(flat_top_window, L_window);

    // 3加窗并执行 FFT
    perform_fft_with_window(extended_data, N, L_window, flat_top_window, 0.5, fft_len, MAX_HARMONICS, fundamental_frequency, SAMPLE_RATE, harmonic_info);

    free(flat_top_window);
}

// Normalize phase to the range [-π, π]
double normalize_phase(double phase)
{
    while (phase > M_PI)
    {
        phase -= 2 * M_PI; // Reduce to within [-π, π]
    }
    while (phase < -M_PI)
    {
        phase += 2 * M_PI; // Increase to within [-π, π]
    }
    return phase;
}

/**
 * @brief 计算给定中心点的幅值以及其左右各两个点的幅值之和
 *
 * 计算给定中心点以及其左右各两个点（共5个点）的幅值之和。
 * 需要注意的是，中心点及其周围点的索引必须在有效范围内。
 *
 * @param out FFT结果的复数数组
 * @param center_index 中心点的索引
 * @param N FFT结果的长度
 *
 * @return 中心点及其左右各两个点的幅值之和
 */
double calculate_magnitude_with_neighbors(kiss_fft_cpx *out, int center_index, int N)
{
    const int Compensating_points = 0;
    double total_magnitude = 0.0;

    // 加总中心点及左右各Compensating_points个点的幅值
    for (int i = center_index - Compensating_points; i <= center_index + Compensating_points; i++)
    {
        if (i > 0 && i < N / 2)
        { // 确保索引有效
            double magnitude = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
            total_magnitude += magnitude;
        }
    }

    return total_magnitude;
}

/**
 * @brief 分析交流电源波形
 *
 * 从指定DDR地址读取波形数据，计算直流分量，并进行FFT变换以分析基波和谐波信息。
 *
 * @param harmonic_info 一个二维数组，用于存储谐波信息。每个元素包含频率、幅值和相位（单位：弧度）。
 * @param channel 通道编号
 * @param ddr_addr DDR地址，用于读取波形数据
 * @param SampleFrequency 采样频率（单位：Hz）
 * @param fundamental_frequency 基波频率（单位：Hz）
 */
void AnalyzeWaveform_AcSource(double harmonic_info[][3], int channel, u32 ddr_addr,
                              int SampleFrequency, double fundamental_frequency)
{
    const int N = sample_points * AD_SAMP_CYCLE_NUMBER; // 扩展后的数据长度
    int extended_data[N];                               // 保存一个通道，16个周期的数据

    // 在读取DDR数据前先使缓存失效
    Xil_DCacheInvalidateRange((UINTPTR)ddr_addr, sample_points * 16 * CHANNL_MAX * AD_SAMP_CYCLE_NUMBER);

    // 从指定DDR地址读取数据
    for (int j = 0; j < AD_SAMP_CYCLE_NUMBER; j++)
    {
        for (int i = 0; i < sample_points; i++)
        {
            extended_data[i + j * sample_points] =
                Xil_In32(ddr_addr + channel * sample_points * 4 + j * sample_points * CHANNL_MAX * 4 + i * 4);
        }
    }

    // // 打印extended_data，用来测试波形是否正确
    // for (int i = 0; i < N; i++)
    // {
    //     printf("x=%d\n", extended_data[i]);
    // }
    //  计算直流分量（平均值）
    double dc_offset = 0.0;
    for (int i = 0; i < N; i++)
    {
        dc_offset += extended_data[i];
    }
    dc_offset /= N;

    // 使用kiss_fft库进行FFT计算
    kiss_fft_cfg cfg = kiss_fft_alloc(N, 0, NULL, NULL);
    kiss_fft_cpx *in = malloc(N * sizeof(kiss_fft_cpx));
    if (in == NULL)
    {
        printf("FFT input memory allocation failed\r\n");
        return;
    }
    kiss_fft_cpx *out = malloc(N * sizeof(kiss_fft_cpx));
    if (out == NULL)
    {
        printf("FFT output memory allocation failed\r\n");
        free(in);
        return;
    }

    // 准备FFT输入数据，并去除直流分量
    for (int i = 0; i < N; i++)
    {
        in[i].r = extended_data[i] - dc_offset; // 减去直流分量
        in[i].i = 0;                            // 虚部为零
    }

    // 执行FFT
    kiss_fft(cfg, in, out);

    // 频率分辨率
    double freq_res = (double)SampleFrequency / N;

    // 将基波频率存储在harmonic_info[0][0]
    harmonic_info[0][0] = fundamental_frequency;

    // 找到对应基波频率的索引
    int fundamental_index = (int)(fundamental_frequency / freq_res);

    // 在附近索引查找最大幅值（提高精度）
    double max_magnitude = 0;
    int best_index = fundamental_index;
    for (int i = fundamental_index - 2; i <= fundamental_index + 2; i++)
    {
        if (i > 0 && i < N / 2)
        { // 确保索引有效
            double magnitude = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
            if (magnitude > max_magnitude)
            {
                max_magnitude = magnitude;
                best_index = i;
            }
        }
    }
    fundamental_index = best_index;

    // 计算基波幅值(包括邻近点)
    double fundamental_magnitude = calculate_magnitude_with_neighbors(out, fundamental_index, N);

    // 归一化处理（考虑FFT的幅值缩放）
    fundamental_magnitude = fundamental_magnitude * 2.0 / N;

    // 初始化总幅值变量
    double total_magnitude = fundamental_magnitude;

    // 存储基波信息
    harmonic_info[0][1] = fundamental_magnitude;
    harmonic_info[0][2] = atan2(out[fundamental_index].i, out[fundamental_index].r) * 180.0 / M_PI;

    // 分析谐波
    for (int i = 1; i < 32; i++)
    {
        int index = fundamental_index * (i + 1);
        if (index >= N / 2)
            continue; // 防止索引越界

        double frequency = fundamental_frequency * (i + 1);
        double magnitude = calculate_magnitude_with_neighbors(out, index, N) * 2.0 / N; // (包括邻近点)
        double phase = atan2(out[index].i, out[index].r);

        harmonic_info[i][0] = frequency;
        harmonic_info[i][1] = magnitude;
        harmonic_info[i][2] = phase * 180.0 / M_PI;

        total_magnitude += magnitude;
    }

    // 释放内存
    kiss_fft_free(cfg);
    free(in);
    free(out);
}
