#include "StateSequence.h"
#include "ADDA.h"
#include "Amplifier_Switch.h" // 需要用到 voltage_to_output, calculate_correction
#include "xil_cache.h"
#include "xil_printf.h"
#include "sleep.h"

// 全局变量
StateSeq_Runtime_t g_StateSeqRuntime;
static XAxiCdma CdmaInstance;

// 全局定义 TTC 实例
XTtcPs SeqTtcInstance;

// 临时波形缓冲区 (8通道)
static uint16_t TempWaveData[8][DATA_LEN];

/**
 * @brief 写入量程 (模仿 power_amplifier_control 的 Range 配置部分)
 * @details 对应逻辑: 595置1 1595置0; 功放start清0 -> 写数据 -> 功放start置1
 */
static void Seq_Hw_SetRange(u32 r0, u32 r1, u32 r2, u32 r3)
{
    u32 BaseAddr = Amplifier_OnOff_BASEADDR;

    // 1. 复位/配置 595 (Range Enable?)
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000000);
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000002); // Bit 1 = 1

    // 2. 写入量程数据
    Xil_Out32(BaseAddr + Amplifier_Din0_ADDR, r0);
    Xil_Out32(BaseAddr + Amplifier_Din1_ADDR, r1);
    Xil_Out32(BaseAddr + Amplifier_Din2_ADDR, r2);
    Xil_Out32(BaseAddr + Amplifier_Din3_ADDR, r3);

    // 3. 触发更新 (Start=1)
    Xil_Out32(BaseAddr + Amplifier_Status_ADDR, 0x00000102); // Bit 8=1, Bit 1=1
}

/**
 * @brief 写入幅值系数 (模仿 power_amplifier_control 的 Value 配置部分)
 * @details 对应逻辑: 595置0 1595置1; 功放start清0 -> 写数据 -> 功放start置1
 */
static void Seq_Hw_SetValue(u32 v0, u32 v1, u32 v2, u32 v3)
{
    u32 BaseAddr = Amplifier_OnOff_BASEADDR;

    // 1. 复位/配置 1595 (Value Enable?)
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
 * @brief 预计算波形数据 (存入DDR)
 * @details
 * DDR数据布局 (对应 BRAM 128位宽):
 * [31:0]   Ch1|Ch0 (U_B|U_A)
 * [63:32]  Ch3|Ch2 (U_X|U_C)
 * [95:64]  Ch5|Ch4 (I_B|I_A)
 * [127:96] Ch7|Ch6 (I_X|I_C)
 */
static void Precalculate_Waveforms()
{
    xil_printf("CPU1: StateSequence - Pre-calculating waveforms ...\r\n");
    // xil_printf("CPU1: [DEBUG] Generating STATIC DC waveform for testing...\r\n");
    for (int step = 0; step < g_StateSequenceTask.StepCount; step++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[step];

        // 1. 清空临时缓冲区
        memset(TempWaveData, 0, sizeof(TempWaveData));

        // 2. 遍历AC配置，生成波形形状
        for (int i = 0; i < pStep->ACCount; i++)
        {
            Struct_Seq_AC *pAC = &pStep->ACs[i];

            // 仅处理 Line 1
            if (pAC->Line != 1)
                continue;

            // 逻辑通道 0-3 (对应 A, B, C, X)
            int chn_logical = pAC->Chn - 1;
            if (chn_logical < 0 || chn_logical > 3)
                continue;

            // ----------------------------------------------------
            // 2.1 电压通道 (物理通道 0-3)
            // ----------------------------------------------------
            int u_hw_idx = chn_logical;
            float u_harm_amps[MAX_HARMONICS] = {0};
            float u_harm_phases[MAX_HARMONICS] = {0};

            // 提取电压谐波 (Harmonics[0]是2次)
            for (int h = 0; h < pAC->HarmCount; h++)
            {
                int order = pAC->Harms[h].HN;
                if (order >= 2 && order < (MAX_HARMONICS + 2))
                {
                    u_harm_amps[order - 2] = pAC->Harms[h].U / 100.0f; // 百分比转小数
                    u_harm_phases[order - 2] = pAC->Harms[h].PhU;
                }
            }
            // 生成电压波形 (满量程，不缩放)
            addHarmonics(TempWaveData[u_hw_idx], DATA_LEN, pAC->PhU, MAX_HARMONICS, u_harm_amps, u_harm_phases, true, true);

            // ----------------------------------------------------
            // 2.2 电流通道 (物理通道 4-7)
            // ----------------------------------------------------
            int i_hw_idx = chn_logical + 4;
            float i_harm_amps[MAX_HARMONICS] = {0};
            float i_harm_phases[MAX_HARMONICS] = {0};

            // 提取电流谐波
            for (int h = 0; h < pAC->HarmCount; h++)
            {
                int order = pAC->Harms[h].HN;
                if (order >= 2 && order < (MAX_HARMONICS + 2))
                {
                    i_harm_amps[order - 2] = pAC->Harms[h].I_ / 100.0f;
                    i_harm_phases[order - 2] = pAC->Harms[h].PhI;
                }
            }
            // 生成电流波形 (满量程，不缩放)
            addHarmonics(TempWaveData[i_hw_idx], DATA_LEN, pAC->PhI, MAX_HARMONICS, i_harm_amps, i_harm_phases, true, true);
        }

        // 3. 打包数据写入 DDR (32位 = 高16位[Ch+1] | 低16位[Ch])
        u32 *pDdrStepBase = (u32 *)(UINTPTR)(STATE_SEQ_DDR_BUFFER_BASE + step * WAVE_STEP_SIZE_BYTES);

        for (int j = 0; j < DATA_LEN; j++)
        {
            pDdrStepBase[j * 4 + 0] = (TempWaveData[1][j] << 16) | TempWaveData[0][j]; // UB|UA
            pDdrStepBase[j * 4 + 1] = (TempWaveData[3][j] << 16) | TempWaveData[2][j]; // UX|UC
            pDdrStepBase[j * 4 + 2] = (TempWaveData[5][j] << 16) | TempWaveData[4][j]; // IB|IA
            pDdrStepBase[j * 4 + 3] = (TempWaveData[7][j] << 16) | TempWaveData[6][j]; // IX|IC
        }

        Xil_DCacheFlushRange((UINTPTR)pDdrStepBase, WAVE_STEP_SIZE_BYTES);
        // [调试] 打印每步的第一个数据点，确认DDR中有数据
        // xil_printf("  [DEBUG] Step %d DDR Sample[0]: 0x%08X (Ch1|Ch0)\r\n", step, pDdrStepBase[0]);
    }
}

/**
 * @brief 预计算硬件寄存器参数 (二级功放系数 & 定时器)
 * @details 这里执行幅值缩放和校准计算，生成 Din0-Din3 的最终值。
 */
static void Precalculate_HwParams()
{
    xil_printf("CPU1: StateSequence - Pre-calculating HW Params (Amplifier Coefficients)...\r\n");

    for (int step = 0; step < g_StateSequenceTask.StepCount; step++)
    {
        Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[step];
        Step_Hw_Params_t *pHw = &g_StateSeqRuntime.StepParams[step];

        u32 Din_Regs[8] = {0};    // 0-3: U, 4-7: I
        pHw->Freq_Divisor = 1953; // 默认初始化频率为 50Hz (1953)，防止未指定

        for (int i = 0; i < pStep->ACCount; i++)
        {
            Struct_Seq_AC *pAC = &pStep->ACs[i];
            if (pAC->Line != 1)
                continue;

            // --- 获取频率并计算分频系数 ---
            // 假设所有通道频率一致，取第一个有效的即可
            if (pAC->F > 0.1f)
            {
                // 公式：SystemClock / Freq / Points
                // 100,000,000 / F / 1024
                pHw->Freq_Divisor = (u32)(100000000.0f / pAC->F / (float)DATA_LEN);
            }

            int chn_logical = pAC->Chn - 1; // 0-3
            if (chn_logical < 0 || chn_logical > 3)
                continue;

            // --- 电压系数计算 ---
            // 1. 获取档位索引
            int idx_u = get_voltage_index_by_value(pAC->UR);
            // 2. 计算幅值百分比
            float amp_pct_u = 0.0f;
            if (pAC->UR > 0.001f)
                amp_pct_u = (pAC->U / pAC->UR) * 100.0f;
            // 3. 获取校准系数 (线性拟合)
            double correction_u = calculate_correction(chn_logical, idx_u, amp_pct_u);
            // 4. 计算最终寄存器值: (百分比/100) * 校准系数
            // 注意：这里没有PID，假设为开环
            Din_Regs[chn_logical] = (u32)((amp_pct_u / 100.0f) * correction_u);

            // --- 电流系数计算 ---
            int idx_i = get_current_index_by_value(pAC->IR);
            float amp_pct_i = 0.0f;
            if (pAC->IR > 0.001f)
                amp_pct_i = (pAC->I_ / pAC->IR) * 100.0f;
            double correction_i = calculate_correction(chn_logical + 4, idx_i, amp_pct_i);
            Din_Regs[chn_logical + 4] = (u32)((amp_pct_i / 100.0f) * correction_i);
        }

        // 组合寄存器值
        pHw->Din0_Value = (Din_Regs[1] << 16) | (Din_Regs[0] & 0xFFFF); // UB | UA
        pHw->Din1_Value = (Din_Regs[3] << 16) | (Din_Regs[2] & 0xFFFF); // UX | UC
        pHw->Din2_Value = (Din_Regs[5] << 16) | (Din_Regs[4] & 0xFFFF); // IB | IA
        pHw->Din3_Value = (Din_Regs[7] << 16) | (Din_Regs[6] & 0xFFFF); // IX | IC

        // 预计算 DO 状态
        pHw->DO_State = 0; // 需要结合全局状态或当前步状态，这里简化为只由该步决定
        for (int k = 0; k < pStep->DOCount; k++)
        {
            if (pStep->DOs[k].Val)
                pHw->DO_State |= (1 << (pStep->DOs[k].Chn + 23));
        }

        // [调试] 打印计算出的系数，确认不是0
        // xil_printf("  [DEBUG] Step %d Din0: 0x%08X (Should be non-zero)\r\n", step, pHw->Din0_Value);
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
        // 超时后必须复位，否则下次还是死
        XAxiCdma_Reset(&CdmaInstance);
        while (!XAxiCdma_ResetIsDone(&CdmaInstance))
            ;
    }
}

/**
 * @brief 设置并启动步进定时器 (TTC)
 * @comment [核心修正] 直接计算 Interval，不再依赖不准确的 Freq 计算
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
 * @details 1. CDMA搬运波形; 2. 写入功放系数; 3. 启动定时器
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

    // 1. 启动 CDMA 搬运波形 (DDR -> BRAM)
    Load_Step_To_BRAM(stepIndex);

    // 2. 写入硬件参数
    // 写入频率分频系数 (Offset 0x04)
    Xil_Out32(dac_whole_base_addr + 4, pHw->Freq_Divisor);

    // 写入功放幅值
    Seq_Hw_SetValue(pHw->Din0_Value, pHw->Din1_Value, pHw->Din2_Value, pHw->Din3_Value);

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
    g_StateSeqRuntime.CurrentStepIndex = 0;
    g_StateSeqRuntime.TotalSteps = g_StateSequenceTask.StepCount;
    g_StateSeqRuntime.RepeatCountRemaining = g_StateSequenceTask.RepeatCount;
    g_StateSeqRuntime.IsRunning = true;

    Precalculate_Waveforms();
    Precalculate_HwParams();

    // 1. 档位配置 (Phase 1: Range Config)
    // [核心修改] 使用严格的时序写入量程 (Range Config)
    Struct_Seq_AC *pAC0 = &g_StateSequenceTask.Steps[0].ACs[0];
    u32 range_u = voltage_to_output(pAC0->UR);
    u32 range_i = current_to_output(pAC0->IR);

    // 组合成4个32位字: High16=Ch_Odd, Low16=Ch_Even
    // Din0: UB|UA, Din1: UX|UC ...
    u32 r_din0 = (range_u << 24) | (range_u << 8); // U
    u32 r_din1 = (range_u << 24) | (range_u << 8); // U
    u32 r_din2 = (range_i << 24) | (range_i << 8); // I
    u32 r_din3 = (range_i << 24) | (range_i << 8); // I

    // 执行量程写入序列
    Seq_Hw_SetRange(r_din0, r_din1, r_din2, r_din3);

    // 2. 开启 DA IP
    Xil_Out32(dac_whole_base_addr + 0, 1);
    Xil_Out32(dac_whole_base_addr + 4, g_StateSeqRuntime.StepParams[0].Freq_Divisor);
    Xil_Out32(dac_whole_base_addr + 8, 0xFF);

    // 3. 执行第一步 (Values, Waveform, Timer)
    Execute_Step(0);
}

void StateSequence_Stop(void)
{
    if (!g_StateSeqRuntime.IsRunning)
        return;

    XTtcPs_Stop(&SeqTtcInstance);

    // 关闭输出 (使用 Value Update 序列写入 0)
    Seq_Hw_SetValue(0, 0, 0, 0);

    // 关闭 DA IP
    Xil_Out32(dac_whole_base_addr + 8, 0x00);

    g_StateSeqRuntime.IsRunning = false;
    xil_printf("CPU1: StateSeq - Stopped.\r\n");
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
        xil_printf("CPU1: StateSeq - DI Triggered transition!\r\n");
        int nextStep = -1;

        XTtcPs_Stop(&SeqTtcInstance);

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

// ================= 测试函数：状态序列自测 (8通道满配版) =================
void Test_StateSequence_Scenario(void)
{
    xil_printf("\r\n=======================================================\r\n");
    xil_printf("CPU1: [TEST] Starting State Sequence (All 8 Channels)\r\n");
    xil_printf("Step0 3V 1A 2s\r\n");
    xil_printf("Step1 5V 2A 3s || DO1 ON\r\n");
    xil_printf("Step2 6V 5A 10s || Trig:{Chn:1, val:1} Jump To Stop || DO2 ON\r\n");
    xil_printf("Step3 1V 0A 2s\r\n");
    xil_printf("=======================================================\r\n");

    // 1. 清空全局任务结构体
    memset(&g_StateSequenceTask, 0, sizeof(g_StateSequenceTask));

    g_StateSequenceTask.StartMode = 0;
    g_StateSequenceTask.StepCount = 4;
    g_StateSequenceTask.RepeatCount = 0;

// --- 辅助宏：快速填充一个通道 ---
// Line固定为1
#define SET_AC_CHN(step_ptr, idx, ch_num, u_val, ph_u, i_val, ph_i) \
    (step_ptr)->ACs[idx].Line = 1;                                  \
    (step_ptr)->ACs[idx].Chn = ch_num;                              \
    (step_ptr)->ACs[idx].F = 50.0f;                                 \
    (step_ptr)->ACs[idx].UR = 6.5f; /* 锁定最大量程 */              \
    (step_ptr)->ACs[idx].IR = 5.0f;                                 \
    (step_ptr)->ACs[idx].U = u_val;                                 \
    (step_ptr)->ACs[idx].PhU = ph_u;                                \
    (step_ptr)->ACs[idx].I_ = i_val;                                \
    (step_ptr)->ACs[idx].PhI = ph_i;                                \
    (step_ptr)->ACs[idx].HarmCount = 0; // 无谐波

    // --- Step 0: 初始状态 (2秒) ---
    // 三相 3.0V, 1A (正序: A=0, B=240, C=120)
    Struct_Seq_Step *s0 = &g_StateSequenceTask.Steps[0];
    s0->MaxDuration = 2000;
    s0->JumpTo = 0;
    s0->TrigLogic = -1;
    s0->ACCount = 4; // !!! 关键：开启4个逻辑通道，对应8个物理通道

    SET_AC_CHN(s0, 0, 1, 3.0f, 0.0f, 1.0f, 0.0f);     // Ch1: UA, IA
    SET_AC_CHN(s0, 1, 2, 3.0f, 240.0f, 1.0f, 240.0f); // Ch2: UB, IB (-120度)
    SET_AC_CHN(s0, 2, 3, 3.0f, 120.0f, 1.0f, 120.0f); // Ch3: UC, IC (+120度)
    SET_AC_CHN(s0, 3, 4, 3.0f, 0.0f, 1.0f, 180.0f);   // Ch4: UX, IX (零序/反相)

    // --- Step 1: 升压 (3秒) ---
    // 三相 5.0V, 2A
    Struct_Seq_Step *s1 = &g_StateSequenceTask.Steps[1];
    s1->MaxDuration = 3000;
    s1->JumpTo = 0;
    s1->TrigLogic = -1;
    s1->ACCount = 4;

    SET_AC_CHN(s1, 0, 1, 5.0f, 0.0f, 2.0f, 0.0f);
    SET_AC_CHN(s1, 1, 2, 5.0f, 240.0f, 2.0f, 240.0f);
    SET_AC_CHN(s1, 2, 3, 5.0f, 120.0f, 2.0f, 120.0f);
    SET_AC_CHN(s1, 3, 4, 5.0f, 0.0f, 2.0f, 180.0f);

    s1->DOCount = 1;
    s1->DOs[0].Chn = 1;
    s1->DOs[0].Val = 1; // DO1 ON

    // --- Step 2: 满幅值 & 等待触发 (10秒) ---
    // 三相 6.0V, 5A
    Struct_Seq_Step *s2 = &g_StateSequenceTask.Steps[2];
    s2->MaxDuration = 10000;
    s2->JumpTo = -1;
    s2->ACCount = 4;

    SET_AC_CHN(s2, 0, 1, 6.0f, 0.0f, 5.0f, 0.0f);
    SET_AC_CHN(s2, 1, 2, 6.0f, 240.0f, 5.0f, 240.0f);
    SET_AC_CHN(s2, 2, 3, 6.0f, 120.0f, 5.0f, 120.0f);
    SET_AC_CHN(s2, 3, 4, 6.0f, 0.0f, 5.0f, 180.0f);

    s2->DOCount = 1;
    s2->DOs[0].Chn = 2;
    s2->DOs[0].Val = 1; // DO2 ON

    s2->TrigLogic = 0;
    s2->TrigDICount = 1;
    s2->TrigDIs[0].Chn = 1;
    s2->TrigDIs[0].Val = 1;

    // --- Step 3: 结束态 (2秒) ---
    // 三相 1.0V, 0A
    Struct_Seq_Step *s3 = &g_StateSequenceTask.Steps[3];
    s3->MaxDuration = 2000;
    s3->JumpTo = -2;
    s3->ACCount = 4;

    SET_AC_CHN(s3, 0, 1, 1.0f, 0.0f, 0.0f, 0.0f);
    SET_AC_CHN(s3, 1, 2, 1.0f, 240.0f, 0.0f, 240.0f);
    SET_AC_CHN(s3, 2, 3, 1.0f, 120.0f, 0.0f, 120.0f);
    SET_AC_CHN(s3, 3, 4, 1.0f, 0.0f, 0.0f, 180.0f);

    s3->DOCount = 2;
    s3->DOs[0].Chn = 1;
    s3->DOs[0].Val = 1;
    s3->DOs[1].Chn = 2;
    s3->DOs[1].Val = 1;

    xil_printf("CPU1: [TEST] 8-Channel Sequence Configured.\r\n");

    StateSequence_PrepareAndStart();
}

/**
 * @brief CDMA DDR->DDR 回环测试
 * @return XST_SUCCESS 或 XST_FAILURE
 */
int StateSequence_Test_CDMA_Loopback(void)
{
    xil_printf("\r\n--- Starting CDMA DDR Loopback Test ---\r\n");

    u32 *SrcPtr = (u32 *)STATE_SEQ_DDR_BUFFER_BASE;
    u32 *DestPtr = (u32 *)STATE_SEQ_DDR_TEST_DEST;
    u32 Length = 1024 * 4; // 测试 4KB 数据 (1024个 words)
    int Status;

    // 1. 初始化源数据 (写入已知模式，如 0, 1, 2...)
    // 同时将目的地址清零，防止误判
    for (int i = 0; i < 1024; i++)
    {
        SrcPtr[i] = i;
        DestPtr[i] = 0;
    }

    // 2. 关键：刷新 Cache
    // CPU 刚写完数据在 Cache 里，还没到 DDR。CDMA 是从 DDR 读的。
    // 所以必须把 Src 刷入 DDR，并把 Dest 从 Cache 中失效（防止CPU读到旧的0）。
    Xil_DCacheFlushRange((UINTPTR)SrcPtr, Length);
    Xil_DCacheInvalidateRange((UINTPTR)DestPtr, Length);

    // 3. 检查 CDMA 是否空闲
    int timeout = 10000;
    while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
    {
        timeout--;
    }
    if (timeout == 0)
    {
        xil_printf("CPU1: [TEST FAIL] CDMA is Busy before start. Resetting...\r\n");
        XAxiCdma_Reset(&CdmaInstance);
        return XST_FAILURE;
    }

    // 4. 启动传输 (DDR -> DDR)
    xil_printf("CPU1: Transferring %d bytes from 0x%X to 0x%X...\r\n",
               Length, (unsigned int)SrcPtr, (unsigned int)DestPtr);

    Status = XAxiCdma_SimpleTransfer(&CdmaInstance, (UINTPTR)SrcPtr, (UINTPTR)DestPtr,
                                     Length, NULL, NULL);
    if (Status != XST_SUCCESS)
    {
        xil_printf("CPU1: [TEST FAIL] CDMA Submit Failed (Status %d)\r\n", Status);
        return XST_FAILURE;
    }

    // 5. 等待完成
    timeout = 1000000;
    while (XAxiCdma_IsBusy(&CdmaInstance) && timeout > 0)
    {
        timeout--;
    }
    if (timeout == 0)
    {
        xil_printf("CPU1: [TEST FAIL] CDMA Timeout! Hardware hung.\r\n");
        XAxiCdma_Reset(&CdmaInstance);
        return XST_FAILURE;
    }

    // 6. 再次失效 Cache (保险起见)
    // 确保 CPU 读取 DestPtr 时是从 DDR 拿最新的 CDMA 搬运过来的数据
    Xil_DCacheInvalidateRange((UINTPTR)DestPtr, Length);

    // 7. 数据校验
    int err_cnt = 0;
    for (int i = 0; i < 1024; i++)
    {
        if (DestPtr[i] != i)
        {
            err_cnt++;
            if (err_cnt < 5)
            { // 只打印前几个错误
                xil_printf("Error at index %d: Read 0x%X, Expected 0x%X\r\n", i, DestPtr[i], i);
            }
        }
    }

    if (err_cnt == 0)
    {
        xil_printf("CPU1: [TEST PASS] CDMA Loopback Successful!\r\n");
        return XST_SUCCESS;
    }
    else
    {
        xil_printf("CPU1: [TEST FAIL] Data Mismatch count: %d\r\n", err_cnt);
        return XST_FAILURE;
    }
}
