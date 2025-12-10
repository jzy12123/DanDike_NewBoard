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
static void Record_Step_Result(int stepIndex, bool triged, double duration)
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
    usleep(10);
    // 2. 写入幅值系数数据
    Xil_Out32(BaseAddr + Amplifier_Din0_ADDR, v0);
    Xil_Out32(BaseAddr + Amplifier_Din1_ADDR, v1);
    Xil_Out32(BaseAddr + Amplifier_Din2_ADDR, v2);
    Xil_Out32(BaseAddr + Amplifier_Din3_ADDR, v3);

    // 3. 触发更新 (Start=1)
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000101); // Bit 8=1, Bit 0=1
    usleep(10);
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
/**
 * @brief 准备并启动状态序列 (修复：全局扫描最大幅值以确定量程)
 */
void StateSequence_PrepareAndStart(void)
{
    g_StateSeqRuntime.CurrentStepIndex = 0;
    g_StateSeqRuntime.TotalSteps = g_StateSequenceTask.StepCount;
    g_StateSeqRuntime.RepeatCountRemaining = g_StateSequenceTask.RepeatCount;
    g_StateSeqRuntime.IsRunning = true;
    g_StateSeqRuntime.IsHolding = false;
    g_StateSeqRuntime.IsFinished = false;

    g_StateSeqRuntime.ExecutedCount = 0;
    g_StateSeqRuntime.ReportedCount = -1;
    memset(g_StateSeqRuntime.ExecResults, 0, sizeof(g_StateSeqRuntime.ExecResults));

    In_CurrTime curr;
    read_current_time(&curr);
    sprintf(g_StateSeqRuntime.StartTimeStr, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            curr.curr_year, curr.curr_month, curr.curr_day,
            curr.curr_hour, curr.curr_minute, curr.curr_second,
            (unsigned int)(curr.curr_subsec / 100000));

    // =================================================================
    // 1. [核心修复] 全局扫描：遍历所有步骤，找出每个通道的最大幅值
    // =================================================================
    float max_u_vals[4] = {0}; // Chn 0~3
    float max_i_vals[4] = {0}; // Chn 4~7 (Step配置里的Chn是1~4, I对应4~7)

    for (int i = 0; i < g_StateSequenceTask.StepCount; i++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[i];
        for (int j = 0; j < pStep->ACCount; j++)
        {
            Struct_Seq_AC *pAC = &pStep->ACs[j];
            if (pAC->Line != 1)
                continue;

            int chn_idx = pAC->Chn - 1; // 0~3
            if (chn_idx >= 0 && chn_idx < 4)
            {
                if (pAC->U > max_u_vals[chn_idx])
                    max_u_vals[chn_idx] = pAC->U;
                if (pAC->I_ > max_i_vals[chn_idx])
                    max_i_vals[chn_idx] = pAC->I_;
            }
        }
    }

    // =================================================================
    // 2. 确定全局最佳量程，并回写到所有步骤
    //    确保 Precalculate_Waveforms 和 硬件配置 使用完全一致的量程
    // =================================================================
    float final_ur[4], final_ir[4];
    u32 range_codes[8]; // 0~3: U, 4~7: I

    // 初始化默认值
    for (int k = 0; k < 8; k++)
        range_codes[k] = voltage_to_output(6); // 获取默认(最小或最大)档位码

    for (int chn = 0; chn < 4; chn++)
    {
        // --- 确定电压量程 ---
        // 逻辑需与 Communications_Protocol.c 保持一致
        if (max_u_vals[chn] > 3.25f)
            final_ur[chn] = 6.5f;
        else if (max_u_vals[chn] > 1.876f)
            final_ur[chn] = 3.25f;
        else
            final_ur[chn] = 1.876f;

        range_codes[chn] = voltage_to_output(final_ur[chn]);

        // --- 确定电流量程 ---
        if (max_i_vals[chn] > 1.0f)
            final_ir[chn] = 5.0f;
        else if (max_i_vals[chn] > 0.2f)
            final_ir[chn] = 1.0f;
        else
            final_ir[chn] = 0.2f;

        range_codes[chn + 4] = current_to_output(final_ir[chn]);
    }

    // 回写量程到所有步骤 (关键！供 Precalculate_Waveforms 使用)
    for (int i = 0; i < g_StateSequenceTask.StepCount; i++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[i];
        for (int j = 0; j < pStep->ACCount; j++)
        {
            Struct_Seq_AC *pAC = &pStep->ACs[j];
            if (pAC->Line == 1)
            {
                int chn_idx = pAC->Chn - 1;
                if (chn_idx >= 0 && chn_idx < 4)
                {
                    pAC->UR = final_ur[chn_idx];
                    pAC->IR = final_ir[chn_idx];
                }
            }
        }
    }

    // 3. 预计算 (现在 UR/IR 已经是全局统一的了)
    Precalculate_Waveforms();
    Precalculate_HwParams();

    // 4. 配置硬件档位 (使用计算好的 range_codes)
    // r_din0: UB(Ch1) | UA(Ch0)
    u32 r_din0 = (range_codes[1] << 24) | (range_codes[0] << 8);
    u32 r_din1 = (range_codes[3] << 24) | (range_codes[2] << 8);
    u32 r_din2 = (range_codes[5] << 24) | (range_codes[4] << 8);
    u32 r_din3 = (range_codes[7] << 24) | (range_codes[6] << 8);

    Seq_Hw_SetRange(r_din0, r_din1, r_din2, r_din3);

    // 5. 配置满量程幅值 (Fixed 100% Gain)
    // 基于 final_ur / final_ir 计算 100% 系数
    u32 din_vals[8] = {0};

    for (int chn = 0; chn < 4; chn++)
    {
        // 电压
        int idx_u = get_voltage_index_by_value(final_ur[chn]);
        double corr_u = calculate_correction(chn, idx_u, 100.0f);
        din_vals[chn] = (u32)(1.0 * corr_u);

        // 电流
        int idx_i = get_current_index_by_value(final_ir[chn]);
        double corr_i = calculate_correction(chn + 4, idx_i, 100.0f);
        din_vals[chn + 4] = (u32)(1.0 * corr_i);
    }

    u32 v_din0 = (din_vals[1] << 16) | (din_vals[0] & 0xFFFF);
    u32 v_din1 = (din_vals[3] << 16) | (din_vals[2] & 0xFFFF);
    u32 v_din2 = (din_vals[5] << 16) | (din_vals[4] & 0xFFFF);
    u32 v_din3 = (din_vals[7] << 16) | (din_vals[6] & 0xFFFF);

    Seq_Hw_SetValue(v_din0, v_din1, v_din2, v_din3);

    // 6. 开启 DA IP
    Xil_Out32(dac_whole_base_addr + 0, 1);
    Xil_Out32(dac_whole_base_addr + 4, g_StateSeqRuntime.StepParams[0].Freq_Divisor);
    Xil_Out32(dac_whole_base_addr + 8, 0xFF);

    Execute_Step(0);
}

void StateSequence_Stop(void)
{
    // 如果不在运行，直接返回 (防止重复调用)
    if (!g_StateSeqRuntime.IsRunning)
        return;

    // 1. 停止定时器
    XTtcPs_Stop(&SeqTtcInstance);

    // 2. 切换状态标志
    g_StateSeqRuntime.IsRunning = false;
    g_StateSeqRuntime.IsHolding = true; // [新增] 进入保持模式，阻断主循环刷新
    g_StateSeqRuntime.IsFinished = true;

    // 标记系统状态为运行 (让上位机知道还在输出)
    devState.bACMeterMode = 0;
    devState.nStatusFund = 1;

    xil_printf("CPU1: StateSeq - Finished. Output HELD (No Sync).\r\n");
}

// [新增] 强制退出状态序列模式 (供外部指令抢占使用)
void StateSequence_QuitMode(void)
{
    // 如果正在运行，先停止定时器
    if (g_StateSeqRuntime.IsRunning)
    {
        XTtcPs_Stop(&SeqTtcInstance);
    }

    // 彻底清除所有标志，释放控制权给主循环
    g_StateSeqRuntime.IsRunning = false;
    g_StateSeqRuntime.IsHolding = false;

    // 注意：此处不操作硬件，硬件状态将在接下来的 SetACS->str_wr_bram 流程中被刷新
    // xil_printf("CPU1: StateSeq - Quit Mode. Control returned to Main Loop.\r\n");
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
    Record_Step_Result(currentIdx, false, (double)pStep->MaxDuration);
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
        double actual_duration = 0;
        if (interval > 0)
        {
            actual_duration = (double)counter_val * pStep->MaxDuration / (double)interval;
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
    // 1. StartTime
    cJSON_AddStringToObject(data, "StartTime", g_StateSeqRuntime.StartTimeStr);

    // 2. ExecutedStates
    cJSON_AddNumberToObject(data, "ExecutedStates", g_StateSeqRuntime.ExecutedCount);

    // 3. [新增] CurStateID (当前正在执行的步号，从1开始)
    cJSON_AddNumberToObject(data, "CurStateID", g_StateSeqRuntime.CurrentStepIndex + 1);

    // 4. RepeatID
    int currentRepeatID = 0;
    if (g_StateSequenceTask.RepeatCount >= (int)g_StateSeqRuntime.RepeatCountRemaining)
    {
        currentRepeatID = g_StateSequenceTask.RepeatCount - (int)g_StateSeqRuntime.RepeatCountRemaining;
    }
    cJSON_AddNumberToObject(data, "RepeatID", currentRepeatID);

    // 5. States 数组
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