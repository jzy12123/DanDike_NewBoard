#include "StateSequence.h"
#include "ADDA.h"
#include "Amplifier_Switch.h" // 需要用到 voltage_to_output, calculate_correction
#include "xil_cache.h"
#include "xil_printf.h"
#include "sleep.h"

// 全局变量
StateSeq_Runtime_t g_StateSeqRuntime;
XAxiCdma CdmaInstance;

// 全局定义 TTC 实例
XTtcPs SeqTtcInstance;

// 临时波形缓冲区 (8通道)
static uint16_t TempWaveData[8][DATA_LEN];

// ================= 内部辅助函数：记录执行结果 =================
static void Record_Step_Result(int stepIndex, bool triged, u32 duration)
{
    if (g_StateSeqRuntime.ExecutedCount >= MAX_SEQ_RESULTS)
    {
        return; // 缓冲区满
    }

    int idx = g_StateSeqRuntime.ExecutedCount;
    Seq_Step_Result_t *res = &g_StateSeqRuntime.ExecResults[idx];

    res->StateID = stepIndex + 1; // [新增] 步号从1开始，所以是 index + 1
    res->Triged = triged;
    res->Duration = duration;
    // 读取当前DI状态
    res->DI_State = OnOff_Read_Current_Input(g_onoff_bit_width);

    g_StateSeqRuntime.ExecutedCount++;
}

/**
 * @brief 写入量程 (模仿 power_amplifier_control 的 Range 配置部分)
 * @details 对应逻辑: 595置1 1595置0; 功放start清0 -> 写数据 -> 功放start置1
 */
static void Seq_Hw_SetRange(u32 r0, u32 r1, u32 r2, u32 r3)
{
    u32 BaseAddr = Amplifier_OnOff_BASEADDR;

    // 1. 复位/配置 595 (Range Enable?)
    // 595置1 1595置0
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000000);
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000002); // Bit 1 = 1
    usleep(10);                                              // 必须有！要不档位设置会失败
    // 2. 写入量程数据
    Xil_Out32(BaseAddr + Amplifier_Din0_ADDR, r0);
    Xil_Out32(BaseAddr + Amplifier_Din1_ADDR, r1);
    Xil_Out32(BaseAddr + Amplifier_Din2_ADDR, r2);
    Xil_Out32(BaseAddr + Amplifier_Din3_ADDR, r3);

    // 3. 触发更新 (Start=1)
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000102); // Bit 8=1, Bit 1=1
    usleep(10);
}

/**
 * @brief 写入幅值系数 (模仿 power_amplifier_control 的 Value 配置部分)
 * @details 对应逻辑: 595置0 1595置1; 功放start清0 -> 写数据 -> 功放start置1
 */
static void Seq_Hw_SetValue(u32 v0, u32 v1, u32 v2, u32 v3)
{
    u32 BaseAddr = Amplifier_OnOff_BASEADDR;

    // 1. 复位/配置 1595 (Value Enable?)
    // 595置0 1595置1
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000000);
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000001); // Bit 0 = 1

    // 2. 写入幅值系数数据
    Xil_Out32(BaseAddr + Amplifier_Din0_ADDR, v0);
    Xil_Out32(BaseAddr + Amplifier_Din1_ADDR, v1);
    Xil_Out32(BaseAddr + Amplifier_Din2_ADDR, v2);
    Xil_Out32(BaseAddr + Amplifier_Din3_ADDR, v3);

    // 3. 触发更新 (Start=1)
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000101); // Bit 8=1, Bit 0=1
}

// ================= 内部辅助函数 =================

/**
 * @brief 预计算波形数据 (存入DDR) 直接对波形数据进行数字缩放
 */
static void Precalculate_Waveforms()
{
    xil_printf("CPU1: StateSeq - Pre-calculating waveforms (Digital Scaling Mode)...\r\n");

    for (int step = 0; step < g_StateSequenceTask.StepCount; step++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[step];
        memset(TempWaveData, 0, sizeof(TempWaveData));

        for (int i = 0; i < pStep->ACCount; i++)
        {
            Struct_Seq_AC *pAC = &pStep->ACs[i];
            if (pAC->Line != 1)
                continue;

            int chn_logical = pAC->Chn - 1;
            if (chn_logical < 0 || chn_logical > 3)
                continue;

            // =================== 电压通道 (Ch 0~3) ===================
            int u_hw_idx = chn_logical;

            // 1. 生成归一化波形 (满幅值 +/- 32767)
            float u_harm_amps[MAX_HARMONICS] = {0};
            float u_harm_phases[MAX_HARMONICS] = {0};
            for (int h = 0; h < pAC->HarmCount; h++)
            {
                int order = pAC->Harms[h].HN;
                if (order >= 2 && order < (MAX_HARMONICS + 2))
                {
                    u_harm_amps[order - 2] = pAC->Harms[h].U / 100.0f;
                    u_harm_phases[order - 2] = pAC->Harms[h].PhU;
                }
            }
            addHarmonics(TempWaveData[u_hw_idx], DATA_LEN, pAC->PhU, MAX_HARMONICS, u_harm_amps, u_harm_phases, true, true);

            // 2. 计算数字缩放因子
            // 目标：Digital_Out * Max_Gain = Target_Output
            // 所以：Digital_Out = Full_Scale * (Target_Gain / Max_Gain)
            int idx_u = get_voltage_index_by_value(pAC->UR);

            // 计算 100% 幅值对应的校准系数 (Max_Gain)
            double corr_u_100 = calculate_correction(chn_logical, idx_u, 100.0f);
            // 100% 时的 Din 值 = 1.0 * corr
            double din_u_100 = 1.0 * corr_u_100;

            // 计算 目标 幅值对应的校准系数 (Target_Gain)
            float amp_pct_u = (pAC->UR > 0.001f) ? (pAC->U / pAC->UR * 100.0f) : 0.0f;
            double corr_u_tgt = calculate_correction(chn_logical, idx_u, amp_pct_u);
            // 目标 Din 值 = (pct/100) * corr
            double din_u_tgt = (amp_pct_u / 100.0f) * corr_u_tgt;

            // 缩放因子
            float scale_u = 0.0f;
            if (din_u_100 > 0.1)
            {
                scale_u = (float)(din_u_tgt / din_u_100);
            }

            // 3. 应用数字缩放 (针对 32768 中点)
            for (int k = 0; k < DATA_LEN; k++)
            {
                int32_t val = (int32_t)TempWaveData[u_hw_idx][k] - 32768; // 转为有符号
                val = (int32_t)(val * scale_u);                           // 缩放
                TempWaveData[u_hw_idx][k] = (uint16_t)(val + 32768);      // 转回无符号
            }

            // =================== 电流通道 (Ch 4~7) ===================
            int i_hw_idx = chn_logical + 4;

            // 1. 生成波形
            float i_harm_amps[MAX_HARMONICS] = {0};
            float i_harm_phases[MAX_HARMONICS] = {0};
            for (int h = 0; h < pAC->HarmCount; h++)
            {
                int order = pAC->Harms[h].HN;
                if (order >= 2 && order < (MAX_HARMONICS + 2))
                {
                    i_harm_amps[order - 2] = pAC->Harms[h].I_ / 100.0f;
                    i_harm_phases[order - 2] = pAC->Harms[h].PhI;
                }
            }
            addHarmonics(TempWaveData[i_hw_idx], DATA_LEN, pAC->PhI, MAX_HARMONICS, i_harm_amps, i_harm_phases, true, true);

            // 2. 计算缩放
            int idx_i = get_current_index_by_value(pAC->IR);

            double corr_i_100 = calculate_correction(chn_logical + 4, idx_i, 100.0f);
            double din_i_100 = 1.0 * corr_i_100;

            float amp_pct_i = (pAC->IR > 0.001f) ? (pAC->I_ / pAC->IR * 100.0f) : 0.0f;
            double corr_i_tgt = calculate_correction(chn_logical + 4, idx_i, amp_pct_i);
            double din_i_tgt = (amp_pct_i / 100.0f) * corr_i_tgt;

            float scale_i = 0.0f;
            if (din_i_100 > 0.1)
            {
                scale_i = (float)(din_i_tgt / din_i_100);
            }

            // 3. 应用缩放
            for (int k = 0; k < DATA_LEN; k++)
            {
                int32_t val = (int32_t)TempWaveData[i_hw_idx][k] - 32768;
                val = (int32_t)(val * scale_i);
                TempWaveData[i_hw_idx][k] = (uint16_t)(val + 32768);
            }
        }

        // 4. 打包写入 DDR
        u32 *pDdrStepBase = (u32 *)(UINTPTR)(STATE_SEQ_DDR_BUFFER_BASE + step * WAVE_STEP_SIZE_BYTES);
        for (int j = 0; j < DATA_LEN; j++)
        {
            pDdrStepBase[j * 4 + 0] = (TempWaveData[1][j] << 16) | TempWaveData[0][j];
            pDdrStepBase[j * 4 + 1] = (TempWaveData[3][j] << 16) | TempWaveData[2][j];
            pDdrStepBase[j * 4 + 2] = (TempWaveData[5][j] << 16) | TempWaveData[4][j];
            pDdrStepBase[j * 4 + 3] = (TempWaveData[7][j] << 16) | TempWaveData[6][j];
        }
        Xil_DCacheFlushRange((UINTPTR)pDdrStepBase, WAVE_STEP_SIZE_BYTES);
    }
}

/**
 * @brief 预计算硬件参数 (优化版)
 * @details
 * 1. 计算频率分频系数。
 * 2. 计算 DO 状态。
 * 3. 不再计算 Din (幅值系数)，因为在“数字缩放”模式下，
 * 硬件增益固定为满量程，波形幅度由 BRAM 数据直接决定。
 */
static void Precalculate_HwParams()
{
    xil_printf("CPU1: StateSeq - Pre-calculating HW Params (Freq, DO)...\r\n");

    for (int step = 0; step < g_StateSequenceTask.StepCount; step++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[step];
        Step_Hw_Params_t *pHw = &g_StateSeqRuntime.StepParams[step];

        // 1. 频率分频系数 (默认 50Hz: 100MHz / 50 / 1024 = 1953)
        pHw->Freq_Divisor = 1953;

        // 遍历寻找有效的频率设置
        for (int i = 0; i < pStep->ACCount; i++)
        {
            if (pStep->ACs[i].Line == 1 && pStep->ACs[i].F > 0.1f)
            {
                pHw->Freq_Divisor = (u32)(100000000.0f / pStep->ACs[i].F / (float)DATA_LEN);
                break; // 取第一个有效频率
            }
        }

        // 2. DO 状态预计算
        pHw->DO_State = 0;
        for (int k = 0; k < pStep->DOCount; k++)
        {
            if (pStep->DOs[k].Val)
                pHw->DO_State |= (1 << (pStep->DOs[k].Chn + 23));
        }
    }
}

/**
 * @brief 立即切换到指定步 (DMA 搬运)
 */
static void Load_Step_To_BRAM(int stepIndex)
{
    if (stepIndex >= g_StateSeqRuntime.TotalSteps)
        return;

    u32 SrcAddr = STATE_SEQ_DDR_BUFFER_BASE + stepIndex * WAVE_STEP_SIZE_BYTES;
    u32 DestAddr = STATE_SEQ_BRAM_BASEADDR;

    // 检查 CDMA 是否空闲，带超时防止死锁
    int timeout = 100000;
    while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
    {
        timeout--;
    }
    if (timeout == 0)
    {
        xil_printf("CPU1: CDMA Busy Stuck - Resetting...\r\n");
        XAxiCdma_Reset(&CdmaInstance);
        while (!XAxiCdma_ResetIsDone(&CdmaInstance))
            ; // 等待复位完成
    }

    // 启动传输
    int Status = XAxiCdma_SimpleTransfer(&CdmaInstance, (UINTPTR)SrcAddr, (UINTPTR)DestAddr, WAVE_STEP_SIZE_BYTES, NULL, NULL);
    if (Status != XST_SUCCESS)
    {
        xil_printf("CPU1: Error - CDMA Submit Failed (Status %d)\r\n", Status);
        return; // 发送失败直接返回
    }

    // 等待传输完成
    timeout = 1000000;
    while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
    {
        timeout--;
    }

    if (timeout == 0)
    {
        xil_printf("CPU1: Error - CDMA Timeout! (Check Address 0x%X)\r\n", DestAddr);
        XAxiCdma_Reset(&CdmaInstance);
        while (!XAxiCdma_ResetIsDone(&CdmaInstance))
            ;
    }
}

/**
 * @brief 设置并启动步进定时器 (TTC)
 */
static void Start_Step_Timer(int duration_ms)
{
    XTtcPs_Stop(&SeqTtcInstance);

    if (duration_ms <= 0)
        return;

    // TTC 输入时钟频率 (Hz)
    u32 PCLK_Freq = 111111115;

    // 计算需要的总 Ticks 数 = PCLK * (ms / 1000)
    // 使用 u64 避免计算溢出
    u64 total_ticks_needed = (u64)PCLK_Freq * duration_ms / 1000;

    u8 prescaler_exp = 0; // 对应寄存器值 N, 分频比 = 2^(N+1)
    u32 interval = 0;

    // 寻找合适的分频系数，使得 interval < 65536
    for (prescaler_exp = 0; prescaler_exp < 16; prescaler_exp++)
    {
        u32 divider = 1 << (prescaler_exp + 1);
        u64 ticks = total_ticks_needed / divider;

        if (ticks <= 65535)
        {
            interval = (u32)ticks;
            break;
        }
    }

    // 如果时间太长超出量程，设为最大值
    if (interval == 0 && total_ticks_needed > 0)
    {
        prescaler_exp = 15; // 最大分频 65536
        interval = 65535;
        xil_printf("CPU1: Warning - Step duration %d ms too long, capped.\r\n", duration_ms);
    }

    // 设置 TTC
    XTtcPs_SetPrescaler(&SeqTtcInstance, prescaler_exp);
    XTtcPs_SetInterval(&SeqTtcInstance, interval);

    // xil_printf("  [DEBUG] Timer: %d ms -> Pre=%d, Int=%d\r\n", duration_ms, prescaler_exp, interval);

    XTtcPs_ResetCounterValue(&SeqTtcInstance);
    XTtcPs_EnableInterrupts(&SeqTtcInstance, XTTCPS_IXR_INTERVAL_MASK);
    XTtcPs_Start(&SeqTtcInstance);
}

/**
 * @brief 执行单步切换
 * @details 1. CDMA搬运波形; 2. 写入DO; 3. 启动定时器
 */
static void Execute_Step(int stepIndex)
{
    // 检查是否结束和循环次数
    if (stepIndex >= g_StateSeqRuntime.TotalSteps)
    {
        if (g_StateSequenceTask.RepeatCount > 0 && g_StateSeqRuntime.RepeatCountRemaining > 0)
        {
            g_StateSeqRuntime.RepeatCountRemaining--;
            Execute_Step(0); // 循环
        }
        else
        {
            StateSequence_Stop(); // 结束
        }
        return;
    }

    g_StateSeqRuntime.CurrentStepIndex = stepIndex;
    Step_Hw_Params_t *pHw = &g_StateSeqRuntime.StepParams[stepIndex];
    Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[stepIndex];

    // 1. 启动 CDMA 搬运波形 (DDR -> BRAM)(更新波形，自带缩放)
    Load_Step_To_BRAM(stepIndex);

    // 2. 写入硬件参数
    // 写入频率分频系数 (Offset 0x04)
    Xil_Out32(dac_whole_base_addr + 4, pHw->Freq_Divisor);

    // 3. 更新 DO
    if (pStep->DOCount > 0)
    {
        // 这里需要更复杂的逻辑合并全局DO，暂时直接写
        OnOff_Write_Continuous(pHw->DO_State);
    }

    // 4. 启动定时器
    if (pStep->MaxDuration > 0)
    {
        Start_Step_Timer(pStep->MaxDuration);
    }

    xil_printf("CPU1: StateSeq - Step %d Executed. Time: %d ms\r\n", stepIndex, pStep->MaxDuration);
}

// ================= 全局接口实现 =================

int StateSequence_Init(void)
{
    int Status;

    // 1. CDMA 初始化
    XAxiCdma_Config *CdmaConfig = XAxiCdma_LookupConfig(CDMA_DEVICE_ID);
    if (!CdmaConfig)
        return XST_FAILURE;
    Status = XAxiCdma_CfgInitialize(&CdmaInstance, CdmaConfig, CdmaConfig->BaseAddress);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    // 禁用CDMA中断 (使用轮询)
    XAxiCdma_IntrDisable(&CdmaInstance, XAXICDMA_XR_IRQ_ALL_MASK);

    // 2. TTC 初始化 (TTC2)
    XTtcPs_Config *TtcConfig = XTtcPs_LookupConfig(SEQ_TTC_DEVICE_ID);
    if (!TtcConfig)
        return XST_FAILURE;
    // 使用全局变量 SeqTtcInstance
    Status = XTtcPs_CfgInitialize(&SeqTtcInstance, TtcConfig, TtcConfig->BaseAddress);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    XTtcPs_SetOptions(&SeqTtcInstance, XTTCPS_OPTION_INTERVAL_MODE);

    g_StateSeqRuntime.IsRunning = false;

    return XST_SUCCESS;
}

void StateSequence_PrepareAndStart(void)
{
    // 1. 初始化运行时状态
    g_StateSeqRuntime.CurrentStepIndex = 0;
    g_StateSeqRuntime.TotalSteps = g_StateSequenceTask.StepCount;
    g_StateSeqRuntime.RepeatCountRemaining = g_StateSequenceTask.RepeatCount;
    g_StateSeqRuntime.IsRunning = true;
    g_StateSeqRuntime.IsFinished = false;
    // 初始化上报计数器
    g_StateSeqRuntime.ExecutedCount = 0;
    // 将 ReportedCount 初始化为 -1
    // 目的：使得 check_and_report 函数在第一次运行时满足 (ExecutedCount(0) > ReportedCount(-1))
    // 从而立即发送一个 ExecutedStates=0, States=[] 的初始报告，告知上位机 StartTime。
    g_StateSeqRuntime.ReportedCount = -1;
    memset(g_StateSeqRuntime.ExecResults, 0, sizeof(g_StateSeqRuntime.ExecResults));
    //  记录启动时间
    In_CurrTime curr;
    read_current_time(&curr);
    sprintf(g_StateSeqRuntime.StartTimeStr, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            curr.curr_year, curr.curr_month, curr.curr_day,
            curr.curr_hour, curr.curr_minute, curr.curr_second,
            (unsigned int)(curr.curr_subsec / 100000)); // ms

    // 预计算波形（带幅值）
    Precalculate_Waveforms();
    // 预计算硬件参数(频率 DO)
    Precalculate_HwParams();

    // 2. 档位配置 (Phase 1: Range Config)
    Struct_Seq_AC *pAC0 = &g_StateSequenceTask.Steps[0].ACs[0];
    u32 range_u = voltage_to_output(pAC0->UR);
    u32 range_i = current_to_output(pAC0->IR);
    // 组合成4个32位字: High16=Ch_Odd, Low16=Ch_Even
    // Din0: UB|UA, Din1: UX|UC ...
    u32 r_din0 = (range_u << 24) | (range_u << 8); // U
    u32 r_din1 = (range_u << 24) | (range_u << 8); // U
    u32 r_din2 = (range_i << 24) | (range_i << 8); // I
    u32 r_din3 = (range_i << 24) | (range_i << 8); // I
    // [修正] 调用 Seq_Hw_SetRange 写入档位
    Seq_Hw_SetRange(r_din0, r_din1, r_din2, r_din3);

    // 3. [核心新增] 将功放幅值增益固定为满量程 (100%)
    // 计算 100% 对应的 Din 值
    int idx_u = get_voltage_index_by_value(pAC0->UR);
    int idx_i = get_current_index_by_value(pAC0->IR);
    // 获取通道 0(UA) 和 4(IA) 在 100% 时的校准系数
    u32 din_vals[8];
    for (int i = 0; i < 4; i++)
    {
        double corr = calculate_correction(i, idx_u, 100.0f); // U
        din_vals[i] = (u32)(1.0 * corr);
    }
    for (int i = 4; i < 8; i++)
    {
        double corr = calculate_correction(i, idx_i, 100.0f); // I
        din_vals[i] = (u32)(1.0 * corr);
    }
    u32 v_din0 = (din_vals[1] << 16) | (din_vals[0] & 0xFFFF);
    u32 v_din1 = (din_vals[3] << 16) | (din_vals[2] & 0xFFFF);
    u32 v_din2 = (din_vals[5] << 16) | (din_vals[4] & 0xFFFF);
    u32 v_din3 = (din_vals[7] << 16) | (din_vals[6] & 0xFFFF);
    // 写入并锁定硬件增益
    Seq_Hw_SetValue(v_din0, v_din1, v_din2, v_din3);

    // 4. 开启 DA IP
    Xil_Out32(dac_whole_base_addr + 0, 1);
    Xil_Out32(dac_whole_base_addr + 4, g_StateSeqRuntime.StepParams[0].Freq_Divisor);
    Xil_Out32(dac_whole_base_addr + 8, 0xFF);

    // 5. 执行第一步 (Values, Waveform, Timer)
    Execute_Step(0);
}

// [新增] 将指定Step的参数同步到全局变量 (setACS, Wave_Amplitude等)
// 确保序列结束后，主循环接管时能保持最后的输出状态
static void Sync_Step_To_Global(int stepIndex)
{
    if (stepIndex < 0 || stepIndex >= g_StateSequenceTask.StepCount)
        return;

    Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[stepIndex];

    // 1. 清空全局谐波数据
    memset(numHarmonics, 0, sizeof(numHarmonics));
    memset(harmonics, 0, sizeof(harmonics));
    memset(harmonics_phases, 0, sizeof(harmonics_phases));

    // 2. [关键修正] 先清空全局基波电气参数 (U, I, Phase)
    // 必须先清空，确保状态序列中未定义的通道或参数回归 0 状态，
    // 防止之前的稳态设置（如 5A）残留下来，干扰后续的单参数修改指令。
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        setACS.Vals[i].U = 0.0f;
        setACS.Vals[i].I_ = 0.0f;
        setACS.Vals[i].PhU = 0.0f;
        setACS.Vals[i].PhI = 0.0f;
        // 注意：不要清零 UR/IR/Line/Chn，否则会导致除零错误或通道映射丢失
        // F 也可以选择重置为 50，或者保持当前值
    }

    // 同时清空全局幅值数组，确保未覆盖的通道输出为0
    memset(Wave_Amplitude, 0, sizeof(Wave_Amplitude));

    // 3. 遍历步骤中的 AC 配置进行覆盖 (将最后一步的状态写入全局变量)
    for (int i = 0; i < pStep->ACCount; i++)
    {
        Struct_Seq_AC *pAC = &pStep->ACs[i];

        // 映射 Line/Chn 到全局索引 (0-7)
        int chn_log = pAC->Chn - 1;
        if (chn_log < 0 || chn_log > 3)
            continue;

        int idx_u = chn_log;     // 0,1,2,3
        int idx_i = chn_log + 4; // 4,5,6,7

        // --- 同步参数到 setACS ---
        setACS.Vals[idx_u].U = pAC->U;
        setACS.Vals[idx_u].PhU = pAC->PhU;
        setACS.Vals[idx_u].UR = pAC->UR;
        setACS.Vals[idx_u].F = pAC->F;

        setACS.Vals[idx_u].I_ = pAC->I_;
        setACS.Vals[idx_u].PhI = pAC->PhI;
        setACS.Vals[idx_u].IR = pAC->IR;

        // 更新全局控制变量
        Wave_Frequency = pAC->F;

        // 更新波形幅值 Wave_Amplitude (百分比)
        // 注意：此处必须重新计算，因为前面被 memset 清空了
        if (pAC->UR > 0.001f)
            Wave_Amplitude[idx_u] = (pAC->U / pAC->UR) * 100.0f;

        if (pAC->IR > 0.001f)
            Wave_Amplitude[idx_i] = (pAC->I_ / pAC->IR) * 100.0f;

        // 更新量程 Wave_Range
        Wave_Range[idx_u] = voltage_to_output(pAC->UR);
        Wave_Range[idx_i] = current_to_output(pAC->IR);

        // 更新相位 Phase_shift
        Phase_shift[idx_u] = pAC->PhU;
        Phase_shift[idx_i] = pAC->PhI;

        // --- 同步谐波参数 ---
        for (int h = 0; h < pAC->HarmCount; h++)
        {
            int hn = pAC->Harms[h].HN;
            int h_idx = hn - 2; // 2次谐波对应索引0
            if (h_idx >= 0 && h_idx < MAX_HARMONICS)
            {
                if (hn > numHarmonics[idx_u])
                    numHarmonics[idx_u] = hn;
                harmonics[idx_u][h_idx] = pAC->Harms[h].U / 100.0f;
                harmonics_phases[idx_u][h_idx] = pAC->Harms[h].PhU;

                if (hn > numHarmonics[idx_i])
                    numHarmonics[idx_i] = hn;
                harmonics[idx_i][h_idx] = pAC->Harms[h].I_ / 100.0f;
                harmonics_phases[idx_i][h_idx] = pAC->Harms[h].PhI;
            }
        }
    }

    // 更新 DO 状态 (保持不变)
    for (int k = 0; k < pStep->DOCount; k++)
    {
        int chn = pStep->DOs[k].Chn;
        int val = pStep->DOs[k].Val;
        if (chn >= 1 && chn <= ChnsDO)
        {
            lineDO.DO[chn - 1].v = val;
            if (val)
                g_do_output_state |= (1 << (chn + 23));
            else
                g_do_output_state &= ~(1 << (chn + 23));
        }
    }
}
void StateSequence_Stop(void)
{
    if (!g_StateSeqRuntime.IsRunning)
        return;

    // 1. 停止定时器 (不再产生新的步骤切换)
    XTtcPs_Stop(&SeqTtcInstance);

    // 2. [关键] 不要清空硬件输出！
    // 原先的 Seq_Hw_SetValue(0...) 和 Xil_Out32(...0x00) 被移除
    // 硬件将保持在最后一次 Execute_Step 设置的状态

    // 3. 将当前（最后执行的）Step 参数同步到全局变量
    // 这样当 IsRunning 变为 false 后，主循环恢复工作时，
    // str_wr_bram 会读取这些全局变量，生成与当前硬件输出一致的波形，实现无缝衔接。
    Sync_Step_To_Global(g_StateSeqRuntime.CurrentStepIndex);

    // 4. 标记系统状态为运行
    // 这告诉主循环：现在是“运行”状态，不要执行关断逻辑
    devState.bACMeterMode = 0; // 源模式
    devState.nStatusFund = 1;  // 运行状态

    // 5. 结束状态序列任务
    g_StateSeqRuntime.IsRunning = false;
    g_StateSeqRuntime.IsFinished = true; // 标记完成，通知主循环发送最后一次Success

    xil_printf("CPU1: StateSeq - Finished. \r\n");
}

// ================= 中断处理 =================

void StateSequence_TTC_Handler(void *CallBackRef)
{
    XTtcPs *Timer = (XTtcPs *)CallBackRef;
    XTtcPs_ClearInterruptStatus(Timer, XTtcPs_GetInterruptStatus(Timer));

    if (!g_StateSeqRuntime.IsRunning)
        return;

    int currentIdx = g_StateSeqRuntime.CurrentStepIndex;
    Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[currentIdx];
    // [新增] 记录本步结果 (超时)
    // 实际持续时间即为设定时间
    Record_Step_Result(currentIdx, false, pStep->MaxDuration);
    int nextStep = -1;

    // 解析跳转逻辑 (0: 下一步, -1: 下一步, -2: 结束)
    if (pStep->JumpTo == 0 || pStep->JumpTo == -1)
        nextStep = currentIdx + 1;
    else if (pStep->JumpTo == -2)
        nextStep = g_StateSeqRuntime.TotalSteps;
    else if (pStep->JumpTo > 0)
        nextStep = pStep->JumpTo - 1;

    Execute_Step(nextStep);
}

void StateSequence_DI_Check(uint32_t changed_bits, uint32_t current_val)
{
    if (!g_StateSeqRuntime.IsRunning)
        return;

    int currentIdx = g_StateSeqRuntime.CurrentStepIndex;
    Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[currentIdx];

    if (pStep->TrigLogic == -1)
        return;

    bool trig = false;
    int match_count = 0;

    for (int i = 0; i < pStep->TrigDICount; i++)
    {
        int chn = pStep->TrigDIs[i].Chn;
        int target_val = pStep->TrigDIs[i].Val; // 协议逻辑值: 1=闭合, 0=断开

        // 1. 获取硬件原始位 (假设 Chn 1 对应 Bit 0)
        int raw_bit = (current_val >> (chn - 1)) & 0x01;

        // 2. [关键修正] 逻辑反转：硬件0=逻辑1(闭合), 硬件1=逻辑0(断开)
        // 必须与 report_di_soe_event 中的逻辑保持一致
        int logic_val = 1 - raw_bit;

        if (logic_val == target_val)
        {
            match_count++;
        }
    }

    if (pStep->TrigLogic == 0)
    { // OR
        if (match_count > 0)
            trig = true;
    }
    else
    { // AND
        if (match_count == pStep->TrigDICount && pStep->TrigDICount > 0)
            trig = true;
    }

    if (trig)
    {
        // 获取 TTC 计数值用于计算实际时长
        u32 counter_val = XTtcPs_GetCounterValue(&SeqTtcInstance);
        // 获取 Interval 值 (目标计数值)
        u16 interval = XTtcPs_GetInterval(&SeqTtcInstance);
        // 计算当前已运行时间： (Count / Interval) * MaxDuration
        // 原理：运行进度的百分比 * 总时间
        u32 actual_duration = 0;
        if (interval > 0)
        {
            // 使用 u64 避免 (counter * duration) 乘法溢出
            actual_duration = (u32)((u64)counter_val * pStep->MaxDuration / interval);
        }
        // 记录本步结果 (Triged=true, 实际时长)
        Record_Step_Result(currentIdx, true, actual_duration);

        XTtcPs_Stop(&SeqTtcInstance);
        int nextStep = -1;
        // 解析 JumpTo (DI触发时的逻辑)
        // JumpTo = 0:  跳下一步
        // JumpTo = -2: 跳下一步
        // JumpTo = -1: 跳结束
        if (pStep->JumpTo == 0 || pStep->JumpTo == -2)
            nextStep = currentIdx + 1;
        else if (pStep->JumpTo == -1)
            nextStep = g_StateSeqRuntime.TotalSteps;
        else if (pStep->JumpTo > 0)
            nextStep = pStep->JumpTo - 1;

        Execute_Step(nextStep);
    }
}

// ================= [新增] 状态上报函数 (在 Timer_intr 中调用) =================

void check_and_report_state_sequence_status(void)
{
    // 判断是否有新数据或状态变化
    // 只要 ExecutedCount > ReportedCount，说明有新步骤完成，需要上报全量列表
    // 或者刚刚结束 (IsFinished=true)，需要发最后一次 Success
    bool has_new_data = (g_StateSeqRuntime.ExecutedCount > g_StateSeqRuntime.ReportedCount);
    bool just_finished = g_StateSeqRuntime.IsFinished;

    if (!has_new_data && !just_finished)
    {
        return; // 无需上报
    }

    // 构建 JSON
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "TaskEvent");
    cJSON_AddStringToObject(report, "FunCode", "StateSequence");

    if (g_StateSeqRuntime.IsRunning)
    {
        cJSON_AddStringToObject(report, "Result", "Doing");
    }
    else if (g_StateSeqRuntime.IsFinished)
    {
        cJSON_AddStringToObject(report, "Result", "Success");
    }
    else
    {
        cJSON_AddStringToObject(report, "Result", "Failure"); // 异常情况
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "ExecutedStates", g_StateSeqRuntime.ExecutedCount);
    cJSON_AddStringToObject(data, "StartTime", g_StateSeqRuntime.StartTimeStr);
    int currentRepeatID = 0;
    if (g_StateSequenceTask.RepeatCount >= (int)g_StateSeqRuntime.RepeatCountRemaining)
    {
        currentRepeatID = g_StateSequenceTask.RepeatCount - (int)g_StateSeqRuntime.RepeatCountRemaining;
    }
    cJSON_AddNumberToObject(data, "Repeated", currentRepeatID);//重复次数计数
    cJSON *statesArr = cJSON_CreateArray();

    // [全量上报逻辑] 遍历所有已执行的步骤 (0 到 ExecutedCount-1)
    for (int i = 0; i < g_StateSeqRuntime.ExecutedCount; i++)
    {
        Seq_Step_Result_t *res = &g_StateSeqRuntime.ExecResults[i];
        cJSON *stepObj = cJSON_CreateObject();

        cJSON_AddNumberToObject(stepObj, "StateID", res->StateID); // <--- [新增] 添加状态步序号
        cJSON_AddBoolToObject(stepObj, "Triged", res->Triged);
        cJSON_AddNumberToObject(stepObj, "Duration", res->Duration);

        // --- DI 数组转换  ---
        cJSON *diArr = cJSON_CreateArray();
        int num_bits = 8;
        switch (g_onoff_bit_width)
        {
        case bit_16:
            num_bits = 16;
            break;
        case bit_24:
            num_bits = 24;
            break;
        case bit_32:
            num_bits = 32;
            break;
        case bit_8:
        default:
            num_bits = 8;
            break;
        }

        for (int b = 0; b < num_bits; b++)
        {
            // 逻辑反转：硬件0=闭合(1)
            int val = 1 - ((res->DI_State >> b) & 1);
            cJSON_AddItemToArray(diArr, cJSON_CreateNumber(val));
        }
        cJSON_AddItemToObject(stepObj, "DI", diArr);
        // ---------------------------

        cJSON_AddItemToArray(statesArr, stepObj);
    }
    cJSON_AddItemToObject(data, "States", statesArr);
    cJSON_AddItemToObject(report, "Data", data);

    // 发送
    char *string = cJSON_PrintUnformatted(report);
    if (string)
    {
        // 打印测试
        // printf("CPU1: [DEBUG] Sending StateSequence Report: %s\r\n", string);
        size_t len = strlen(string);
        char *finalStr = malloc(len + 3);
        if (finalStr)
        {
            snprintf(finalStr, len + 3, "|%s|", string);
            MsgQue_write(finalStr, strlen(finalStr));
            free(finalStr);
        }
        free(string);
    }
    cJSON_Delete(report);

    // 更新状态
    g_StateSeqRuntime.ReportedCount = g_StateSeqRuntime.ExecutedCount;

    // 只有当发送完 Success 之后，才清除 IsFinished 标志，停止后续发送
    if (g_StateSeqRuntime.IsFinished)
    {
        g_StateSeqRuntime.IsFinished = false;
    }
}