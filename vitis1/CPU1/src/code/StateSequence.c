#include "StateSequence.h"
#include "ADDA.h"
#include "Amplifier_Switch.h" // 需要用到 voltage_to_output, calculate_correction
#include "xil_cache.h"
#include "xil_printf.h"
#include "sleep.h"
#include "soft_timer.h"
#include "WaveRecord.h"
#include <stdlib.h>

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

    // 尝试次数
    int retries = 2;
    while (retries > 0)
    {
        // 1. 等待空闲 (带超时)
        int timeout = 100000;
        while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
        {
            timeout--;
        }

        // 2. 如果超时，复位硬件
        if (timeout == 0)
        {
            xil_printf("CPU1: CDMA Busy Stuck - Resetting...\r\n");
            XAxiCdma_Reset(&CdmaInstance);
            while (!XAxiCdma_ResetIsDone(&CdmaInstance))
                ;
        }

        // 3. 提交传输
        int Status = XAxiCdma_SimpleTransfer(&CdmaInstance, (UINTPTR)SrcAddr, (UINTPTR)DestAddr, WAVE_STEP_SIZE_BYTES, NULL, NULL);

        if (Status == XST_SUCCESS)
        {
            // 提交成功，等待完成
            timeout = 1000000;
            while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
            {
                timeout--;
            }
            if (timeout == 0)
            {
                // 传输过程中卡死，需要复位并重试
                xil_printf("CPU1: CDMA Transfer Timeout!\r\n");
                XAxiCdma_Reset(&CdmaInstance);
                while (!XAxiCdma_ResetIsDone(&CdmaInstance))
                    ;
                retries--;
                continue; // 重试
            }
            // 传输彻底完成
            return;
        }
        else
        {
            // 提交失败 (可能是复位后状态不对)，复位并重试
            xil_printf("CPU1: CDMA Submit Failed (Status %d), Retrying...\r\n", Status);
            XAxiCdma_Reset(&CdmaInstance);
            while (!XAxiCdma_ResetIsDone(&CdmaInstance))
                ;
            retries--;
        }
    }

    xil_printf("CPU1: Error - Load Step %d Failed after retries!\r\n", stepIndex);
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

//  辅助函数：检查当前 DI 状态是否已经满足了步骤的触发条件
static bool Check_Step_Condition_Met(Struct_Seq_Step *pStep)
{
    if (pStep->TrigLogic == -1)
        return false; // 该步未启用触发，不满足

    u32 current_val = OnOff_Read_Current_Input(g_onoff_bit_width);
    int match_count = 0;

    for (int i = 0; i < pStep->TrigDICount; i++)
    {
        int chn = pStep->TrigDIs[i].Chn;
        int target_val = pStep->TrigDIs[i].Val;

        // 获取当前硬件状态 (假设 Chn 1 对应 Bit 0)
        int raw_bit = (current_val >> (chn - 1)) & 0x01;
        int logic_val = 1 - raw_bit; // 逻辑反转: 硬件0=闭合(1)

        if (logic_val == target_val)
        {
            match_count++;
        }
    }

    if (pStep->TrigLogic == 0)
    { // OR 逻辑
        if (match_count > 0)
            return true;
    }
    else
    { // AND 逻辑
        if (match_count == pStep->TrigDICount && pStep->TrigDICount > 0)
            return true;
    }

    return false;
}

/**
 * @brief 执行单步切换
 * @details 1. CDMA搬运波形; 2. 写入DO; 3. 启动定时器
 */
static void Execute_Step(int stepIndex)
{
    // 1. 边界与循环检查
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

    // 2. 准入检查：如果当前 DI 状态已满足触发条件，则直接跳过该步
    Struct_Seq_Step *pCheckStep = &g_StateSequenceTask.Steps[stepIndex];

    // 如果满足触发条件 (且不是无条件执行)
    if (Check_Step_Condition_Met(pCheckStep))
    {
        xil_printf("CPU1: StateSeq - Step %d Condition Pre-Met! Skipping...\r\n", stepIndex);

        // 记录该步执行结果：触发成功，耗时 0ms
        Record_Step_Result(stepIndex, true, 0.0);

        // 计算下一步 (模拟 DI 触发时的跳转逻辑)
        int nextStep = -1;
        // JumpTo: 0=Next, -1=End(Triggered), -2=Next
        if (pCheckStep->JumpTo == 0 || pCheckStep->JumpTo == -2)
        {
            nextStep = stepIndex + 1;
        }

        else if (pCheckStep->JumpTo == -1)
        {
            nextStep = g_StateSeqRuntime.TotalSteps; // Jump to End
            // [核心修改] 强制结束整个任务，清除剩余循环次数
            // 这样 Execute_Step(TotalSteps) 时会直接进入 Stop 分支，而不会进入 Loop 分支
            g_StateSeqRuntime.RepeatCountRemaining = 0;
        }

        else if (pCheckStep->JumpTo > 0)
        {
            nextStep = pCheckStep->JumpTo - 1;
        }

        // 递归调用，尝试执行下一步
        // (如果下一步条件也满足，会继续递归跳过，直到找到一个需要等待的步或结束)
        Execute_Step(nextStep);
        return; // 直接返回，不执行下面的硬件加载
    }

    g_StateSeqRuntime.CurrentStepIndex = stepIndex;
    Step_Hw_Params_t *pHw = &g_StateSeqRuntime.StepParams[stepIndex];
    Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[stepIndex];

    // 3. 启动 CDMA 搬运波形 (DDR -> BRAM)(更新波形，自带缩放)
    Load_Step_To_BRAM(stepIndex);

    // 4. 写入硬件参数 写入频率分频系数 (Offset 0x04)
    Xil_Out32(dac_whole_base_addr + 4, pHw->Freq_Divisor);

    // 5. 更新 DO
    if (pStep->DOCount > 0)
    {
        // 这里需要更复杂的逻辑合并全局DO，暂时直接写
        OnOff_Write_Continuous(pHw->DO_State);
    }

    // 6. 启动定时器
    if (pStep->MaxDuration > 0)
    {
        Start_Step_Timer(pStep->MaxDuration);
    }

    // 7. 录波启动逻辑
    // RecStartState: 0=不录, >=1 代表从第几步开始录
    if (g_StateSequenceTask.RecStartState > 0 &&
        (stepIndex + 1) == g_StateSequenceTask.RecStartState)
    {
        // 只有在第一次运行序列时才触发 (RepeatCountRemaining == RepeatCount)
        if (g_StateSeqRuntime.RepeatCountRemaining == g_StateSequenceTask.RepeatCount)
        {
            xil_printf("CPU1: StateSeq - Triggering SetTaskWaveRecord at Step %d\r\n", stepIndex + 1);
            // 启动录波 (传入设定的时长，建议设为 -1 无限长)
            WaveRecord_Start(g_StateSequenceTask.RecMS, "SetTaskStateSequence");
        }
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
 * @brief 阶段1：任务规划 (计算密集型)
 * @details 执行所有耗时操作：解析参数、全局量程扫描、DDR波形生成、寄存器值预算。
 * 此函数不会操作硬件输出，也不会启动定时器。
 */
void StateSequence_Plan(const char *startTimeStr)
{
    // 1. 先让装置输出全部为0
    memset(TempWaveData, 0, sizeof(TempWaveData));

    // 写入 DDR (临时借用 Step 0 的位置，反正下面计算流程会覆盖它)
    u32 ZeroSrcAddr = STATE_SEQ_DDR_BUFFER_BASE;
    u32 *pDdrBase = (u32 *)(UINTPTR)ZeroSrcAddr;

    // 简单的内存拷贝，将 0 数据填入 DDR
    // TempWaveData 是 uint16_t [8][1024]，总大小正好是 WAVE_STEP_SIZE_BYTES
    memcpy(pDdrBase, TempWaveData, WAVE_STEP_SIZE_BYTES);

    //  刷 Cache (确保 DDR 里真的是 0)
    Xil_DCacheFlushRange((UINTPTR)pDdrBase, WAVE_STEP_SIZE_BYTES);

    //  【关键动作】通过 CDMA 搬运到 BRAM
    int Status = XAxiCdma_SimpleTransfer(&CdmaInstance, (UINTPTR)ZeroSrcAddr, (UINTPTR)STATE_SEQ_BRAM_BASEADDR, WAVE_STEP_SIZE_BYTES, NULL, NULL);

    if (Status == XST_SUCCESS)
    {
        // 等待传输完成
        int timeout = 1000000;
        while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
            timeout--;

        if (timeout == 0)
        {
            xil_printf("CPU1: Debug - CDMA Timeout during Zero Write! (Bus Locked?)\r\n");
            // 如果超时，尝试复位一下，以免影响后面的正常流程
            XAxiCdma_Reset(&CdmaInstance);
            while (!XAxiCdma_ResetIsDone(&CdmaInstance))
                ;
        }
        else
        {
            // xil_printf("CPU1: Debug - Zero Write Complete. Output should be 0V now.\r\n");
        }
    }
    else
    {
        xil_printf("CPU1: Debug - CDMA Submit Failed %d\r\n", Status);
    }
    // 先输出10ms
    usleep(10000);
    // ============================================================

    // 2. 初始化基础状态
    g_StateSeqRuntime.CurrentStepIndex = 0;
    g_StateSeqRuntime.TotalSteps = g_StateSequenceTask.StepCount;
    g_StateSeqRuntime.RepeatCountRemaining = g_StateSequenceTask.RepeatCount;

    g_StateSeqRuntime.IsRunning = false;
    g_StateSeqRuntime.IsWaiting = true; // 进入等待状态
    g_StateSeqRuntime.IsHolding = false;
    g_StateSeqRuntime.IsFinished = false;

    g_StateSeqRuntime.ExecutedCount = 0;
    g_StateSeqRuntime.ReportedCount = 0; // [关键] 保持为0，不立即上报！等待 Run 时再设为 -1

    memset(g_StateSeqRuntime.ExecResults, 0, sizeof(g_StateSeqRuntime.ExecResults));

    // 保存启动时间字符串
    if (startTimeStr)
    {
        strncpy(g_StateSeqRuntime.StartTimeStr, startTimeStr, 31);
    }
    else
    {
        In_CurrTime curr;
        read_current_time(&curr);
        sprintf(g_StateSeqRuntime.StartTimeStr, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
                curr.curr_year, curr.curr_month, curr.curr_day,
                curr.curr_hour, curr.curr_minute, curr.curr_second,
                (unsigned int)(curr.curr_subsec / 100000));
    }

    xil_printf("CPU1: StateSeq - Planning task for %s...\r\n", g_StateSeqRuntime.StartTimeStr);

    // 3. 全局扫描：确定最佳量程
    float max_u_vals[4] = {0};
    float max_i_vals[4] = {0};

    for (int i = 0; i < g_StateSequenceTask.StepCount; i++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[i];
        for (int j = 0; j < pStep->ACCount; j++)
        {
            Struct_Seq_AC *pAC = &pStep->ACs[j];
            if (pAC->Line != 1)
                continue;
            int chn_idx = pAC->Chn - 1;
            if (chn_idx >= 0 && chn_idx < 4)
            {
                if (pAC->U > max_u_vals[chn_idx])
                    max_u_vals[chn_idx] = pAC->U;
                if (pAC->I_ > max_i_vals[chn_idx])
                    max_i_vals[chn_idx] = pAC->I_;
            }
        }
    }

    // 4. 计算量程并回写
    float final_ur[4], final_ir[4];
    u32 range_codes[8];
    for (int k = 0; k < 8; k++)
    {
        // 设置默认量程
        range_codes[k] = voltage_to_output(6);
    }

    for (int chn = 0; chn < 4; chn++)
    {
        // 电压量程
        if (max_u_vals[chn] > 3.25f)
            final_ur[chn] = 6.5f;
        else if (max_u_vals[chn] > 1.876f)
            final_ur[chn] = 3.25f;
        else
            final_ur[chn] = 1.876f;
        range_codes[chn] = voltage_to_output(final_ur[chn]);

        // 电流量程
        if (max_i_vals[chn] > 1.0f)
            final_ir[chn] = 5.0f;
        else if (max_i_vals[chn] > 0.2f)
            final_ir[chn] = 1.0f;
        else
            final_ir[chn] = 0.2f;
        range_codes[chn + 4] = current_to_output(final_ir[chn]);
    }

    // 回写量程到 Steps
    for (int i = 0; i < g_StateSequenceTask.StepCount; i++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[i];
        for (int j = 0; j < pStep->ACCount; j++)
        {
            if (pStep->ACs[j].Line == 1)
            {
                int idx = pStep->ACs[j].Chn - 1;
                if (idx >= 0 && idx < 4)
                {
                    pStep->ACs[j].UR = final_ur[idx];
                    pStep->ACs[j].IR = final_ir[idx];
                }
            }
        }
    }

    // 5. 执行预计算 (写入DDR，计算TTC等)
    Precalculate_Waveforms();
    Precalculate_HwParams();

    // 6. 将计算好的硬件寄存器值存入缓存，此时不写硬件
    // 缓存量程寄存器
    g_StateSeqRuntime.Cached_Hw.Range_Regs[0] = (range_codes[1] << 24) | (range_codes[0] << 8);
    g_StateSeqRuntime.Cached_Hw.Range_Regs[1] = (range_codes[3] << 24) | (range_codes[2] << 8);
    g_StateSeqRuntime.Cached_Hw.Range_Regs[2] = (range_codes[5] << 24) | (range_codes[4] << 8);
    g_StateSeqRuntime.Cached_Hw.Range_Regs[3] = (range_codes[7] << 24) | (range_codes[6] << 8);

    // 缓存幅值寄存器 (100% Gain)
    u32 din_vals[8] = {0};
    for (int chn = 0; chn < 4; chn++)
    {
        int idx_u = get_voltage_index_by_value(final_ur[chn]);
        din_vals[chn] = (u32)(1.0 * calculate_correction(chn, idx_u, 100.0f));

        int idx_i = get_current_index_by_value(final_ir[chn]);
        din_vals[chn + 4] = (u32)(1.0 * calculate_correction(chn + 4, idx_i, 100.0f));
    }
    g_StateSeqRuntime.Cached_Hw.Value_Regs[0] = (din_vals[1] << 16) | (din_vals[0] & 0xFFFF);
    g_StateSeqRuntime.Cached_Hw.Value_Regs[1] = (din_vals[3] << 16) | (din_vals[2] & 0xFFFF);
    g_StateSeqRuntime.Cached_Hw.Value_Regs[2] = (din_vals[5] << 16) | (din_vals[4] & 0xFFFF);
    g_StateSeqRuntime.Cached_Hw.Value_Regs[3] = (din_vals[7] << 16) | (din_vals[6] & 0xFFFF);

    // 缓存初始频率
    g_StateSeqRuntime.Cached_Hw.Init_Freq_Div = g_StateSeqRuntime.StepParams[0].Freq_Divisor;

    // 强制把 Cache 里的数据全刷进 DDR，并给一点时间让硬件完成 Write Buffer 的清空
    Xil_DCacheFlush();
    usleep(20000); // 休息 20ms
    xil_printf("CPU1: StateSeq - Plan Complete. Ready to trigger.\r\n");
}

/**
 * @brief 阶段2：执行任务 (IO密集型)
 * @details 仅执行寄存器写入和启动操作，耗时极短。通常在中断或立即启动时调用。
 */
/**
 * @brief 阶段2：执行任务 (IO密集型)
 * @details 修复了手动模式干扰导致的启动冲击问题
 */
void StateSequence_ApplyAndRun(void)
{
    // 1. 设置运行标志
    g_StateSeqRuntime.IsWaiting = false;  // 结束等待
    g_StateSeqRuntime.IsRunning = true;   // 开始运行
    g_StateSeqRuntime.IsHolding = false;  // 结束保持标志
    g_StateSeqRuntime.IsFinished = false; // 完成标志
    g_StateSeqRuntime.ReportedCount = -1;

    // 2. 设置硬件系数
    // 档位
    Seq_Hw_SetRange(g_StateSeqRuntime.Cached_Hw.Range_Regs[0],
                    g_StateSeqRuntime.Cached_Hw.Range_Regs[1],
                    g_StateSeqRuntime.Cached_Hw.Range_Regs[2],
                    g_StateSeqRuntime.Cached_Hw.Range_Regs[3]);
    // 幅值 (状态序列模式下，这里通常被设为 100% 满增益)
    Seq_Hw_SetValue(g_StateSeqRuntime.Cached_Hw.Value_Regs[0],
                    g_StateSeqRuntime.Cached_Hw.Value_Regs[1],
                    g_StateSeqRuntime.Cached_Hw.Value_Regs[2],
                    g_StateSeqRuntime.Cached_Hw.Value_Regs[3]);

    // 3. 开启 DAC
    Xil_Out32(dac_whole_base_addr + 0, 1);
    Xil_Out32(dac_whole_base_addr + 4, g_StateSeqRuntime.Cached_Hw.Init_Freq_Div);
    Xil_Out32(dac_whole_base_addr + 8, 0xFF);

    // 4. 执行第一步逻辑
    Execute_Step(0);
}

void StateSequence_Stop(void)
{
    // 如果不在运行，直接返回 (防止重复调用) 允许在等待状态下停止
    if (!g_StateSeqRuntime.IsRunning && !g_StateSeqRuntime.IsWaiting)
        return;

    // 1. 停止定时器
    XTtcPs_Stop(&SeqTtcInstance);

    // 2. 如果处于等待状态，必须关闭硬件闹钟
    if (g_StateSeqRuntime.IsWaiting)
    {
        xil_printf("CPU1: StateSeq - Cancelling Alarm...\r\n");
        g_SoftTimer_Reg15_Shadow &= ~STIMER_ALARM_EN_MASK; // 清除使能位
        Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG15, g_SoftTimer_Reg15_Shadow);
        g_StateSeqRuntime.IsWaiting = false;
    }

    // 3. 停止录波
    //  即使录波已经因为时长到达自动停止，调用此函数也是安全的
    WaveRecord_Stop();

    // 4. 切换状态标志
    g_StateSeqRuntime.IsRunning = false;
    g_StateSeqRuntime.IsHolding = true; // [新增] 进入保持模式，阻断主循环刷新
    g_StateSeqRuntime.IsFinished = true;

    // 标记系统状态为运行 (保持输出)
    devState.bACMeterMode = 0;
    devState.nStatusFund = 1;

    xil_printf("CPU1: StateSeq - Stopped.\r\n");
}

// 强制退出状态序列模式 (供外部指令抢占使用)
void StateSequence_QuitMode(void)
{
    // 如果正在运行，先停止定时器
    if (g_StateSeqRuntime.IsRunning)
    {
        XTtcPs_Stop(&SeqTtcInstance);
    }

    //  如果在等待闹钟，也要关掉
    if (g_StateSeqRuntime.IsWaiting)
    {
        g_SoftTimer_Reg15_Shadow &= ~STIMER_ALARM_EN_MASK;
        Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG15, g_SoftTimer_Reg15_Shadow);
        g_StateSeqRuntime.IsWaiting = false;
    }

    // 停止录波
    WaveRecord_Stop();

    // 彻底清除所有标志，释放控制权给主循环
    g_StateSeqRuntime.IsRunning = false;
    g_StateSeqRuntime.IsHolding = false;
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
        {
            nextStep = g_StateSeqRuntime.TotalSteps;
            g_StateSeqRuntime.RepeatCountRemaining = 0; // 强制结束整个任务，清除剩余循环次数
        }

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
    cJSON_AddStringToObject(report, "FunCode", "SetTaskStateSequence");

    // 状态判断逻辑
    // 只要任务处于“活跃”状态（无论是正在跑 IsRunning，还是定时等待中），只要没 Finish，都算 Doing。
    if (g_StateSeqRuntime.IsFinished)
    {
        cJSON_AddStringToObject(report, "Result", "Success");
    }
    else
    {
        // 包括 IsRunning=true (运行中) 和 IsRunning=false (定时等待中/启动瞬间)
        cJSON_AddStringToObject(report, "Result", "Doing");
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
        // printf("CPU1: [DEBUG] Sending SetTaskStateSequence Report: %s\r\n", string);
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

// ================= 定时启动功能实现 =================
/**
 * @brief 解析时间字符串并设置软时钟闹钟
 * @param startTimeStr 格式: "2025-01-01 12:00:00.123"
 * @return XST_SUCCESS 或 XST_FAILURE
 */
int StateSequence_EnableAlarm(const char *startTimeStr)
{
    if (startTimeStr == NULL || strlen(startTimeStr) < 19)
        return XST_FAILURE;

    // ============================================================
    // 【关键修复】在做任何规划前，强制关闭 DAC！
    // ============================================================
    // 1. 关闭 DAC 输出使能 (释放 BRAM 占用)
    Xil_Out32(dac_whole_base_addr + 0, 0);

    // 3. 延时一小会儿，确保总线彻底释放
    usleep(1000);

    // 2. 解析时间并设置闹钟 (同之前)
    char h_str[3] = {startTimeStr[11], startTimeStr[12], '\0'};
    char m_str[3] = {startTimeStr[14], startTimeStr[15], '\0'};
    char s_str[3] = {startTimeStr[17], startTimeStr[18], '\0'};
    int hour = atoi(h_str);
    int min = atoi(m_str);
    int sec = atoi(s_str);

    xil_printf("CPU1: Scheduling Alarm at %02d:%02d:%02d\r\n", hour, min, sec);

    uint32_t bcd_h = ((hour / 10) << 4) | (hour % 10);
    uint32_t bcd_m = ((min / 10) << 4) | (min % 10);
    uint32_t bcd_s = ((sec / 10) << 4) | (sec % 10);

    // 使用影子寄存器设置
    g_SoftTimer_Reg15_Shadow &= STIMER_RDSERIAL_EN_MASK;
    g_SoftTimer_Reg15_Shadow |= STIMER_ALARM_EN_MASK;
    g_SoftTimer_Reg15_Shadow |= (bcd_h << STIMER_ALARM_HOUR_SHIFT);
    g_SoftTimer_Reg15_Shadow |= (bcd_m << STIMER_ALARM_MIN_SHIFT);
    g_SoftTimer_Reg15_Shadow |= (bcd_s << STIMER_ALARM_SEC_SHIFT);

    Xil_Out32(SoftTimer_BASEADDR + SoftTimer_REG15, g_SoftTimer_Reg15_Shadow);

    // 1. 先进行任务规划 (计算参数，写DDR)
    // 这样当闹钟响时，数据已经准备好了
    StateSequence_Plan(startTimeStr);

    return XST_SUCCESS;
}