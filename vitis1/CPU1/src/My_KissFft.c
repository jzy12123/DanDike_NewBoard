
#include "My_kissFft.h"
#include <float.h> // For FLT_MAX
#define FILTER_ORDER 100
double b[FILTER_ORDER + 1] = {
    /* ��������MATLAB���ɵ�bϵ�� 100��fir�˲��� */
    0.0009, 0.0010, 0.0010, 0.0011, 0.0012, 0.0013, 0.0015, 0.0017, 0.0019, 0.0021, 0.0024, 0.0027, 0.0030, 0.0034,
    0.0038, 0.0042, 0.0046, 0.0051, 0.0056, 0.0061, 0.0066, 0.0072, 0.0077, 0.0083, 0.0089, 0.0095, 0.0101, 0.0107,
    0.0114, 0.0120, 0.0126, 0.0132, 0.0138, 0.0144, 0.0149, 0.0155, 0.0160, 0.0165, 0.0170, 0.0175, 0.0179, 0.0183,
    0.0186, 0.0189, 0.0192, 0.0194, 0.0196, 0.0198, 0.0199, 0.0200, 0.0200, 0.0200, 0.0199, 0.0198, 0.0196, 0.0194,
    0.0192, 0.0189, 0.0186, 0.0183, 0.0179, 0.0175, 0.0170, 0.0165, 0.0160, 0.0155, 0.0149, 0.0144, 0.0138, 0.0132,
    0.0126, 0.0120, 0.0114, 0.0107, 0.0101, 0.0095, 0.0089, 0.0083, 0.0077, 0.0072, 0.0066, 0.0061, 0.0056, 0.0051,
    0.0046, 0.0042, 0.0038, 0.0034, 0.0030, 0.0027, 0.0024, 0.0021, 0.0019, 0.0017, 0.0015, 0.0013, 0.0012, 0.0011,
    0.0010, 0.0010, 0.0009};

// FIR �˲�����
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

#define MAX_N 4096 // ���֧�ֵĵ���

// ���� M ����
void CalculateM(double *wave_data, int data_len, double *M)
{
    double h[MAX_N + 1], b[MAX_N + 1], c[MAX_N + 1], d[MAX_N + 1];
    double u[MAX_N + 1], v[MAX_N + 1], temp_y[MAX_N + 1];

    // ��Ȼ�߽�����
    M[0] = M[data_len - 1] = 0;

    // ���� h��b��c �� d
    for (int i = 0; i < data_len - 1; i++)
    {
        h[i] = 1.0; // ԭʼ���ݵ�Ⱦࣨx[i+1] - x[i] = 1.0��
    }
    for (int i = 1; i < data_len - 1; i++)
    {
        b[i] = h[i] / (h[i] + h[i - 1]);
        c[i] = 1 - b[i];
        d[i] = 6 * ((wave_data[i + 1] - wave_data[i]) / h[i] - (wave_data[i] - wave_data[i - 1]) / h[i - 1]) /
               (h[i] + h[i - 1]);
    }

    // ��׷�Ϸ����� M[i]
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

// ����������ֵ
double *interp1_spline(double *wave_data, int data_len, int factor, int *new_len)
{
    int interp_len = data_len * factor; // ��ֵ������ݳ���
    *new_len = interp_len;

    // �����ֵ�������
    double *interp_data = malloc(interp_len * sizeof(double));
    if (!interp_data)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // ���� M ����
    double M[MAX_N + 1];
    CalculateM(wave_data, data_len, M);

    // ��ֵ����
    for (int i = 0; i < interp_len; i++)
    {
        double pos = (double)i / factor; // ��ֵ���Ӧ��ԭʼ����λ��
        int k = (int)floor(pos);         // �ҵ� pos ���ڵ�����
        if (k >= data_len - 1)
        {
            k = data_len - 2; // ��ֹԽ��
        }

        double h = 1.0; // ԭʼ���ݵ�Ⱦࣨx[k+1] - x[k] = 1.0��
        double p = k + 1 - pos;
        double q = pos - k;
        interp_data[i] = (p * p * p * M[k] + q * q * q * M[k + 1]) / (6 * h) +
                         (p * wave_data[k] + q * wave_data[k + 1]) / h -
                         h * (p * M[k] + q * M[k + 1]) / 6;
    }

    return interp_data;
}

// ��ֵ���������Բ�ֵ
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

// ��ֵ�������̶��ȼ���ֵ
double *interp1_linear_fix(double *x, int x_len, int factor, int *new_len)
{
    // ��ֵ��ĵ���
    int interp_len = x_len * factor;
    *new_len = interp_len;

    // Ϊ��ֵ��������ڴ�
    double *x_interp = malloc(interp_len * sizeof(double));
    if (!x_interp)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    // ��ֵ����
    for (int i = 0; i < x_len - 1; i++)
    { // ����ÿ������
        double x1 = x[i];
        double x2 = x[i + 1];

        for (int j = 0; j < factor; j++)
        { // ��ÿ��������ֵ
            double fraction = (double)j / factor;
            x_interp[i * factor + j] = x1 + (x2 - x1) * fraction;
        }
    }

    // ���һ������Ҫ��������
    x_interp[(x_len - 1) * factor] = x[x_len - 1];

    return x_interp;
}

// �����Ȼ�����ƻ���Ƶ�ʣ��ӵڶ������ڿ�ʼ������10��Ƶ�ʲ�ȡƽ����
double interp_most_like_find_frequency(double *wave_data, int d_range_min, int d_range_max, int fs, int data_len)
{
    const int interp_factor = 10; // ��ֵ����
    int interp_len;
    double *interp_data = interp1_linear(wave_data, data_len, interp_factor, &interp_len);
    // ��ȡ�źŵĵڶ������ڿ�ʼ
    int segment_len = d_range_min * interp_factor; // һ�����ڵĳ���
    int start_index = segment_len;                 // �ӵڶ������ڿ�ʼ
    int num_cycles = 1;                            // ����1��Ƶ��
    double total_frequency = 0.0;

    // ȷ�����㹻�Ĳ�ֵ�������ڹ���10��
    if (start_index + num_cycles * segment_len > interp_len)
    {
        fprintf(stderr, "Error: Not enough data to estimate 10 cycles.\n");
        free(interp_data);
        return -1; // ���ش���
    }

    for (int cycle = 0; cycle < num_cycles; cycle++)
    {
        double *segment = malloc(segment_len * sizeof(double));
        memcpy(segment, interp_data + start_index + cycle * segment_len, segment_len * sizeof(double)); // �ӵ�ǰ���ڽ�ȡ

        double max_likelihood = -INFINITY;
        int best_d = 0;

        // ����d��Χ������Ȼ��
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

        // ���㵱ǰ���ڵ�Ƶ��
        double d_T = (double)best_d / interp_factor;
        double f_est = fs / d_T;

        total_frequency += f_est; // �ۼӹ���Ƶ��
        free(segment);
    }

    free(interp_data);

    // ����ƽ��Ƶ��
    return total_frequency / num_cycles;
}

// ����Flat-Top������
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
    int *data,                // �����ź�
    int data_len,             // �źų���
    int L_window,             // ����������
    double *flat_top_window,  // Flat-top ������
    double overlap_ratio,     // �ص���
    int fft_length,           // FFT ����
    int num_harmonics,        // ���г����
    double fundamental_freq,  // ����Ƶ��
    double Fs,                // ������
    double harmonic_info[][3] // ���ڴ洢г����Ϣ�Ķ�ά����
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
        // Zero-pad the remaining FFT input ʵ�ʲ�������
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
    int N = sample_points * AD_SAMP_CYCLE_NUMBER / 2; // ��չ������ݳ���
    int extended_data[N];

    // ��ȡ����
    int SecondHalfWave = AD_SAMP_CYCLE_NUMBER / 2; // ��ȡ��벨�� ����
    for (int j = 0; j < AD_SAMP_CYCLE_NUMBER / 2; j++)
    {
        for (int i = 0; i < sample_points; i++)
        {

            extended_data[i + j * sample_points] = Xil_In32(SecondHalfWave * sample_points * CHANNL_MAX * 4 + Share_addr + channel * sample_points * 4 + j * sample_points * CHANNL_MAX * 4 + i * 4);
        }
    }
    //	//��ӡԭʼ����
    //	for(int i = 0;i < N;i++){
    //		printf("x=%d\n", extended_data[i]);
    //	}

    double filtered_data[N]; // �˲����ź�
    // ���� FIR �˲���
    fir_filter(extended_data, filtered_data, N, b, FILTER_ORDER);
    // ��ӡ�˲�����
    //	for(int i = 0;i < N;i++){
    //		printf("x=%.1f\n", filtered_data[i]);
    //	}

    // 1 ����Ƶ�ʼ�⣨ʹ�������Ȼ����
    double fundamental_frequency = interp_most_like_find_frequency(filtered_data, D_RANGE_MIN, D_RANGE_MAX, SAMPLE_RATE, N);
    // ����ҵ��Ļ���Ƶ��
    //    printf("Fundamental Frequency (via FindFrequence): %.4f Hz\n", *fundamental_frequency);

    // 2���� Flat-Top ������
    const int L_window = N / 2;
    const int fft_len = N / 2;
    double *flat_top_window = malloc(L_window * sizeof(double));
    generate_flat_top_window(flat_top_window, L_window);

    // 3�Ӵ���ִ�� FFT
    perform_fft_with_window(extended_data, N, L_window, flat_top_window, 0.5, fft_len, MAX_HARMONICS, fundamental_frequency, SAMPLE_RATE, harmonic_info);

    free(flat_top_window);
}

// Normalize phase to the range [-��, ��]
double normalize_phase(double phase)
{
    while (phase > M_PI)
    {
        phase -= 2 * M_PI; // Reduce to within [-��, ��]
    }
    while (phase < -M_PI)
    {
        phase += 2 * M_PI; // Increase to within [-��, ��]
    }
    return phase;
}


/**
 * @brief ����������ĵ�ķ�ֵ�Լ������Ҹ�������ķ�ֵ֮��
 *
 * ����������ĵ��Լ������Ҹ������㣨��5���㣩�ķ�ֵ֮�͡�
 * ��Ҫע����ǣ����ĵ㼰����Χ���������������Ч��Χ�ڡ�
 *
 * @param out FFT����ĸ�������
 * @param center_index ���ĵ������
 * @param N FFT����ĳ���
 *
 * @return ���ĵ㼰�����Ҹ�������ķ�ֵ֮��
 */
double calculate_magnitude_with_neighbors(kiss_fft_cpx *out, int center_index, int N)
{
    const int Compensating_points = 0;
    double total_magnitude = 0.0;

    // �������ĵ㼰���Ҹ�Compensating_points����ķ�ֵ
    for (int i = center_index - Compensating_points; i <= center_index + Compensating_points; i++)
    {
        if (i > 0 && i < N / 2)
        { // ȷ��������Ч
            double magnitude = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
            total_magnitude += magnitude;
        }
    }

    return total_magnitude;
}

/**
 * @brief ����������Դ����
 *
 * ��ָ��DDR��ַ��ȡ�������ݣ�����ֱ��������������FFT�任�Է���������г����Ϣ��
 *
 * @param harmonic_info һ����ά���飬���ڴ洢г����Ϣ��ÿ��Ԫ�ذ���Ƶ�ʡ���ֵ����λ����λ�����ȣ���
 * @param channel ͨ�����
 * @param ddr_addr DDR��ַ�����ڶ�ȡ��������
 * @param SampleFrequency ����Ƶ�ʣ���λ��Hz��
 * @param fundamental_frequency ����Ƶ�ʣ���λ��Hz��
 */
void AnalyzeWaveform_AcSource(double harmonic_info[][3], int channel, u32 ddr_addr,
                              int SampleFrequency, double fundamental_frequency)
{
    const int N = sample_points * AD_SAMP_CYCLE_NUMBER; // ��չ������ݳ���
    int extended_data[N];                               // ����һ��ͨ����16�����ڵ�����

    // �ڶ�ȡDDR����ǰ��ʹ����ʧЧ
    Xil_DCacheInvalidateRange((UINTPTR)ddr_addr, sample_points * 16 * CHANNL_MAX * AD_SAMP_CYCLE_NUMBER);

    // ��ָ��DDR��ַ��ȡ����
    for (int j = 0; j < AD_SAMP_CYCLE_NUMBER; j++)
    {
        for (int i = 0; i < sample_points; i++)
        {
            extended_data[i + j * sample_points] =
                Xil_In32(ddr_addr + channel * sample_points * 4 + j * sample_points * CHANNL_MAX * 4 + i * 4);
        }
    }

    // ����ֱ��������ƽ��ֵ��
    double dc_offset = 0.0;
    for (int i = 0; i < N; i++)
    {
        dc_offset += extended_data[i];
    }
    dc_offset /= N;

    // ʹ��kiss_fft�����FFT����
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

    // ׼��FFT�������ݣ���ȥ��ֱ������
    for (int i = 0; i < N; i++)
    {
        in[i].r = extended_data[i] - dc_offset; // ��ȥֱ������
        in[i].i = 0;                            // �鲿Ϊ��
    }

    // ִ��FFT
    kiss_fft(cfg, in, out);

    // Ƶ�ʷֱ���
    double freq_res = (double)SampleFrequency / N;

    // ������Ƶ�ʴ洢��harmonic_info[0][0]
    harmonic_info[0][0] = fundamental_frequency;

    // �ҵ���Ӧ����Ƶ�ʵ�����
    int fundamental_index = (int)(fundamental_frequency / freq_res);

    // �ڸ���������������ֵ����߾��ȣ�
    double max_magnitude = 0;
    int best_index = fundamental_index;
    for (int i = fundamental_index - 2; i <= fundamental_index + 2; i++)
    {
        if (i > 0 && i < N / 2)
        { // ȷ��������Ч
            double magnitude = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
            if (magnitude > max_magnitude)
            {
                max_magnitude = magnitude;
                best_index = i;
            }
        }
    }
    fundamental_index = best_index;

    // ���������ֵ(�����ڽ���)
    double fundamental_magnitude = calculate_magnitude_with_neighbors(out, fundamental_index, N);

    // ��һ����������FFT�ķ�ֵ���ţ�
    fundamental_magnitude = fundamental_magnitude * 2.0 / N;

    // ��ʼ���ܷ�ֵ����
    double total_magnitude = fundamental_magnitude;

    // �洢������Ϣ
    harmonic_info[0][1] = fundamental_magnitude;
    harmonic_info[0][2] = atan2(out[fundamental_index].i, out[fundamental_index].r) * 180.0 / M_PI;

    // ����г��
    for (int i = 1; i < 32; i++)
    {
        int index = fundamental_index * (i + 1);
        if (index >= N / 2)
            continue; // ��ֹ����Խ��

        double frequency = fundamental_frequency * (i + 1);
        double magnitude = calculate_magnitude_with_neighbors(out, index, N) * 2.0 / N; // (�����ڽ���)
        double phase = atan2(out[index].i, out[index].r);

        harmonic_info[i][0] = frequency;
        harmonic_info[i][1] = magnitude;
        harmonic_info[i][2] = phase * 180.0 / M_PI;

        total_magnitude += magnitude;
    }

    // �ͷ��ڴ�
    kiss_fft_free(cfg);
    free(in);
    free(out);

    // // ���������Ϣ
    // printf("DC offset removed: %.2f, Fundamental freq: %.2f Hz, Amplitude: %.6f\r\n",
    //        dc_offset, fundamental_frequency, fundamental_magnitude);
}
