#include "ADDA.h"
#include "xscugic_hw.h"
#include "My_KissFft.h"
/*
 * 变量
 */
XAxiDma axidma;          // XAxiDma实例
XScuGic intc;            // 中断控制器的实例
XIntc AxiIntc_BareMetal; // AXI中断控制器实例
XScuTimer Timer;         // 定时器驱动程序实例

// AC分析专用缓冲区 (CPU1私有DDR，非共享)
int g_AcAnalysisData[CHANNL_MAX][FFT_DATA_LEN];

volatile u8 g_CurBufferIndex = 0;
volatile u8 g_ProcessBufferFlag = 0;

// ad
int dma_rx_8[8][sample_points] = {0}; // 8个通道，每个通道的采样点数sample_points
u16 *rx_buffer_ptr = (u16 *)RX_BUFFER_BASE;
u16 *tx_buffer_ptr = (u16 *)TX_BUFFER_BASE;

// 波形修改参数
float Phase_shift[8] = {0, -120, 120, 0, 0, -120, 120, 0}; // 8路波形相位偏移 单位度
uint16_t enable = 0xff;                                    // 使能通道输出
float Wave_Frequency = 50;
float Wave_Amplitude[8] = {0, 0, 0, 0, 0, 0, 0, 0};
u32 Wave_Range[8] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
uint16_t Wave_NewData[8][DATA_LEN]; // 修改后8路通道所有数据

int numHarmonics[CHANNL_MAX] = {0};                      // 每个通道有几个谐波
float harmonics[CHANNL_MAX][MAX_HARMONICS] = {0};        // 每个通道每次谐波的幅值
float harmonics_phases[CHANNL_MAX][MAX_HARMONICS] = {0}; // 每个通道每次谐波的相位

// 功放输出参数
double DA_Correct_100[8][3];
// 功放20%幅值时的校准参数
double DA_Correct_20[8][3];
// 相位校准参数数组（单位：度）
double DA_CorrectPhase_100[8][3];
// AD校准参数数组
double AD_Correct[8][3];

u64 g_LastDmaIrqTime_us = 0; // 【新增】记录上一次 DMA 中断的微秒时间戳

// 出厂设定参数
const double DA_CorrectConst_100[8][3] = {
    // Voltage channels (UA, UB, UC, UX) - for 6.5V, 3.25V, 1.876V
    {35740.445421, 35688.838937, 38472.249867}, // UA 111
    {35818.076424, 35754.250401, 38507.972835}, // UB 111
    {35740.654385, 35699.823498, 38492.768400}, // UC 111
    {35740.654385, 35699.823498, 38472.249867}, // UX 000

    // Current channels (IA, IB, IC, IX) - for 5A, 1A, 0.2A
    {35471.388977, 41083.116459, 41370.352427}, // IA 111
    {35451.349561, 41026.379569, 41089.986359}, // IB 111
    {35455.531774, 41026.379569, 41164.529778}, // IC 111
    {35462.627137, 41026.379569, 40960.744977}  // IX 111
};
// 功放20%幅值时的校准参数
const double DA_CorrectConst_20[8][3] = {
    // 电压通道 (UA, UB, UC, UX) - 分别对应 6.5V, 3.25V, 1.876V
    {35731.608731, 35708.515107, 38471.090857}, // UA 20%幅值校准参数
    {35789.473684, 35806.869734, 38471.090857}, // UB 20%幅值校准参数
    {35731.608731, 35702.746365, 38503.990878}, // UC 20%幅值校准参数
    {35731.608731, 35702.746365, 38503.990878}, // UX 20%幅值校准参数

    // 电流通道 (IA, IB, IC, IX) - 分别对应 5A, 1A, 0.2A
    {35453.597497, 41052.631579, 41935.483871}, // IA 20%幅值校准参数
    {35789.473684, 41031.036297, 41935.483871}, // IB 20%幅值校准参数
    {35453.597497, 41052.631579, 41379.310345}, // IC 20%幅值校准参数
    {35453.597497, 41052.631579, 41379.310345}  // IX 20%幅值校准参数
};

// 相位校准参数数组（单位：度）
const double DA_CorrectPhaseConst_100[8][3] = {
    // 电压通道 (UA, UB, UC, UX) - 分别对应 6.5V, 3.25V, 1.876V
    {0.0, 0.0, 0.0},          // UA 相位校准参数
    {-0.017, -0.006, -0.001}, // UB 相位校准参数
    {-0.012, -0.006, -0.003}, // UC 相位校准参数
    {0.0, 0.0, 0.0},          // UX 相位校准参数

    // 电流通道 (IA, IB, IC, IX) - 分别对应 5A, 1A, 0.2A
    {-0.205, -0.310, -0.184}, // IA 相位校准参数
    {-0.204, -0.304, -0.177}, // IB 相位校准参数
    {-0.195, -0.293, -0.166}, // IC 相位校准参数
    {0.0, 0.0, 0.0}           // IX 相位校准参数
};
// AD校准参数数组
const double ADConst_Correct[8][3] = {
    // 电压通道 (UA, UB, UC, UX) - 分别对应 6.5V, 3.25V, 1.876V
    {20171.482379, 20159.124764, 21776.522459}, // UA 111
    {20197.562831, 20178.557506, 21775.366372}, // UB 111
    {20155.068093, 20142.372400, 21769.351245}, // UC 111
    {20155.068093, 20142.372400, 21769.351245}, // UX 000

    // 电流通道 (IA, IB, IC, IX) - 分别对应 5A, 1A, 0.2A
    {20029.339034, 23237.211085, 23485.098698}, // IA 111
    {20045.211964, 23237.211085, 23355.930655}, // IB 111
    {20026.621825, 23211.482920, 23379.357041}, // IC 111
    {20026.621825, 23211.482920, 23250.247711}  // IX 111
};

void Adc_Continuous_Start(void)
{
    g_CurBufferIndex = 0;
    g_ProcessBufferFlag = 0;

    // 1. 设置ADC分频与点数
    Xil_Out32(adc_whole_base_addr + 4, 1953);
    Xil_Out32(adc_whole_base_addr + 8, POINTS_PER_BLOCK);

    // 2. 启动第一次DMA (Ping Buffer)
    // 【关键修改】使用硬编码地址
    UINTPTR PingAddr = RX_BUFFER_PING_ADDR;

    // 刷新 Cache，确保 CPU 不会把 Cache 里的脏数据写回覆盖 DMA 的数据
    Xil_DCacheInvalidateRange(PingAddr, DMA_BUFFER_SIZE);

    // 使用 SimpleTransfer 启动 S2MM
    int status = XAxiDma_SimpleTransfer(&axidma, PingAddr,
                                        DMA_BUFFER_SIZE, XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS)
    {
        xil_printf("CPU1: DMA Start Failed! Status=%d\r\n", status);
        return;
    }

    // 3. 启动ADC
    Xil_Out32(adc_whole_base_addr + 0, 1);

    xil_printf("CPU1: ADC Continuous Mode Started.\r\n");
}

// 对于DMA缓冲区，使用更强的缓存同步
void sync_dma_buffer(UINTPTR addr, size_t size, int direction)
{
    if (direction == XAXIDMA_DMA_TO_DEVICE)
    {
        // CPU写入，DMA读取
        Xil_DCacheFlushRange(addr, size);
    }
    else
    {
        // 先刷新，再失效
        Xil_DCacheFlushRange(addr, size);
        Xil_DCacheInvalidateRange(addr, size);
    }

    // 添加内存屏障确保操作顺序
    __asm__ __volatile__("dsb sy" : : : "memory");
}

/**
 * @brief DMA 传输启动（带重试机制，ISR 安全版）
 * @details 模仿 SafeDmaTransfer，但在失败时进行微秒级等待，避免 ISR 阻塞太久
 */
static int ISR_Safe_DmaTransfer(XAxiDma *AxiDmaInstPtr, UINTPTR BuffAddr, u32 Length, int Direction)
{
    int retry_count = 0;
    int max_retries = 10; // 增加重试次数
    int status = XST_FAILURE;

    while (retry_count < max_retries)
    {
        status = XAxiDma_SimpleTransfer(AxiDmaInstPtr, BuffAddr, Length, Direction);

        if (status == XST_SUCCESS)
        {
            return XST_SUCCESS;
        }

        // 失败后短暂等待，让总线或状态机喘口气
        // 在 666MHz CPU 下，usleep(10) 是忙等待，不会调度，适合 ISR
        usleep(10);
        retry_count++;
    }

    return status;
}

/**
 * @brief DMA 故障恢复函数 (修正版)
 * @details 复位 -> 清除错误 -> 重新提交 (让 SimpleTransfer 负责启动)
 */
void Adc_Dma_Reset_And_Restart(void)
{
    u32 RxBaseAddr = axidma.RegBase + XAXIDMA_RX_OFFSET;

    // 1. 复位 DMA 引擎
    XAxiDma_Reset(&axidma);
    int timeout = 10000;
    while (!XAxiDma_ResetIsDone(&axidma) && timeout > 0)
    {
        timeout--;
    }

    if (timeout == 0)
    {
        xil_printf("CPU1: Critical - DMA Reset Timeout!\r\n");
        return;
    }

    // 2. 显式清除 SR 中的错误位 (通过写 1 清除)
    // SR 偏移 0x04. 错误位在 bit 4,5,6. Halted 在 bit 0.
    // 读出当前 SR，并对错误位写 1
    u32 sr = XAxiDma_ReadReg(RxBaseAddr, XAXIDMA_SR_OFFSET);
    XAxiDma_WriteReg(RxBaseAddr, XAXIDMA_SR_OFFSET, sr | 0x70);

    // 3. 重新启用中断 (关键！复位后中断被禁用，必须重新使能)
    // 注意：这里只写中断使能位，【不要】写 XAXIDMA_CR_RUNSTOP_MASK (bit 0)
    // 让 SimpleTransfer 函数去负责置位 Run/Stop，这样它不会误判为 Busy
    u32 cr = XAxiDma_ReadReg(RxBaseAddr, XAXIDMA_CR_OFFSET);
    cr |= (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK);
    XAxiDma_WriteReg(RxBaseAddr, XAXIDMA_CR_OFFSET, cr);

    // 4. 准备目标地址
    UINTPTR TargetAddr = (g_CurBufferIndex == 0) ? RX_BUFFER_PING_ADDR : RX_BUFFER_PONG_ADDR;

    // Invalidate Cache
    Xil_DCacheInvalidateRange(TargetAddr, DMA_BUFFER_SIZE);

    // 5. 提交传输
    // SimpleTransfer 会检测到 DMA 处于 Halted 状态，并自动启动它
    int status = ISR_Safe_DmaTransfer(&axidma, TargetAddr, DMA_BUFFER_SIZE, XAXIDMA_DEVICE_TO_DMA);

    if (status == XST_SUCCESS)
    {
        // xil_printf("CPU1: DMA Recovered! (Addr: 0x%X)\r\n", TargetAddr);
    }
    else
    {
        // 如果依然失败，打印详细 SR 供分析
        u32 sr_after = XAxiDma_ReadReg(RxBaseAddr, XAXIDMA_SR_OFFSET);
        xil_printf("CPU1: DMA Recovery Failed (Status: %d, SR: 0x%08X)\r\n", status, sr_after);
    }
}

/**
 * @brief 获取当前精确的微秒级时间戳
 * @note  基于 FPGA 10MHz 软时钟，curr_subsec 单位为 0.1us
 */
u64 Get_Current_Time_US(void)
{
    In_CurrTime curr;
    // 从 FPGA 寄存器读取时间 (slv_reg0 ~ slv_reg7)
    read_current_time(&curr);

    // 【修正点】硬件 curr_subsec 是 10MHz 计数 (0.1us/tick)
    u64 subsec_us = (u64)curr.curr_subsec / 10;

    // 计算总微秒数 (忽略日期，仅计算当天的时分秒，足以处理短时间差)
    u64 total_us = (u64)curr.curr_hour * 3600000000ULL + // 3600 * 10^6
                   (u64)curr.curr_minute * 60000000ULL + // 60 * 10^6
                   (u64)curr.curr_second * 1000000ULL +  // 1 * 10^6
                   subsec_us;

    return total_us;
}

/**
 * @brief DMA RX中断处理 (保持之前的逻辑，调用增强后的恢复函数)
 */
void rx_intr_handler(void *callback)
{
    XAxiDma *axidma_inst = (XAxiDma *)callback;
    u32 irq_status;

    // 1. 读取并清除中断
    irq_status = XAxiDma_IntrGetIrq(axidma_inst, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrAckIrq(axidma_inst, irq_status, XAXIDMA_DEVICE_TO_DMA);

    // 2. 错误检测与恢复
    u32 sr = XAxiDma_ReadReg(axidma_inst->RegBase + XAXIDMA_RX_OFFSET, XAXIDMA_SR_OFFSET);

    // 如果有 Error 位 (bit 4,5,6) 或者 Halted (bit 0) 且不是正常的 idle
    // 注意：正常传输完成瞬间 Halted 可能会短暂置 1，所以主要看 Error
    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK) || (sr & 0x70))
    {
        // xil_printf("ISR: DMA Error! SR=0x%08X. Recovering...\r\n", sr);
        Adc_Dma_Reset_And_Restart();
        return;
    }

    // 3. 正常传输完成
    if (irq_status & XAXIDMA_IRQ_IOC_MASK)
    {
        // 立即记录当前块结束的时刻,用来剔除没用的波形
        g_LastDmaIrqTime_us = Get_Current_Time_US();

        // 切换 Buffer
        u8 nextBufferIndex = 1 - g_CurBufferIndex;
        UINTPTR nextBufferAddr = (nextBufferIndex == 0) ? RX_BUFFER_PING_ADDR : RX_BUFFER_PONG_ADDR;

        Xil_DCacheInvalidateRange(nextBufferAddr, DMA_BUFFER_SIZE);

        // 使用带重试的提交，防止偶发 Busy
        int status = ISR_Safe_DmaTransfer(axidma_inst, nextBufferAddr, DMA_BUFFER_SIZE, XAXIDMA_DEVICE_TO_DMA);

        if (status != XST_SUCCESS)
        {
            xil_printf("ISR: Submit Failed (%d), Resetting...\r\n", status);
            Adc_Dma_Reset_And_Restart();
        }
        else
        {
            g_ProcessBufferFlag = (g_CurBufferIndex == 0) ? 1 : 2;
            g_CurBufferIndex = nextBufferIndex;
        }
    }
}

/**
 * @brief ADC数据预处理 (解交错 + 4抽1)
 * @param pRawSrc 指向包含 500ms 数据的原始缓冲区
 */
static void Adc_Data_Preprocessing(u16 *pRawSrc)
{
    // 我们需要取最后 16 个周波的数据进行 FFT
    // 原始采样率 51200Hz, 16个周波 @ 50Hz = 320ms
    // 点数 = 16 * (51200/50) = 16384 点 (原始点)
    // 缓冲区总点数 POINTS_PER_BLOCK = 25600
    // 起始索引 = 25600 - 16384 = 9216

#define RAW_POINTS_NEEDED (FFT_ANALYSIS_CYCLES * (FS_RATE / 50)) // 16384
#define RAW_START_INDEX (POINTS_PER_BLOCK - RAW_POINTS_NEEDED)

    // 4抽1逻辑
    int dest_idx = 0;

    // 遍历原始数据，步长为4 (降采样)
    for (int i = RAW_START_INDEX; i < POINTS_PER_BLOCK; i += 4)
    {
        // pRawSrc 排列: [Ch0][Ch1]...[Ch7] [Ch0][Ch1]...
        u16 *pSampleBase = &pRawSrc[i * CHN_NUM];

        for (int ch = 0; ch < CHN_NUM; ch++)
        {
            // 物理通道映射 (根据原 Adc_Data_processing 逻辑)
            // 物理: IA(0), UA(1), IB(2), UB(3), IC(4), UC(5), IX(6), UX(7)
            // 逻辑: UA(0), UB(1), UC(2), UX(3), IA(4), IB(5), IC(6), IX(7)

            u16 raw_val = 0;
            switch (ch)
            {
            case 0:
                raw_val = pSampleBase[1];
                break; // UA
            case 1:
                raw_val = pSampleBase[3];
                break; // UB
            case 2:
                raw_val = pSampleBase[5];
                break; // UC
            case 3:
                raw_val = pSampleBase[7];
                break; // UX
            case 4:
                raw_val = pSampleBase[0];
                break; // IA
            case 5:
                raw_val = pSampleBase[2];
                break; // IB
            case 6:
                raw_val = pSampleBase[4];
                break; // IC
            case 7:
                raw_val = pSampleBase[6];
                break; // IX
            }

            // 转换并存入 CPU1 私有数组 g_AcAnalysisData
            g_AcAnalysisData[ch][dest_idx] = code_to_real(raw_val);
        }

        dest_idx++;
        if (dest_idx >= FFT_DATA_LEN)
            break; // 应该正好是 4096
    }
}
/**
 * @brief 主循环处理入口
 */
void Process_ADC_Buffer(void)
{
    // 1. 检查标志位
    if (g_ProcessBufferFlag == 0)
        return;

    // 2. 锁定缓冲区物理地址
    UINTPTR SrcAddr = (g_ProcessBufferFlag == 1) ? RX_BUFFER_PING_ADDR : RX_BUFFER_PONG_ADDR;
    u16 *pSrcBuffer = (u16 *)SrcAddr;

    // 3. Cache 失效 (数据一致性防线)
    Xil_DCacheInvalidateRange(SrcAddr, DMA_BUFFER_SIZE);

    g_ProcessBufferFlag = 0; // 清除标志，允许下一轮中断置位

    // ============================================================
    // 分支 1: 录波处理 (无条件 / 独立控制)
    // ============================================================
    // 录波模块内部有 if(isRecording) 判断，所以这里无条件调用即可。
    // 即使源停止了，如果用户强行开了录波，也能录到底噪，这符合录波逻辑。
    WaveRecord_Process(pSrcBuffer, POINTS_PER_BLOCK);

    // ============================================================
    // 分支 2: 常规 AC 分析 (FFT + PID)
    // ============================================================

    // 条件A: 系统处于“运行”或“暂停”状态
    // nStatus: 0=Stop, 1=Run, 2=Pause. 只要不全为0，就说明需要观测数据
    bool any_active = (devState.nStatusFund != 0) || (devState.nStatusHarm != 0) || (devState.nStatusInharm != 0);
    // 条件B: 状态序列未运行
    bool seq_idle = !g_StateSeqRuntime.IsRunning;

    // 只有同时满足：(有组件激活) 且 (非状态序列模式) 且 (拿到ADC锁)
    if (any_active && seq_idle)
    {
        if (acquire_resource_lock(LOCK_OWNER_ADC, MUTEX_ADC_ACQUIRE_TIMEOUT_US))
        {
            // 解交错并抽值 (400KB -> 16周波数据)
            Adc_Data_Preprocessing(pSrcBuffer);

            // 运行 FFT 计算与 PID 调整
            RunADCPIDCycle();

            release_resource_lock(LOCK_OWNER_ADC);
        }
    }
    // else: 如果全停止，什么都不做，数据丢弃，保持静默，符合原逻辑
}

void RunADCPIDCycle(void)
{
    // 重置计算值
    double Phase_reference = 0; // 定义相位基准
    double calculated_total_p = 0.0;
    double calculated_total_q = 0.0;
    double calculated_total_pf = 0.0;

    // 12800Hz 采样率 (4抽1后)
    int eff_sample_rate = FS_RATE / 4;

    // 循环处理4个通道（A, B, C, X），但只累加前3个通道的总功率
    for (int i = 0; i < 4; i++)
    {
        // 分析FFT
        double harmonic_info_U[HarmNumberMax][3] = {0}; // 创建用于存储谐波的数组
        double harmonic_info_I[HarmNumberMax][3] = {0};

        // 调用更新后的 FFT 分析函数，传入本地缓冲区指针
        AnalyzeWaveform_AcSource(harmonic_info_U, g_AcAnalysisData[i], eff_sample_rate, Wave_Frequency);
        AnalyzeWaveform_AcSource(harmonic_info_I, g_AcAnalysisData[i + 4], eff_sample_rate, Wave_Frequency);

        if (i == 0)
        {
            // 定义相位基准
            Phase_reference = harmonic_info_U[0][2];
        }

        // --- 填充 lineAC 结构体 ---
        // 获取电压和电流量程索引
        int idx_u = get_voltage_index_by_value(setACS.Vals[i].UR);
        int idx_i = get_current_index_by_value(setACS.Vals[i].IR);

        lineAC.f[i] = harmonic_info_U[0][0]; // 频率 (基波)
        lineAC.ur[i] = setACS.Vals[i].UR;    // 电压档位
        lineAC.ir[i] = setACS.Vals[i].IR;    // 电流档位

        // 计算 RMS (电压)
        double sum_of_squares_u_rms = 0.0;
        for (int h = 0; h < g_harm_number_thd; h++) // 遍历所有谐波分量（包括基波）
        {
            double corrected_u_amp = harmonic_info_U[h][1] / AD_Correct[i][idx_u] * setACS.Vals[i].UR;
            sum_of_squares_u_rms += corrected_u_amp * corrected_u_amp;
        }
        lineAC.u[i] = sqrt(sum_of_squares_u_rms); // U[ChnsAC] //总有效值

        // 计算 RMS (电流)
        double sum_of_squares_i_rms = 0.0;
        for (int h = 0; h < g_harm_number_thd; h++) // 遍历所有谐波分量（包括基波）
        {
            double corrected_i_amp = (harmonic_info_I[h][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR;
            sum_of_squares_i_rms += corrected_i_amp * corrected_i_amp;
        }
        lineAC.i[i] = sqrt(sum_of_squares_i_rms); // I[ChnsAC] //总有效值
        /*******************************************************************************************/

        lineAC.phu[i] = harmonic_info_U[0][2] - Phase_reference; // 电压相位 角度制（UA为参考, 依然是基波相位）
        if (lineAC.phu[i] < 0)
        {
            lineAC.phu[i] += 360;
        }
        lineAC.phi[i] = harmonic_info_I[0][2] - Phase_reference; // 电流相位（UA为参考, 依然是基波相位）
        if (lineAC.phi[i] < 0)
        {
            lineAC.phi[i] += 360;
        }

        // 初始化总谐波畸变率变量
        double thdu = 0.0;
        double thdi = 0.0;
        double baseU_for_thd = (harmonic_info_U[0][1] / AD_Correct[i][idx_u]) * setACS.Vals[i].UR;
        double baseI_for_thd = (harmonic_info_I[0][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR;

        // 计算电压总谐波畸变率 (THDU)
        if (baseU_for_thd >= 0.0001)
        { // 避免除以零
            double sum_of_squares_u_thd = 0.0;
            // 遍历从2次谐波到指定次数谐波
            for (int h = 1; h < g_harm_number_thd; h++)
            {
                double corrected_u_amp = harmonic_info_U[h][1] / AD_Correct[i][idx_u] * setACS.Vals[i].UR;
                sum_of_squares_u_thd += corrected_u_amp * corrected_u_amp;
            }
            thdu = sqrt(sum_of_squares_u_thd) / baseU_for_thd;
        }
        else
        {
            thdu = 0.0;
        }
        // 计算电流总谐波畸变率 (THDI)
        if (baseI_for_thd >= 0.0001)
        { // 避免除以零
            double sum_of_squares_i_thd = 0.0;
            // 遍历从2次谐波到指定次数谐波
            for (int h = 1; h < g_harm_number_thd; h++)
            {
                double corrected_i_amp = (harmonic_info_I[h][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR;
                sum_of_squares_i_thd += corrected_i_amp * corrected_i_amp;
            }
            thdi = sqrt(sum_of_squares_i_thd) / baseI_for_thd;
        }
        else
        {
            thdi = 0.0;
        }
        // 保存结果
        lineAC.thdu[i] = thdu * 100.0;
        lineAC.thdi[i] = thdi * 100.0;

        /*lineHarm*/
        // 初始化该通道的总功率累加变量
        lineHarm.harm[i].totalP = 0.0;
        lineHarm.harm[i].totalQ = 0.0;

        // 存储基波幅值和相位，用于计算百分比和相对相位
        double baseU_raw = harmonic_info_U[0][1];
        double baseI_raw = harmonic_info_I[0][1];

        // 填充直流分量（索引0）
        lineHarm.harm[i].u[0] = 0.0;   // 直流电压（暂设为0）
        lineHarm.harm[i].i[0] = 0.0;   // 直流电流（暂设为0）
        lineHarm.harm[i].phu[0] = 0.0; // 直流相位（直流无相位）
        lineHarm.harm[i].phi[0] = 0.0; // 直流相位（直流无相位）
        lineHarm.harm[i].p[0] = 0.0;   // 直流有功功率
        lineHarm.harm[i].q[0] = 0.0;   // 直流无功功率（直流无无功）

        // 填充各次谐波
        for (int j = 1; j < HarmNumberMax; j++)
        {
            // 电压和电流幅值处理
            if (j == 1)
            {
                // 基波(索引1)特殊处理：使用实际幅值
                lineHarm.harm[i].u[j] = (harmonic_info_U[j - 1][1] / AD_Correct[i][idx_u]) * setACS.Vals[i].UR;
                lineHarm.harm[i].i[j] = (harmonic_info_I[j - 1][1] / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR;

                // 基波相位直接采用相对于参考相位的值
                lineHarm.harm[i].phu[j] = harmonic_info_U[j - 1][2] - Phase_reference;
                lineHarm.harm[i].phi[j] = harmonic_info_I[j - 1][2] - Phase_reference;
            }
            else
            {
                // 谐波(索引2及以上)：u/i计算为基波的百分比
                if (baseU_raw > 0.0001)
                {
                    lineHarm.harm[i].u[j] = (harmonic_info_U[j - 1][1] / baseU_raw) * 100.0;
                }
                else
                {
                    lineHarm.harm[i].u[j] = 0.0;
                }

                if (baseI_raw > 0.0001)
                {
                    lineHarm.harm[i].i[j] = (harmonic_info_I[j - 1][1] / baseI_raw) * 100.0;
                }
                else
                {
                    lineHarm.harm[i].i[j] = 0.0;
                }

                // 谐波相位计算
                double n = j;
                double u_relative_phase = harmonic_info_U[j - 1][2] - n * Phase_reference;
                double i_relative_phase = harmonic_info_I[j - 1][2] - n * Phase_reference;
                lineHarm.harm[i].phu[j] = fmod(u_relative_phase + 360.0, 360.0);
                lineHarm.harm[i].phi[j] = fmod(i_relative_phase + 360.0, 360.0);

                switch ((j - 1) % 4)
                {
                case 0:
                    lineHarm.harm[i].phu[j] -= 0.0;
                    lineHarm.harm[i].phi[j] -= 0.0;
                    break;
                case 1:
                    lineHarm.harm[i].phu[j] -= 270.0;
                    lineHarm.harm[i].phi[j] -= 270.0;
                    break;
                case 2:
                    lineHarm.harm[i].phu[j] -= 180.0;
                    lineHarm.harm[i].phi[j] -= 180.0;
                    break;
                case 3:
                    lineHarm.harm[i].phu[j] -= 90.0;
                    lineHarm.harm[i].phi[j] -= 90.0;
                    break;
                }
                lineHarm.harm[i].phu[j] = fmod(lineHarm.harm[i].phu[j], 360.0);
                if (lineHarm.harm[i].phu[j] < 0)
                {
                    lineHarm.harm[i].phu[j] += 360;
                }
                lineHarm.harm[i].phi[j] = fmod(lineHarm.harm[i].phi[j], 360.0);
                if (lineHarm.harm[i].phi[j] < 0)
                {
                    lineHarm.harm[i].phi[j] += 360;
                }
            }

            // 计算谐波的相位差（角度）
            double h_phase_diff = lineHarm.harm[i].phu[j] - lineHarm.harm[i].phi[j];

            // 计算谐波的有功和无功功率（P/Q按幅值表示）
            if (j == 1)
            {
                // 基波：直接用实际幅值计算功率
                lineHarm.harm[i].p[j] = lineHarm.harm[i].u[j] * lineHarm.harm[i].i[j] * cos(h_phase_diff * M_PI / 180.0);
                lineHarm.harm[i].q[j] = lineHarm.harm[i].u[j] * lineHarm.harm[i].i[j] * sin(h_phase_diff * M_PI / 180.0);
            }
            else
            {
                // 谐波：需要将百分比转换回实际幅值来计算功率
                double actual_u_h = (lineHarm.harm[i].u[j] / 100.0) * ((baseU_raw / AD_Correct[i][idx_u]) * setACS.Vals[i].UR);
                double actual_i_h = (lineHarm.harm[i].i[j] / 100.0) * ((baseI_raw / AD_Correct[i + 4][idx_i]) * setACS.Vals[i].IR);
                lineHarm.harm[i].p[j] = actual_u_h * actual_i_h * cos(h_phase_diff * M_PI / 180.0);
                lineHarm.harm[i].q[j] = actual_u_h * actual_i_h * sin(h_phase_diff * M_PI / 180.0);
            }

            // 累加到总功率
            lineHarm.harm[i].totalP += lineHarm.harm[i].p[j];
            lineHarm.harm[i].totalQ += lineHarm.harm[i].q[j];
        }

        // 中文注释: 使用 lineHarm 中已正确计算的各谐波功率之和来更新 lineAC 中的总功率
        lineAC.p[i] = lineHarm.harm[i].totalP;
        lineAC.q[i] = lineHarm.harm[i].totalQ;

        // 中文注释: 计算该通道的总视在功率 S = sqrt(P^2 + Q^2)
        double apparent_power_s = sqrt(lineAC.p[i] * lineAC.p[i] + lineAC.q[i] * lineAC.q[i]);

        // 中文注释: 计算该通道的总功率因数 PF = P / S
        if (apparent_power_s > 1e-6) // 避免除零
        {
            lineAC.pf[i] = lineAC.p[i] / apparent_power_s;
        }
        else
        {
            lineAC.pf[i] = 0.0;
        }

        // 中文注释: 累加前三个通道(A, B, C)的功率，现在使用的是正确的总谐波功率
        if (i < 3)
        {
            calculated_total_p += lineAC.p[i];
            calculated_total_q += lineAC.q[i];
        }
    }

    // 总功率因数，使用局部变量完成所有相关计算
    double totalApparentPower = sqrt(calculated_total_p * calculated_total_p + calculated_total_q * calculated_total_q);
    if (totalApparentPower > 0.0001)
    {
        calculated_total_pf = calculated_total_p / totalApparentPower;
    }
    else
    {
        calculated_total_pf = 0.0;
    }

    // 在所有计算完成后，将最终结果“发布”到全局变量和中断安全的“影子”变量。
    lineAC.totalP = calculated_total_p;
    lineAC.totalQ = calculated_total_q;
    lineAC.totalPF = calculated_total_pf;

    g_safe_total_p_for_isr = calculated_total_p;
    g_safe_total_q_for_isr = calculated_total_q;

    // 输出电能脉冲
    PowerPulse_UpdateOutput(lineAC.totalP, lineAC.totalQ);

    // 标记UDP数据已更新
    udp_data_changed_flag = true;

    // [核心修改]
    // 只有当状态序列 [既不运行] [也不保持] 时，才允许 ADC 中断触发稳态输出刷新。
    // 这样在状态序列结束后(Holding状态)，主循环虽然在跑ADC，但不会去写 str_wr_bram，
    // 从而保证了输出波形绝对静止，直到新的 SetACS 指令打破 Holding 状态。
    if (!g_StateSeqRuntime.IsRunning && !g_StateSeqRuntime.IsHolding)
    {
        dac_parameters_updated_by_command = true;
    }
}


// DMA TX中断处理函数 dac
void tx_intr_handler(void *callback)
{
    int timeout;
    u32 irq_status;
    XAxiDma *axidma_inst = (XAxiDma *)callback;

    // 读取待处理的中断
    irq_status = XAxiDma_IntrGetIrq(axidma_inst, XAXIDMA_DMA_TO_DEVICE);
    // 确认待处理的中断
    XAxiDma_IntrAckIrq(axidma_inst, irq_status, XAXIDMA_DMA_TO_DEVICE);

    // Tx出错
    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK))
    {

        XAxiDma_Reset(axidma_inst);
        timeout = RESET_TIMEOUT_COUNTER;
        while (timeout)
        {
            if (XAxiDma_ResetIsDone(axidma_inst))
                break;
            timeout -= 1;
        }
        return;
    }

    // Tx完成
    if ((irq_status & XAXIDMA_IRQ_IOC_MASK))
    {
    }
}

// DAC FIFO溢：读空fifo后继续读则导致下溢
void underflow_handler()
{
    // xil_printf("underflow\r\n");

    //	Copy_Wave_to_tx_buffer_ptr();
    sync_dma_buffer((UINTPTR)tx_buffer_ptr, DATA_LEN * 16, XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_SimpleTransfer(&axidma, (UINTPTR)tx_buffer_ptr, DATA_LEN * 16, XAXIDMA_DMA_TO_DEVICE);
}

// 定时器中断处理函数
void timer_intr_handler(void *CallBackRef)
{
    XScuTimer *timer_ptr = (XScuTimer *)CallBackRef;
    /*1 消息队列*/
    // 读取消息队列
    char buffer[MAX_DATA_LEN];
    ssize_t bytesRead = MsgQue_read(buffer, sizeof(buffer));
    if (bytesRead > 0)
    {
        // 解析JSON指令
        Parse_JsonCommand(buffer);
    }

    /*2 回报UDP结构体*/
    ReportUDP_Structure(reportStatus);

    /*3 读故障信号*/
    RdSerial(); // 读取并处理硬件故障信号

    /* 4. 调用新的对时任务周期处理器 */
    TimeSync_TickHandler();

    // =========================================================
    // [新增] 5. 检查并上报事务性任务状态 (电能测试 & 状态序列)
    // =========================================================
    check_and_report_energy_test_status();
    check_and_report_state_sequence_status();
    // 清除定时器中断标志
    XScuTimer_ClearInterruptStatus(timer_ptr);
}

// 定时器初始化程序
int timer_init(XScuTimer *timer_ptr)
{
    int status;
    // 私有定时器初始化
    XScuTimer_Config *timer_cfg_ptr;
    timer_cfg_ptr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
    if (NULL == timer_cfg_ptr)
        return XST_FAILURE;
    status = XScuTimer_CfgInitialize(timer_ptr, timer_cfg_ptr, timer_cfg_ptr->BaseAddr);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    XScuTimer_LoadTimer(timer_ptr, TIMER_LOAD_VALUE); // 加载计数周期
    XScuTimer_EnableAutoReload(timer_ptr);            // 设置自动装载模式

    return XST_SUCCESS;
}

/**
 * @brief 强制设置一个SPI中断的目标CPU。
 */
static void ScuGic_SetInterruptTarget(u32 DistBaseAddress, u32 Int_Id, u8 Cpu_Id)
{
    u32 RegValue;
    u32 Offset;
    u8 TargetCpuMask = (u8)(0x01 << Cpu_Id);

    // 计算目标寄存器的偏移量
    Offset = XSCUGIC_SPI_TARGET_OFFSET + ((Int_Id / 4U) * 4U);

    // 读取-修改-写入
    RegValue = Xil_In32(DistBaseAddress + Offset);
    RegValue &= ~(0xFFU << ((Int_Id % 4U) * 8U));
    RegValue |= (u32)TargetCpuMask << ((Int_Id % 4U) * 8U);
    Xil_Out32(DistBaseAddress + Offset, RegValue);
}
/**
 * @brief 初始化并配置中断控制器和中断处理函数（AMP安全最终版）
 * @details 此版本修正了初始化顺序，以防止在中断处理程序就绪前发生中断导致系统挂起。
 * @return 成功返回XST_SUCCESS，失败返回XST_FAILURE
 */
int setup_intr_system(XScuGic *int_ins_ptr,
                      XScuTimer *timer_ptr,
                      XTtcPs *debounce_timer_ptr,
                      XUartLite *gps_uart_ptr,
                      XTtcPs *gps_ttc_ptr,
                      XTtcPs *seq_ttc_ptr)
{
    int status;
    XScuGic_Config *gic_config;

    /* =================================================================
     * 步骤 1: 【最优先】设置CPU的顶层异常处理框架
     * 先搭建好中断处理的“骨架”，确保任何意外中断都能被引导到GIC处理函数。
     * ================================================================= */
    gic_config = XScuGic_LookupConfig(XPAR_SCUGIC_0_DEVICE_ID);
    if (NULL == gic_config)
    {
        printf("ERROR: XScuGic_LookupConfig failed.\n");
        return XST_FAILURE;
    }

    status = XScuGic_CfgInitialize(int_ins_ptr, gic_config, gic_config->CpuBaseAddress);
    if (status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    Xil_ExceptionInit();
    // 将GIC的驱动处理函数注册为CPU的官方中断处理入口
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 int_ins_ptr);

    /* =================================================================
     * 步骤 2: 初始化并连接所有中断源
     * 在这一步，我们只做软件配置（连接处理函数），不使能任何中断。
     * ================================================================= */
    // 初始化裸机专用的AXI INTC
    status = XIntc_Initialize(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_DEVICE_ID);
    if (status != XST_SUCCESS)
    {
        printf("ERROR: Failed to initialize BareMetal AXI INTC\n");
        return XST_FAILURE;
    }

    // 连接PL中断源到AXI INTC
    // Pin 0: DMA ADC完成中断
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_ADDA_AXI_DMA_0_S2MM_INTROUT_INTR, (XInterruptHandler)rx_intr_handler, (void *)&axidma);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // Pin 1: DMA DAC完成中断
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_ADDA_AXI_DMA_0_MM2S_INTROUT_INTR, (XInterruptHandler)tx_intr_handler, (void *)&axidma);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // Pin 2: prog_empty (FIFO空) -> underflow_handler
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_ADDA_AXIS_DATA_FIFO_1_PROG_EMPTY_INTR, (XInterruptHandler)underflow_handler, (void *)0);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // Pin 3: onoff_done (开关量完成) -> onoff_handler
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_ONOFF_DONE_INTR, (XInterruptHandler)onoff_handler, (void *)0);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // Pin 4: power_pulse_P(电能脉冲)
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_POWER_PULSE_V1_AXI_0_INTRPT_P_INTR, (XInterruptHandler)PowerPulse_P_IntrHandler, (void *)0);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // Pin 5: power_pulse_Q
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_POWER_PULSE_V1_AXI_0_INTRPT_Q_INTR, (XInterruptHandler)PowerPulse_Q_IntrHandler, (void *)0);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // Pin 6: interrupt (GPS UART)
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AXI_UARTLITE_0_INTERRUPT_INTR, (XInterruptHandler)XUartLite_InterruptHandler, (void *)gps_uart_ptr);
    if (status != XST_SUCCESS)
        return XST_FAILURE;
    // Pin 7: bm_sync_end
    // Pin 8: date_update (日期更新)
    // Pin 9: PPS_IN

    // Pin 10: 软时钟闹钟中断
    status = XIntc_Connect(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_TIME_UP_INTR, (XInterruptHandler)SoftTimer_AlarmHandler, (void *)0);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    //  连接AXI INTC的输出到GIC
    XScuGic_SetPriorityTriggerType(int_ins_ptr, XPAR_FABRIC_AXI_INTC_BAREMETAL_IRQ_INTR, 0x40, 0x1);             // 高电平触发
    ScuGic_SetInterruptTarget(int_ins_ptr->Config->DistBaseAddress, XPAR_FABRIC_AXI_INTC_BAREMETAL_IRQ_INTR, 1); // 将中断映射到目标CPU1
    status = XScuGic_Connect(int_ins_ptr, XPAR_FABRIC_AXI_INTC_BAREMETAL_IRQ_INTR, (Xil_ExceptionHandler)BareMetal_Intc_Handler, (void *)&AxiIntc_BareMetal);

    // 主定时器中断 (PS私有定时器，ID=XPAR_SCUTIMER_INTR)
    XScuGic_SetPriorityTriggerType(int_ins_ptr, XPAR_SCUTIMER_INTR, 0xB0, 0x3); // 上升沿
    status = XScuGic_Connect(int_ins_ptr, XPAR_SCUTIMER_INTR, (Xil_ExceptionHandler)timer_intr_handler, (void *)timer_ptr);

    // GPS超时TTC定时器中断 (PS TTC，ID=XPAR_XTTCPS_1_INTR)
    XScuGic_SetPriorityTriggerType(int_ins_ptr, XPAR_XTTCPS_1_INTR, 0x40, 0x3);             // 上升沿
    ScuGic_SetInterruptTarget(int_ins_ptr->Config->DistBaseAddress, XPAR_XTTCPS_1_INTR, 1); // 将中断映射到目标CPU1
    status = XScuGic_Connect(int_ins_ptr, XPAR_XTTCPS_1_INTR, (Xil_InterruptHandler)GpsTimeoutHandler, (void *)gps_ttc_ptr);

    // 开关量防抖TTC定时器中断 (PS TTC, ID=XPAR_XTTCPS_0_INTR)
    XScuGic_SetPriorityTriggerType(int_ins_ptr, XPAR_XTTCPS_0_INTR, 0xA0, 0x3);             // 上升沿
    ScuGic_SetInterruptTarget(int_ins_ptr->Config->DistBaseAddress, XPAR_XTTCPS_0_INTR, 1); // 将中断映射到目标CPU1
    status = XScuGic_Connect(int_ins_ptr, XPAR_XTTCPS_0_INTR, (Xil_ExceptionHandler)debounce_timer_handler, (void *)debounce_timer_ptr);

    // 状态序列TTC定时器中断(PS TTC，ID = XPAR_XTTCPS_2_INTR);
    XScuGic_SetPriorityTriggerType(int_ins_ptr, SEQ_TTC_INTR_ID, 0xA0, 0x3);
    ScuGic_SetInterruptTarget(int_ins_ptr->Config->DistBaseAddress, SEQ_TTC_INTR_ID, 1);
    status = XScuGic_Connect(int_ins_ptr, SEQ_TTC_INTR_ID, (Xil_ExceptionHandler)StateSequence_TTC_Handler, (void *)seq_ttc_ptr);
    if (status != XST_SUCCESS)
    {
        printf("StateSequence_TTC_init_error");
        return XST_FAILURE;
    }

    /* =================================================================
     * 步骤 4: 【关键】手动初始化CPU1的GIC接口并使能所有中断
     * 此时异常处理已就绪，可以安全地打开硬件中断了。
     * ================================================================= */
    // 启动AXI INTC
    status = XIntc_Start(&AxiIntc_BareMetal, XIN_REAL_MODE);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    // 使能AXI INTC上的中断输入
    // XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_ADDA_AXIS_DATA_FIFO_1_PROG_EMPTY_INTR);
    // XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AXI_UARTLITE_0_INTERRUPT_INTR);                        // 初始化的时候不使能GPS中断，在启动GPS对时再使能
    // XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_RTC_EEPROM_AXI_IIC_0_IIC2INTC_IRPT_INTR);              // 裸机下不使用该中断
    XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_ADDA_AXI_DMA_0_S2MM_INTROUT_INTR);
    XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_ADDA_AXI_DMA_0_MM2S_INTROUT_INTR);
    // XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_ONOFF_DONE_INTR);//在onoff_start中使能中断
    XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_POWER_PULSE_V1_AXI_0_INTRPT_P_INTR);
    XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_POWER_PULSE_V1_AXI_0_INTRPT_Q_INTR);
    XIntc_Enable(&AxiIntc_BareMetal, XPAR_AXI_INTC_BAREMETAL_AC_8_CHANNEL_0_STIMER_ONOFF_PA_RW_A_0_TIME_UP_INTR);

    // 使能GIC上的中断
    XScuGic_Enable(int_ins_ptr, XPAR_FABRIC_AXI_INTC_BAREMETAL_IRQ_INTR);
    XScuGic_Enable(int_ins_ptr, XPAR_SCUTIMER_INTR);
    XScuGic_Enable(int_ins_ptr, XPAR_XTTCPS_0_INTR);
    XScuGic_Enable(int_ins_ptr, XPAR_XTTCPS_1_INTR);
    XScuGic_Enable(int_ins_ptr, XPAR_XTTCPS_2_INTR);

    // 使能外设自身的中断产生
    // 使能PS外设中断源
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA); // DMA
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE); // DMA
    XScuTimer_EnableInterrupt(timer_ptr);                                     // 定时器
    XTtcPs_EnableInterrupts(debounce_timer_ptr, XTTCPS_IXR_INTERVAL_MASK);    // 使能TTC定时

    // 【最后一步】打开CPU的总中断开关
    Xil_ExceptionEnable();

    printf("CPU1: BareMetal AMP-safe interrupt system initialized SUCCESSFULLY.\n");
    return XST_SUCCESS;
}

int code_to_real(u16 x)
{ // 将16位数据转换成实际电压值
    int x1 = 0;
    int x2 = 0;

    if ((x >> 15) == 0)
    { // 如果是正电压，最高位为0
        x2 = +((int)(x + 1));
        // x2 = +(((float)(x+1))*(20.0f))/(65536.0f);
    }
    else
    {                           // 如果是负电压
        x1 = 0xFFFF - x + 0x01; // 将16位二进制数据全部取反，再+1
        x2 = -((int)(x1));
        // x2 = -(((float)(x1))*(20.0f))/(65536.0f);
    }
    return x2;
}

bool AdcFinish_Flag; // ADc完成标志，在中断处理函数中写1，主循环中读取
/**
 * @brief 处理DMA传输完成后的ADC数据 (修改后，采用指针访问以提高效率和清晰度)
 *
 * 此函数在rx_intr_handler中被调用。
 * 它负责将DMA缓冲区rx_buffer_ptr中的交错数据，正确地“解交错”并写入到
 * 共享DDR内存(Share_addr)中，确保每个通道的数据在内存中是连续存放的。
 */
void Adc_Data_processing()
{
    // 定义指向共享内存区域的指针数组，每个指针对应一个逻辑通道的基地址
    volatile u32 *channel_dest_base_ptrs[CHANNL_MAX];

    // 外层循环：遍历所有 AD_SAMP_CYCLE_NUMBER 个“原始周期”的数据块
    for (int cycle_idx = 0; cycle_idx < AD_SAMP_CYCLE_NUMBER; cycle_idx++)
    {
        // 1. 计算当前处理周期的共享内存基地址
        u32 *current_cycle_base_addr = (u32 *)(Share_addr + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32));

        // 2. 初始化8个通道在本周期的目标写入地址指针
        for (int ch = 0; ch < CHANNL_MAX; ch++)
        {
            channel_dest_base_ptrs[ch] = current_cycle_base_addr + ch * sample_points;
        }

        // 3. 计算DMA缓冲区中当前周期的起始地址
        u16 *current_cycle_dma_src_ptr = rx_buffer_ptr + cycle_idx * CHANNL_MAX * sample_points;

        // 4. 内层循环：遍历当前周期的所有采样点，进行解交错和写入
        for (int i = 0; i < sample_points; i++)
        {
            // 按照硬件实际的交错顺序读取DMA源数据
            // 物理顺序: JIA(0), JUA(1), JIB(2), JUB(3), JIC(4), UC(5), JIX(6), UX(7)
            u16 raw_ia = current_cycle_dma_src_ptr[i * CHANNL_MAX + 0];
            u16 raw_ua = current_cycle_dma_src_ptr[i * CHANNL_MAX + 1];
            u16 raw_ib = current_cycle_dma_src_ptr[i * CHANNL_MAX + 2];
            u16 raw_ub = current_cycle_dma_src_ptr[i * CHANNL_MAX + 3];
            u16 raw_ic = current_cycle_dma_src_ptr[i * CHANNL_MAX + 4];
            u16 raw_uc = current_cycle_dma_src_ptr[i * CHANNL_MAX + 5];
            u16 raw_ix = current_cycle_dma_src_ptr[i * CHANNL_MAX + 6];
            u16 raw_ux = current_cycle_dma_src_ptr[i * CHANNL_MAX + 7];

            // 将转换后的值通过指针写入对应的连续内存区域
            // 逻辑顺序: UA, UB, UC, UX, IA, IB, IC, IX
            *(channel_dest_base_ptrs[0] + i) = (u32)code_to_real(raw_ua);
            *(channel_dest_base_ptrs[1] + i) = (u32)code_to_real(raw_ub);
            *(channel_dest_base_ptrs[2] + i) = (u32)code_to_real(raw_uc);
            *(channel_dest_base_ptrs[3] + i) = (u32)code_to_real(raw_ux);
            *(channel_dest_base_ptrs[4] + i) = (u32)code_to_real(raw_ia);
            *(channel_dest_base_ptrs[5] + i) = (u32)code_to_real(raw_ib);
            *(channel_dest_base_ptrs[6] + i) = (u32)code_to_real(raw_ic);
            *(channel_dest_base_ptrs[7] + i) = (u32)code_to_real(raw_ix);
        }
    }
}

void Write_Wave_to_Wave_NewData()
{
    // 获取独立的运行状态 (0=停止, 1=运行, 2=暂停)
    // 只有状态为 1 时，才在波形中生成对应的分量
    bool run_fund = (devState.nStatusFund == 1);
    bool run_harm = (devState.nStatusHarm == 1);

    // 二维数组8*DATA_LEN     Wave_NewData中存储的是8个通道，每个通道DATA_LEN个点，正弦波
    for (int i = 0; i < CHANNL_MAX; i++)
    {
        // 【修正】传入 run_fund 和 run_harm
        addHarmonics(Wave_NewData[i], DATA_LEN,
                     Phase_shift[i],
                     numHarmonics[i], harmonics[i], harmonics_phases[i],
                     run_fund, run_harm);
    }

    // 处理数据长度扩展 (保持原有逻辑不变)
    if (DATA_LEN == 2048)
    {
        // 8*1024改成8*2048
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 1024; j++)
            {
                Wave_NewData[i][j + 1024] = Wave_NewData[i][j];
            }
        }
    }

    if (DATA_LEN == 4096)
    {
        // 8*1024改成8*2048
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 1024; j++)
            {
                Wave_NewData[i][j + 1024] = Wave_NewData[i][j];
            }
        }

        // 8*2048改成8*4096
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 2048; j++)
            {
                Wave_NewData[i][j + 2048] = Wave_NewData[i][j];
            }
        }
    }
}

void Copy_Wave_to_tx_buffer_ptr()
{
    // 要给一维数组tx_buffer_ptr赋值，tx_buffer_ptr中存的是8个通道的所有数据，正弦波。前8位分别对应8个通道的第一个点，依次类推，每个通道存DATA_LEN个点
    int k = 0;
    for (int j = 0; j < DATA_LEN; j++)
    {
        for (int i = 0; i < 8; i++)
        {
            tx_buffer_ptr[k + i] = Wave_NewData[i][j];
        }
        k += 8;
    }
}

void start_dma_dac()
{
    u16 frequency_divisor; // 分频系数 默认为1953
    // 修改通道使能和分频系数
    frequency_divisor = 100000000 / Wave_Frequency / DATA_LEN; // 分频系数

    /*
     * dma_enable     (slv_reg0[16])
     * dma_freq_div   (slv_reg1[31:16])
     * dma_channels   (slv_reg2[23:16])
     */
    Xil_Out32(dac_whole_base_addr + 0, (u32)0x10000);
    Xil_Out32(dac_whole_base_addr + 4, (u32)(frequency_divisor << 16));
    Xil_Out32(dac_whole_base_addr + 8, (u32)(enable) << 16);

    Write_Wave_to_Wave_NewData();
    Copy_Wave_to_tx_buffer_ptr();
    Xil_DCacheFlushRange((UINTPTR)tx_buffer_ptr, DATA_LEN * 16); // 刷新Data Cache
    XAxiDma_SimpleTransfer(&axidma, (UINTPTR)tx_buffer_ptr, DATA_LEN * 16, XAXIDMA_DMA_TO_DEVICE);
}

// 将波形写入BRAM(CDMA版)
void str_wr_bram(PID_STATE pid_state)
{
    // 尝试获取DAC/功放操作的锁
    if (!acquire_resource_lock(LOCK_OWNER_DAC, MUTEX_DAC_ACQUIRE_TIMEOUT_US))
    {
        printf("CPU1: str_wr_bram could not acquire lock.\n");
        return; // 获取锁失败，不执行后续操作
    }

    u16 frequency_divisor; // 分频系数 默认为1953

    // 1. 获取当前的运行状态
    // 只有状态为 1 (Run) 时，才在波形中生成对应的分量
    // 状态 0 (Stop) 或 2 (Pause) 时，对应的分量不生成 (视为0)
    bool run_fund = (devState.nStatusFund == 1);
    bool run_harm = (devState.nStatusHarm == 1);

    // PID调整相位
    double Phase_PID_Increment[CHANNL_MAX] = {0};
    if (pid_state == PID_ON)
    {
        for (int i = 0; i < CHANNL_MAX; i++)
        {
            // 假设lineAC.phu和lineAC.phi存储的是对应通道的实际相位
            double actual_value = (i < 4) ? lineAC.phu[i] : lineAC.phi[i - 4]; // 根据通道选择实际值

            Phase_PID_Increment[i] = PID_adjust_phase(Phase_shift[i], actual_value, &phase_pid[i]);
        }
    }
    else
    {
        // 当PID调节关闭时，清空phase_pid的累计值
        for (int i = 0; i < CHANNL_MAX; i++)
        {
            Phase_PID_Increment[i] = 0; // 清空PID累计值
            phase_pid[i].integral = 0;
            phase_pid[i].prev_error = 0;
        }
    }

    // 2. 计算波形数据到 Wave_NewData (CPU计算部分)
    for (int i = 0; i < CHANNL_MAX; i++)
    {
        // 获取当前通道的量程索引
        int range_idx;
        if (i < 4)
        { // 电压通道
            range_idx = get_voltage_index_by_value(setACS.Vals[i].UR);
        }
        else
        { // 电流通道
            range_idx = get_current_index_by_value(setACS.Vals[i - 4].IR);
        }

        // 应用PID调节值和相位校准参数
        addHarmonics(Wave_NewData[i], DATA_LEN,
                     Phase_shift[i] + Phase_PID_Increment[i] + DA_CorrectPhase_100[i][range_idx],
                     numHarmonics[i], harmonics[i], harmonics_phases[i],
                     run_fund, run_harm);
    }

    // 3. [核心修改] 打包数据到 DDR 并使用 CDMA 搬运
    // 使用与 SetTaskStateSequence 相同的 DDR 缓冲区首地址 (稳态输出时借用此空间)
    u32 *pDdrBuf = (u32 *)(UINTPTR)STATE_SEQ_DDR_BUFFER_BASE;

    // 数据打包：将8通道16位数据拼装成128位宽格式 (4个32位字)
    for (int j = 0; j < DATA_LEN; j++)
    {
        // Word 0: Ch1(High) | Ch0(Low) -> UB | UA
        pDdrBuf[j * 4 + 0] = (Wave_NewData[1][j] << 16) | Wave_NewData[0][j];
        // Word 1: Ch3 | Ch2 -> UX | UC
        pDdrBuf[j * 4 + 1] = (Wave_NewData[3][j] << 16) | Wave_NewData[2][j];
        // Word 2: Ch5 | Ch4 -> IB | IA
        pDdrBuf[j * 4 + 2] = (Wave_NewData[5][j] << 16) | Wave_NewData[4][j];
        // Word 3: Ch7 | Ch6 -> IX | IC
        pDdrBuf[j * 4 + 3] = (Wave_NewData[7][j] << 16) | Wave_NewData[6][j];
    }

    // 刷新 Data Cache，确保数据写入 DDR
    Xil_DCacheFlushRange((UINTPTR)pDdrBuf, WAVE_STEP_SIZE_BYTES);

    // 确保 CDMA 空闲
    int timeout = 10000;
    while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
        timeout--;
    if (timeout == 0)
    {
        XAxiCdma_Reset(&CdmaInstance); // 尝试复位
        while (!XAxiCdma_ResetIsDone(&CdmaInstance))
            ;
    }

    // 启动 CDMA 传输: DDR -> BRAM
    int Status = XAxiCdma_SimpleTransfer(&CdmaInstance, (UINTPTR)pDdrBuf,
                                         (UINTPTR)STATE_SEQ_BRAM_BASEADDR,
                                         WAVE_STEP_SIZE_BYTES, NULL, NULL);

    if (Status != XST_SUCCESS)
    {
        printf("CPU1: Error - Standard Waveform CDMA Submit Failed\r\n");
    }
    else
    {
        // 等待传输完成
        timeout = 1000000;
        while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
            timeout--;
        if (timeout == 0)
        {
            printf("CPU1: Error - Standard Waveform CDMA Timeout\r\n");
            XAxiCdma_Reset(&CdmaInstance);
        }
    }

    // 4. 修改通道使能和分频系数 (硬件寄存器配置)
    frequency_divisor = 100000000 / Wave_Frequency / DATA_LEN;

    Xil_Out32(dac_whole_base_addr + 0, 1); // start_dds
    Xil_Out32(dac_whole_base_addr + 4, (u32)frequency_divisor);
    Xil_Out32(dac_whole_base_addr + 8, (u32)enable);

    // 释放锁
    release_resource_lock(LOCK_OWNER_DAC);
}

/*
 *    添加谐波
 */
/**
 * @brief 向数组中添加谐波分量
 *
 * 该函数将一个基本波形与多个谐波分量相加，并将结果存储到指定的数组中。
 *
 * @param NewData 存储结果的数组
 * @param Array_length NewData数组的长度
 * @param Base_Phase_Degrees 基本波形的相位偏移（以度为单位）
 * @param numHarmonics 要添加的谐波数量
 * @param harmonics 谐波幅值的数组，harmonics[0]为2次谐波
 * @param harmonics_phases 谐波相位偏移的数组（以度为单位）
 */
void addHarmonics(uint16_t NewData[], int Array_length, float Base_Phase_Degrees, int numHarmonics, float harmonics[], float harmonics_phases[], bool en_fund, bool en_harm)
{
    // harmonics[0]为2次谐波
    for (int i = 0; i < Array_length; i++)
    {
        double phase = 2 * M_PI * i / Array_length;
        double sum = 0.0;

        // 1. 如果基波使能 (nStatusFund == 1)，计算基波分量
        if (en_fund)
        {
            double shifted_phase = phase + (Base_Phase_Degrees * M_PI / 180.0);
            sum += sin(shifted_phase);
        }

        // 2. 如果谐波使能 (nStatusHarm == 1)，计算谐波分量
        if (en_harm)
        {
            for (int j = 0; j < numHarmonics; j++)
            {
                double harmonic_phase = (j + 2) * phase;
                double shifted_harmonic_phase = harmonic_phase + harmonics_phases[j] * M_PI / 180.0;
                double harmonic_value = sin(shifted_harmonic_phase);
                sum += harmonic_value * harmonics[j];
            }
        }

        // 3. 归一化处理
        // 注意：分母计算也需要根据开关状态调整，否则幅值比例会错
        // 如果基波关了，分母中不应该包含1.0？这取决于Wave_Amplitude的定义。
        // 通常Wave_Amplitude定义了满量程DAC数值。
        // 此处保持原归一化逻辑，确保波形不削顶即可。
        // 如果只发谐波，谐波幅值是相对于基波满幅值的百分比，直接叠加即可。

        double total_amp = 0.0;
        if (en_fund)
            total_amp += 1.0;
        if (en_harm)
            total_amp += sumHarmonics(harmonics, numHarmonics);

        // 防止除0
        if (total_amp < 0.0001)
            total_amp = 1.0;

        // 映射到 uint16
        NewData[i] = (uint16_t)((sum / total_amp) * 32768 + 32767);
    }
}
// 辅助函数，计算谐波幅值总和
double sumHarmonics(float harmonics[], int numHarmonics)
{
    double sum = 0.0;
    for (int i = 0; i < numHarmonics; i++)
    {
        sum += harmonics[i];
    }
    return sum;
}

/**
 * @brief 根据电压值获取电压等级索引
 *
 * 根据给定的电压值，返回对应的电压等级索引。
 *
 * @param voltage 电压值，单位为伏特（V）
 * @return 返回电压等级索引：
 *         - 0：表示电压值大于等于6.0V（对应6.5V）
 *         - 1：表示电压值大于等于3.0V且小于6.0V（对应3.25V）
 *         - 2：表示电压值小于3.0V（对应1.876V）
 */
int get_voltage_index_by_value(float voltage)
{
    if (voltage >= 6.0)
        return 0; // 6.5V
    else if (voltage >= 3.0)
        return 1; // 3.25V
    else
        return 2; // 1.876V
}

/**
 * @brief 根据电流值获取当前索引
 *
 * 根据给定的电流值（current），返回对应的索引值。
 *
 * @param current 电流值，类型为float
 * @return 返回对应的索引值，类型为int。如果电流值大于等于3.0，则返回0（表示5A）；
 *         如果电流值大于等于0.5但小于3.0，则返回1（表示1A）；
 *         如果电流值小于0.5，则返回2（表示0.2A）。
 */
int get_current_index_by_value(float current)
{
    if (current >= 3.0)
        return 0; // 5A
    else if (current >= 0.5)
        return 1; // 1A
    else
        return 2; // 0.2A
}

/**
 * @brief axi_intc_BareMetal 的主中断服务程序
 * @details 当GIC的28号中断触发时，此函数被调用。
 * 它的职责是调用AXI INTC驱动提供的处理函数，
 * 该函数会自动查询INTC的状态并调用已注册的具体中断源的服务程序。
 * @param CallBackRef 回调引用，这里是AXI INTC实例的指针
 */
void BareMetal_Intc_Handler(void *CallbackRef)
{
    // printf("CPU1: BareMetal_Intc_Handler\n");
    XIntc_InterruptHandler((XIntc *)CallbackRef);
}
