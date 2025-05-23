#include "ADDA.h"
/*
 * 变量
 */
XAxiDma axidma;  // XAxiDma实例
XScuGic intc;    // 中断控制器的实例
XScuTimer Timer; // 定时器驱动程序实例

// ad
int dma_rx_8[8][sample_points] = {0}; // 8个通道，每个通道的采样点数sample_points
u16 *rx_buffer_ptr = (u16 *)RX_BUFFER_BASE;
u16 *tx_buffer_ptr = (u16 *)TX_BUFFER_BASE;

// 波形修改参数
float Phase_shift[8] = {0, 120, 240, 0, 0, 120, 240, 0}; // 8路波形相位偏移 单位度
uint16_t enable = 0xff;                                  // 使能通道输出
float Wave_Frequency = 50;
float Wave_Amplitude[8] = {0, 0, 0, 0, 0, 0, 0, 0};
u32 Wave_Range[8] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
uint16_t Wave_NewData[8][DATA_LEN]; // 修改后8路通道所有数据

int numHarmonics[CHANNL_MAX] = {0};                      // 每个通道有几个谐波
float harmonics[CHANNL_MAX][MAX_HARMONICS] = {0};        // 每个通道每次谐波的幅值
float harmonics_phases[CHANNL_MAX][MAX_HARMONICS] = {0}; // 每个通道每次谐波的相位

ADC_Process_State adcState = ADC_STATE_IDLE; // 初始化ADC状态结构体，初始化为空闲状态
// 功放输出参数
double DA_Correct_100[8][3] = {
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
double DA_Correct_20[8][3] = {
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
double DA_CorrectPhase_100[8][3] = {
    // 电压通道 (UA, UB, UC, UX) - 分别对应 6.5V, 3.25V, 1.876V
    {0.0, 0.0, 0.0},          // UA 相位校准参数
    {-0.012, -0.006, -0.003}, // UB 相位校准参数
    {-0.017, -0.006, -0.001}, // UC 相位校准参数
    {0.0, 0.0, 0.0},          // UX 相位校准参数

    // 电流通道 (IA, IB, IC, IX) - 分别对应 5A, 1A, 0.2A
    {-0.205, -0.310, -0.184}, // IA 相位校准参数
    {-0.195, -0.293, -0.166}, // IB 相位校准参数
    {-0.204, -0.304, -0.177}, // IC 相位校准参数
    {0.0, 0.0, 0.0}           // IX 相位校准参数
};
// AD校准参数数组
double AD_Correct[8][3] = {
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

/**
 * @brief 启动一次ADC硬件采样和DMA传输 (单次批量操作)
 * @param TotalSamplePointsPerChannel 每个通道在此次采集中需要获取的总样本点数
 * @param SampleFrequency ADC的实际采样频率 (Hz)
 * @comment 此函数配置ADC IP核进行一次包含所有周期的连续采样，并启动DMA将所有数据传输到内存。
 */
void Adc_Start(int SamplePointsPerPeriod, int SampleFrequency, int NumSamplingPeriods)
{
    AdcFinish_Flag = 0;

    int total_sample_points = SamplePointsPerPeriod * NumSamplingPeriods; // ADC采样个数256×16周期
    uint32_t total_dma_bytes = total_sample_points * CHANNL_MAX * 16;     // total_sample_points *16位*8个通道

    /*1 开启ADC采样，设置采样点数和采样频率*/
    Xil_Out32(adc_whole_base_addr + 8, total_sample_points + 256 * 4); // 采样点：sample_points写256
    Xil_Out32(adc_whole_base_addr + 4, 99993600 / SampleFrequency);    // 7812，对应采样频率50*256

    Xil_Out32(adc_whole_base_addr + 0, 0); // 开启一次ADC
    Xil_Out32(adc_whole_base_addr + 0, 1);

    /*2 开启DMA传输*/
    int status = SafeDmaTransfer(&axidma, (UINTPTR)rx_buffer_ptr, total_dma_bytes, XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS)
    {
        // 处理DMA传输失败情况
        printf("CPU1: ADC DMA transfer failed, will retry next cycle\n");
    }

    // 接下来进入DMA传输完成函数。
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

// DMA RX中断处理函数 adc
void rx_intr_handler(void *callback)
{
    // 进入到该中断函数中代表DMA已经完成了一次传输
    uint32_t irq_status;
    int timeout;
    XAxiDma *axidma_inst = (XAxiDma *)callback;

    irq_status = XAxiDma_IntrGetIrq(axidma_inst, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrAckIrq(axidma_inst, irq_status, XAXIDMA_DEVICE_TO_DMA);

    // Rx出错
    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK))
    {

        XAxiDma_Reset(axidma_inst);
        timeout = RESET_TIMEOUT_COUNTER;
        printf("CPU1:DMA RX Interrupt Handler: Error Transfer\n");
        while (timeout)
        {
            if (XAxiDma_ResetIsDone(axidma_inst))
                break;
            timeout -= 1;
        }
        return;
    }

    // Rx完成
    if ((irq_status & XAXIDMA_IRQ_IOC_MASK))
    {

        u32 total_dma_bytes_received = sample_points * AD_SAMP_CYCLE_NUMBER * CHANNL_MAX * 16;

        sync_dma_buffer((UINTPTR)rx_buffer_ptr, total_dma_bytes_received, XAXIDMA_DEVICE_TO_DMA);
        Adc_Data_processing();
        // 设置ADC（及数据处理）完成标志，通知主循环数据已准备好可以进行后续分析（如FFT）
        AdcFinish_Flag = 1; //
    }
}

int SafeDmaTransfer(XAxiDma *AxiDmaInstPtr, UINTPTR BuffAddr, u32 Length, int Direction)
{
    int retry_count = 0;
    int max_retries = 5;
    int status = XST_FAILURE;

    while (retry_count < max_retries)
    {
        status = XAxiDma_SimpleTransfer(AxiDmaInstPtr, BuffAddr, Length, Direction);

        if (status == XST_SUCCESS)
        {
            return XST_SUCCESS;
        }

        printf("CPU1: DMA transfer failed, retrying (%d)...\n", retry_count + 1);
        retry_count++;
        usleep(1000); // 短暂延迟
    }

    printf("CPU1: DMA transfer failed after %d retries\n", max_retries);
    return status;
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

// 建立DMA中断系统
//   @param   int_ins_ptr是指向XScuGic实例的指针
//   @param   AxiDmaPtr是指向DMA引擎实例的指针
//   @param   tx_intr_id是TX通道中断ID
//   @param   rx_intr_id是RX通道中断ID
//   @return：成功返回XST_SUCCESS，否则返回XST_FAILURE
int setup_intr_system(XScuGic *int_ins_ptr, XAxiDma *axidma_ptr, XScuTimer *timer_ptr,
                      u16 rx_intr_id, u16 tx_intr_id, u16 underflow_id, u16 Timer_id)
{
    int status;
    XScuGic_Config *intc_config;

    // 初始化中断控制器驱动
    intc_config = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (NULL == intc_config)
    {
        return XST_FAILURE;
    }
    status = XScuGic_CfgInitialize(int_ins_ptr, intc_config,
                                   intc_config->CpuBaseAddress);
    if (status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    // 设置优先级和触发类型
    // ad
    XScuGic_SetPriorityTriggerType(int_ins_ptr, rx_intr_id, 8, 0x3);
    // da
    XScuGic_SetPriorityTriggerType(int_ins_ptr, tx_intr_id, 8, 0x3);
    XScuGic_SetPriorityTriggerType(int_ins_ptr, underflow_id, 8, 0x3);
    // 为定时器中断设置较低优先级
    XScuGic_SetPriorityTriggerType(int_ins_ptr, Timer_id, 0x20, 0x3);

    // 为中断设置中断处理函数
    XScuGic_Connect(int_ins_ptr, rx_intr_id, (Xil_InterruptHandler)rx_intr_handler, axidma_ptr);
    XScuGic_Connect(int_ins_ptr, tx_intr_id, (Xil_InterruptHandler)tx_intr_handler, axidma_ptr);
    XScuGic_Connect(int_ins_ptr, underflow_id, (Xil_InterruptHandler)underflow_handler, (void *)1);
    XScuGic_Connect(int_ins_ptr, Timer_id, (Xil_ExceptionHandler)timer_intr_handler, (void *)timer_ptr); // 定时器

    // 显式地将ADC和DMA中断映射到CPU1
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, rx_intr_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, tx_intr_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, underflow_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, Timer_id);

    // 使能
    // ad
    XScuGic_Enable(int_ins_ptr, rx_intr_id);
    // da
    XScuGic_Enable(int_ins_ptr, tx_intr_id);
    XScuGic_Enable(int_ins_ptr, underflow_id);
    // 定时器
    XScuGic_Enable(int_ins_ptr, Timer_id);
    // 启用来自硬件的中断
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 (void *)int_ins_ptr);
    Xil_ExceptionEnable();

    // 使能中断
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA); // DMA
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    XScuTimer_EnableInterrupt(timer_ptr); // 定时器
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
 * @brief 处理DMA传输完成后的ADC数据 (单个完整批次)
 *
 * 此函数在rx_intr_handler中被调用，当一次包含所有原始周期的ADC数据
 * 通过DMA完整传输到rx_buffer_ptr后执行。
 * 它负责将rx_buffer_ptr中的整个数据块，按照AD_SAMP_CYCLE_NUMBER个原始周期的结构，
 * 逐个原始周期地进行处理（码值转换），并将结果通过Xil_Out32写入到共享DDR内存(Share_addr)。
 *
 * @details
 * - rx_buffer_ptr 包含了所有 AD_SAMP_CYCLE_NUMBER 个“原始周期”的、所有通道的、交错的ADC数据。
 * - 外层循环遍历 AD_SAMP_CYCLE_NUMBER 个“原始周期”。
 * - 内层循环遍历每个“原始周期”内的 sample_points 个采样时刻。
 * - `current_rx_buffer_offset_u16` 用于追踪在 `rx_buffer_ptr` (u16指针) 中当前处理的“原始周期”数据块的起始索引。
 * - `k_rx` 作为 `rx_buffer_ptr` 的索引，在每个“原始周期”开始时基于 `current_rx_buffer_offset_u16` 初始化，
 * 并在内层循环中递增，以正确读取每个通道在每个采样时刻的数据。
 * - 写入 `Share_addr` 时，使用 `cycle_idx` (当前“原始周期”的索引) 来计算正确的内存偏移。
 */
void Adc_Data_processing()
{
    /************************** 数据处理 *****************************/
    // current_rx_buffer_offset_u16 用于追踪在 rx_buffer_ptr (u16类型指针) 中当前处理的“原始周期”数据块的起始索引
    int current_rx_buffer_offset_u16 = 0;

    // 外层循环：遍历所有 AD_SAMP_CYCLE_NUMBER 个“原始周期”的数据块
    for (int cycle_idx = 0; cycle_idx < AD_SAMP_CYCLE_NUMBER; cycle_idx++) //
    {
        // k_rx 指向 rx_buffer_ptr 中当前“原始周期”的第一个采样点数据 (即通道0的第0个点，或按实际交错顺序的第一个点)
        int k_rx = current_rx_buffer_offset_u16;

        // 内层循环：遍历当前“原始周期”内的 sample_points 个采样时刻
        for (int i = 0; i < sample_points; i++) // i 是当前周期内的采样点索引
        {
            // 假设ADC数据在rx_buffer_ptr中的交错顺序是：
            // 时刻0: CH4(JIA), CH0(JUA), CH5(JIB), CH1(JUB), CH6(JIC), CH2(JUC), CH7(JIX), CH3(JUX)
            // 所以 k_rx 对应 JIA, k_rx+1 对应 JUA, k_rx+2 对应 JIB, ..., k_rx+7 对应 JUX
            // 写入 Share_addr 时，目标地址结构是：
            // Share_addr_base + (通道逻辑偏移) + (周期偏移) + (周期内点偏移)
            // 通道逻辑偏移: channel_logic_id * sample_points * sizeof(u32)
            // 周期偏移:     cycle_idx * CHANNL_MAX * sample_points * sizeof(u32)
            // 周期内点偏移: i * sizeof(u32)

            // JUA (逻辑通道0, 物理索引1 in rx_buffer_ptr group)
            Xil_Out32(Share_addr + 0 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 1]));

            // JUB (逻辑通道1, 物理索引3)
            Xil_Out32(Share_addr + 1 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 3]));

            // JUC (逻辑通道2, 物理索引5)
            Xil_Out32(Share_addr + 2 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 5]));

            // JUX (逻辑通道3, 物理索引7)
            Xil_Out32(Share_addr + 3 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 7]));

            // JIA (逻辑通道4, 物理索引0)
            Xil_Out32(Share_addr + 4 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 0]));

            // JIB (逻辑通道5, 物理索引2)
            Xil_Out32(Share_addr + 5 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 2]));

            // JIC (逻辑通道6, 物理索引4)
            Xil_Out32(Share_addr + 6 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 4]));

            // JIX (逻辑通道7, 物理索引6)
            Xil_Out32(Share_addr + 7 * sample_points * sizeof(u32) + cycle_idx * CHANNL_MAX * sample_points * sizeof(u32) + i * sizeof(u32),
                      (u32)code_to_real(rx_buffer_ptr[k_rx + 6]));

            k_rx += CHANNL_MAX; // 移动到 rx_buffer_ptr 中下一组8通道交错数据 (即下一个采样时刻的数据)
        }
        // 当前“原始周期”的数据已处理完毕并写入Share_addr。
        // 更新 current_rx_buffer_offset_u16 以指向 rx_buffer_ptr 中下一个“原始周期”数据块的起始位置。
        // 由于 k_rx 已经在内层循环中正确递增并覆盖了一个周期的所有数据，
        // current_rx_buffer_offset_u16 应该直接等于下一个周期的起始 k_rx 值。
        current_rx_buffer_offset_u16 = k_rx;
    }
}

void Write_Wave_to_Wave_NewData()
{
    // 二维数组8*DATA_LEN     Wave_NewData中存储的是8个通道，每个通道DATA_LEN个点，正弦波

    for (int i = 0; i < CHANNL_MAX; i++)
    {
        addHarmonics(Wave_NewData[i], DATA_LEN, Phase_shift[i], numHarmonics[i], harmonics[i], harmonics_phases[i]);
    }

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

// 将波形写入BRAM
void str_wr_bram(PID_STATE pid_state)
{
    u32 data_2channl; // 32位数据，存放2个通道16位的波形
    u16 channel_cnt = 0;

    u16 frequency_divisor; // 分频系数 默认为1953
    // 修改波形

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

    // 修改波形数据
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
                     numHarmonics[i], harmonics[i], harmonics_phases[i]);
    }

    // 修改通道使能和分频系数
    frequency_divisor = 100000000 / Wave_Frequency / DATA_LEN; // 分频系数
    /*
     *  dds_enable     (slv_reg0[0])
     *  dds_freq_div   (slv_reg1[15:0])    //1953
     *  dds_channels   (slv_reg2[7:0])
     */
    Xil_Out32(dac_whole_base_addr + 0, 1); // start_dds写1,第17位
    Xil_Out32(dac_whole_base_addr + 4, (u32)frequency_divisor);
    Xil_Out32(dac_whole_base_addr + 8, (u32)enable);

    // 写入BRAM
    // 最大地址为32bit*4 *1024 = 16KB
    for (int j = 0; j < DATA_LEN; j++)
    {
        for (int i = 0; i < 4 * BRAM_DATA_BYTE; i += BRAM_DATA_BYTE)
        { // 更新了8通道的第一个采样点
            data_2channl = (Wave_NewData[channel_cnt + 1][j] << 16) + Wave_NewData[channel_cnt][j];
            XBram_WriteReg(XPAR_BRAM_0_BASEADDR, i + j * 4 * BRAM_DATA_BYTE, data_2channl);
            channel_cnt += 2;
        }
        channel_cnt = 0;
    }
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
void addHarmonics(uint16_t NewData[], int Array_length, float Base_Phase_Degrees, int numHarmonics, float harmonics[], float harmonics_phases[])
{
    // harmonics[0]为2次谐波
    // 遍历数组中的每个元素
    for (int i = 0; i < Array_length; i++)
    {
        // 计算基本波形的相位
        double phase = 2 * M_PI * i / Array_length; // 基本波形的相位
        // 添加基波相位偏移
        double shifted_phase = phase + (Base_Phase_Degrees * M_PI / 180.0); // 添加基波相位偏移
        // 计算基本正弦波的值
        double sum = sin(shifted_phase); // 基本正弦波

        // 添加谐波
        // 遍历谐波数组
        for (int j = 0; j < numHarmonics; j++)
        {
            // 计算谐波相位
            double harmonic_phase = (j + 2) * phase; // 基波的整数倍
            // 添加谐波相位偏移
            double shifted_harmonic_phase = harmonic_phase + harmonics_phases[j] * M_PI / 180.0; // 使用基波和谐波的相位偏移
            // 计算谐波值
            double harmonic_value = sin(shifted_harmonic_phase);

            // 添加谐波并乘以相应的幅值
            sum += harmonic_value * harmonics[j]; // 添加谐波并乘以相应的幅值
        }

        // 归一化并转换为 uint16_t 类型
        // 对计算出的和进行归一化，并转换为 uint16_t 类型
        NewData[i] = (uint16_t)((sum / (1.0 + sumHarmonics(harmonics, numHarmonics))) * 32768 + 32767);
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
