#include "ADDA.h"
/*
 * ����
 */
XAxiDma axidma;                     // XAxiDmaʵ��
XScuGic intc;                       // �жϿ�������ʵ��
XScuTimer Timer;                    // ��ʱ����������ʵ��
volatile int tx_done;               // ������ɱ�־
volatile int error = 0;             // ��������־
volatile int adcS_done;             // adc������ɱ�־
volatile u8 Timer_Flag;             // ��ʱ����ɱ�־
volatile u8 Current_DDR_Region = 0; // 0��ʾʹ��Share_addr_1��1��ʾʹ��Share_addr_2

// ad
int dma_rx_8[8][sample_points] = {0}; // 8��ͨ����ÿ��ͨ���Ĳ�������sample_points
u16 *rx_buffer_ptr = (u16 *)RX_BUFFER_BASE;
u16 *tx_buffer_ptr = (u16 *)TX_BUFFER_BASE;
volatile int ADC_Sampling_ddr = 0; // ���ڴ���д��������

// �����޸Ĳ���
float Phase_shift[8] = {0, 120, 240, 0, 0, 120, 240, 0}; // 8·������λƫ�� ��λ��
uint16_t enable = 0xff;                                       // ʹ��ͨ�����
float Wave_Frequency = 50;
float Wave_Amplitude[8] = {0, 0, 0, 0, 0, 0, 0, 0};
u32 Wave_Range[8] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
uint16_t Wave_NewData[8][DATA_LEN]; // �޸ĺ�8·ͨ����������

int numHarmonics[CHANNL_MAX] = {0};                      // ÿ��ͨ���м���г��
float harmonics[CHANNL_MAX][MAX_HARMONICS] = {0};        // ÿ��ͨ��ÿ��г���ķ�ֵ
float harmonics_phases[CHANNL_MAX][MAX_HARMONICS] = {0}; // ÿ��ͨ��ÿ��г������λ

// �����������
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
// ����20%��ֵʱ��У׼����
double DA_Correct_20[8][3] = {
    // ��ѹͨ�� (UA, UB, UC, UX) - �ֱ��Ӧ 6.5V, 3.25V, 1.876V
    {35731.608731, 35708.515107, 38471.090857}, // UA 20%��ֵУ׼����
    {35789.473684, 35806.869734, 38471.090857}, // UB 20%��ֵУ׼����
    {35731.608731, 35702.746365, 38503.990878}, // UC 20%��ֵУ׼����
    {35731.608731, 35702.746365, 38503.990878}, // UX 20%��ֵУ׼����

    // ����ͨ�� (IA, IB, IC, IX) - �ֱ��Ӧ 5A, 1A, 0.2A
    {35453.597497, 41052.631579, 41935.483871}, // IA 20%��ֵУ׼����
    {35789.473684, 41031.036297, 41935.483871}, // IB 20%��ֵУ׼����
    {35453.597497, 41052.631579, 41379.310345}, // IC 20%��ֵУ׼����
    {35453.597497, 41052.631579, 41379.310345}  // IX 20%��ֵУ׼����
};

// ��λУ׼�������飨��λ���ȣ�
double DA_CorrectPhase_100[8][3] = {
    // ��ѹͨ�� (UA, UB, UC, UX) - �ֱ��Ӧ 6.5V, 3.25V, 1.876V
    {0.0, 0.0, 0.0},          // UA ��λУ׼����
    {-0.012, -0.006, -0.003}, // UB ��λУ׼����
    {-0.017, -0.006, -0.001}, // UC ��λУ׼����
    {0.0, 0.0, 0.0},          // UX ��λУ׼����

    // ����ͨ�� (IA, IB, IC, IX) - �ֱ��Ӧ 5A, 1A, 0.2A
    {-0.205, -0.310, -0.184}, // IA ��λУ׼����
    {-0.195, -0.293, -0.166}, // IB ��λУ׼����
    {-0.204, -0.304, -0.177}, // IC ��λУ׼����
    {0.0, 0.0, 0.0}           // IX ��λУ׼����
};
// ADУ׼��������
double AD_Correct[8][3] = {
    // ��ѹͨ�� (UA, UB, UC, UX) - �ֱ��Ӧ 6.5V, 3.25V, 1.876V
    {20171.482379, 20159.124764, 21776.522459}, // UA 111
    {20197.562831, 20178.557506, 21775.366372}, // UB 111
    {20155.068093, 20142.372400, 21769.351245}, // UC 111
    {20155.068093, 20142.372400, 21769.351245}, // UX 000

    // ����ͨ�� (IA, IB, IC, IX) - �ֱ��Ӧ 5A, 1A, 0.2A
    {20029.339034, 23237.211085, 23485.098698}, // IA 111
    {20045.211964, 23237.211085, 23355.930655}, // IB 111
    {20026.621825, 23211.482920, 23379.357041}, // IC 111
    {20026.621825, 23211.482920, 23250.247711}  // IX 111
};

/*
 * ��������Ϊ����Ƶ��
 * �Զ�����IP�˵Ĳ�����Ͳ���Ƶ��
 */
void AdcDma_Start_OneBulk(int SamplePoints, int SampleFrequency)
{
    // �ر������ж�
    Xil_ExceptionDisable();

    /*1 ����ADC���������ò��������Ͳ���Ƶ��*/
    if (sample_points == 256)
    {
        Xil_Out32(adc_whole_base_addr + 8, SamplePoints);               // �����㣺sample_pointsд256
        Xil_Out32(adc_whole_base_addr + 4, 99993600 / SampleFrequency); // 7812����Ӧ����Ƶ��50*256
    }

    Xil_Out32(adc_whole_base_addr + 0, 0); // ����һ��ADC
    Xil_Out32(adc_whole_base_addr + 0, 1);

    /*2 ����DMA����*/
    // ͬʱ����DMA����ADC�����DDR rx_buffer_ptr
    // sample_points���� *16λ*8��ͨ��
    sync_dma_buffer((UINTPTR)rx_buffer_ptr, sample_points * 16 * CHANNL_MAX, XAXIDMA_DEVICE_TO_DMA);
    int status = SafeDmaTransfer(&axidma, (UINTPTR)rx_buffer_ptr, sample_points * 16 * CHANNL_MAX, XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS)
    {
        // ����DMA����ʧ�����
        printf("CPU1: ADC DMA transfer failed, will retry next cycle\n");
    }

    // ���������ж�
    Xil_ExceptionEnable();
    // ����������adcS_intr_handler�жϺ���
}
void Adc_Start(int SamplePoints, int SampleFrequency, int SamplingPeriodNumber)
{
    error = 0;
    AdcFinish_Flag = 0;
    ADC_Sampling_ddr = SamplingPeriodNumber;             // Ҫ�������ٸ�����
    AdcDma_Start_OneBulk(SamplePoints, SampleFrequency); // ����ÿ�����ڵĲ��������Ͳ���Ƶ��
    usleep(500000);
    // �������adcS_intr_handler����
}

void dma_dac_init()
{
    // dma_dac
    start_dma_dac();
}

void dds_dac_init()
{

    // ����ʹ�ܡ�Ƶ�ʡ���λ��ͨ��ʹ��
    str_wr_bram(PID_OFF);
}

// ����DMA��������ʹ�ø�ǿ�Ļ���ͬ��
void sync_dma_buffer(UINTPTR addr, size_t size, int direction)
{
    if (direction == XAXIDMA_DMA_TO_DEVICE)
    {
        // CPUд�룬DMA��ȡ
        Xil_DCacheFlushRange(addr, size);
    }
    else
    {
        // ��ˢ�£���ʧЧ
        Xil_DCacheFlushRange(addr, size);
        Xil_DCacheInvalidateRange(addr, size);
    }

    // ����ڴ�����ȷ������˳��
    __asm__ __volatile__("dsb sy" : : : "memory");
}

// DMA RX�жϴ����� adc
void rx_intr_handler(void *callback)
{
    // ���뵽���жϺ����д���DMA�Ѿ������һ�δ���
    u32 irq_status;
    int timeout;
    XAxiDma *axidma_inst = (XAxiDma *)callback;

    irq_status = XAxiDma_IntrGetIrq(axidma_inst, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrAckIrq(axidma_inst, irq_status, XAXIDMA_DEVICE_TO_DMA);

    // Rx����
    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK))
    {
        error = 1;
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

    // Rx���
    if ((irq_status & XAXIDMA_IRQ_IOC_MASK))
    {
        // �û�����
        // ADC�������
        if ((Xil_In32(adc_whole_base_addr + 0) && 0x1) != 1)
        {
            error += 1;
        }
        // ����������һ�ε�ADC
        if (ADC_Sampling_ddr > 1)
        {
            // ������һ�ε�ADC��DMA
            AdcDma_Start_OneBulk(sample_points, sample_points * Wave_Frequency);
        }
        else
        {
            // ˵��AD_SAMP_CYCLE_NUMBERд����һ��
            AdcFinish_Flag = 1; // ��ADc��ɱ�־д1
        }

        sync_dma_buffer((UINTPTR)rx_buffer_ptr, FIFO_DEPTH * 16 * CHANNL_MAX, XAXIDMA_DEVICE_TO_DMA);
        Adc_Data_processing();
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
        usleep(1000); // �����ӳ�
    }

    printf("CPU1: DMA transfer failed after %d retries\n", max_retries);
    return status;
}

// DMA TX�жϴ����� dac
void tx_intr_handler(void *callback)
{
    int timeout;
    u32 irq_status;
    XAxiDma *axidma_inst = (XAxiDma *)callback;

    // ��ȡ��������ж�
    irq_status = XAxiDma_IntrGetIrq(axidma_inst, XAXIDMA_DMA_TO_DEVICE);
    // ȷ�ϴ�������ж�
    XAxiDma_IntrAckIrq(axidma_inst, irq_status, XAXIDMA_DMA_TO_DEVICE);

    // Tx����
    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK))
    {
        error = 1;
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

    // Tx���
    if ((irq_status & XAXIDMA_IRQ_IOC_MASK))
    {
        tx_done = 1;
    }
}

// DAC FIFO�磺����fifo���������������
void underflow_handler()
{
    // xil_printf("underflow\r\n");

    //	Copy_Wave_to_tx_buffer_ptr();
    sync_dma_buffer((UINTPTR)tx_buffer_ptr, DATA_LEN * 16, XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_SimpleTransfer(&axidma, (UINTPTR)tx_buffer_ptr, DATA_LEN * 16, XAXIDMA_DMA_TO_DEVICE);
}

// ����жϺ���
void soft_intr_handler()
{
    xil_printf("CPU1: CPU1 Soft Interrupt\r\n");
}

// ��ʱ���жϴ�����
void timer_intr_handler(void *CallBackRef)
{
    XScuTimer *timer_ptr = (XScuTimer *)CallBackRef;
    Timer_Flag = 1;

    /*1 ��Ϣ����*/
    // ��ȡ��Ϣ����
    char buffer[MAX_DATA_LEN];
    ssize_t bytesRead = MsgQue_read(buffer, sizeof(buffer));
    if (bytesRead > 0)
    {
        // ����JSONָ��
        Parse_JsonCommand(buffer);
    }

    // �����ʱ���жϱ�־
    XScuTimer_ClearInterruptStatus(timer_ptr);
}

// ��ʱ����ʼ������
int timer_init(XScuTimer *timer_ptr)
{
    int status;
    // ˽�ж�ʱ����ʼ��
    XScuTimer_Config *timer_cfg_ptr;
    timer_cfg_ptr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
    if (NULL == timer_cfg_ptr)
        return XST_FAILURE;
    status = XScuTimer_CfgInitialize(timer_ptr, timer_cfg_ptr, timer_cfg_ptr->BaseAddr);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    XScuTimer_LoadTimer(timer_ptr, TIMER_LOAD_VALUE); // ���ؼ�������
    XScuTimer_EnableAutoReload(timer_ptr);            // �����Զ�װ��ģʽ

    return XST_SUCCESS;
}

// ����DMA�ж�ϵͳ
//   @param   int_ins_ptr��ָ��XScuGicʵ����ָ��
//   @param   AxiDmaPtr��ָ��DMA����ʵ����ָ��
//   @param   tx_intr_id��TXͨ���ж�ID
//   @param   rx_intr_id��RXͨ���ж�ID
//   @return���ɹ�����XST_SUCCESS�����򷵻�XST_FAILURE
int setup_intr_system(XScuGic *int_ins_ptr, XAxiDma *axidma_ptr, XScuTimer *timer_ptr,
                      u16 rx_intr_id, u16 tx_intr_id, u16 underflow_id, u16 switch_id, u16 amplifier_id, u16 SoftIntrCpu1_id, u16 Timer_id)
{
    int status;
    XScuGic_Config *intc_config;

    // ��ʼ���жϿ���������
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

    // �������ȼ��ʹ�������
    // ad
    XScuGic_SetPriorityTriggerType(int_ins_ptr, rx_intr_id, 8, 0x3);
    // da
    XScuGic_SetPriorityTriggerType(int_ins_ptr, tx_intr_id, 0xA0, 0x3);
    XScuGic_SetPriorityTriggerType(int_ins_ptr, underflow_id, 0xA0, 0x3);
    // switch amp
    XScuGic_SetPriorityTriggerType(int_ins_ptr, switch_id, 0xA0, 0x3);
    XScuGic_SetPriorityTriggerType(int_ins_ptr, amplifier_id, 0xA0, 0x3); // �ߵ�ƽ��Ч
    // Ϊ��ʱ���ж����ýϸ����ȼ�
    XScuGic_SetPriorityTriggerType(int_ins_ptr, Timer_id, 0x20, 0x3);

    // Ϊ�ж������жϴ�����
    XScuGic_Connect(int_ins_ptr, rx_intr_id, (Xil_InterruptHandler)rx_intr_handler, axidma_ptr);
    XScuGic_Connect(int_ins_ptr, tx_intr_id, (Xil_InterruptHandler)tx_intr_handler, axidma_ptr);
    XScuGic_Connect(int_ins_ptr, underflow_id, (Xil_InterruptHandler)underflow_handler, (void *)1);
    XScuGic_Connect(int_ins_ptr, switch_id, (Xil_ExceptionHandler)Switch_INT_handler, (void *)1);
    XScuGic_Connect(int_ins_ptr, amplifier_id, (Xil_ExceptionHandler)Amplifier_INT_handler, (void *)1);
    XScuGic_Connect(int_ins_ptr, SoftIntrCpu1_id, (Xil_ExceptionHandler)soft_intr_handler, (void *)int_ins_ptr); // CPU1���ж�
    XScuGic_Connect(int_ins_ptr, Timer_id, (Xil_ExceptionHandler)timer_intr_handler, (void *)timer_ptr);         // ��ʱ��

    // ��ʽ�ؽ�ADC��DMA�ж�ӳ�䵽CPU1
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, rx_intr_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, tx_intr_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, underflow_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, switch_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, amplifier_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, SoftIntrCpu1_id);
    XScuGic_InterruptMaptoCpu(int_ins_ptr, CPU1_ID, Timer_id);

    // ͬʱ��CPU0���ӳ�䣨��ֹLinux����
    XScuGic_InterruptUnmapFromCpu(int_ins_ptr, CPU0_ID, rx_intr_id);
    XScuGic_InterruptUnmapFromCpu(int_ins_ptr, CPU0_ID, tx_intr_id);
    XScuGic_InterruptUnmapFromCpu(int_ins_ptr, CPU0_ID, underflow_id);
    XScuGic_InterruptUnmapFromCpu(int_ins_ptr, CPU0_ID, switch_id);
    XScuGic_InterruptUnmapFromCpu(int_ins_ptr, CPU0_ID, amplifier_id);

    // ʹ��
    // ad
    XScuGic_Enable(int_ins_ptr, rx_intr_id);
    // da
    XScuGic_Enable(int_ins_ptr, tx_intr_id);
    XScuGic_Enable(int_ins_ptr, underflow_id);
    // switch amp
    XScuGic_Enable(int_ins_ptr, switch_id);
    XScuGic_Enable(int_ins_ptr, amplifier_id);
    // CPU1���ж�
    XScuGic_Enable(int_ins_ptr, SoftIntrCpu1_id); // CPU1����ж�
    // ��ʱ��
    XScuGic_Enable(int_ins_ptr, Timer_id);
    // ��������Ӳ�����ж�
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 (void *)int_ins_ptr);
    Xil_ExceptionEnable();

    // ʹ���ж�
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA); // DMA
    XAxiDma_IntrEnable(&axidma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    XScuTimer_EnableInterrupt(timer_ptr); // ��ʱ��
    return XST_SUCCESS;
}

int code_to_real(u16 x)
{ // ��16λ����ת����ʵ�ʵ�ѹֵ
    int x1 = 0;
    int x2 = 0;

    if ((x >> 15) == 0)
    { // ���������ѹ�����λΪ0
        x2 = +((int)(x + 1));
        // x2 = +(((float)(x+1))*(20.0f))/(65536.0f);
    }
    else
    {                           // ����Ǹ���ѹ
        x1 = 0xFFFF - x + 0x01; // ��16λ����������ȫ��ȡ������+1
        x2 = -((int)(x1));
        // x2 = -(((float)(x1))*(20.0f))/(65536.0f);
    }
    return x2;
}

bool AdcFinish_Flag = 0; // ADc��ɱ�־�����жϴ�������д1����ѭ���ж�ȡ

/**
 * @brief ����ADC�ɼ�������
 *
 * ���ɼ�����ADC����ͨ��code_to_real����ת������������洢��dma_rx_8�����С�
 * Ȼ�󽫴���������д�빲��DDR�У����ں�������
 *
 * @details
 * 1. ѭ������ÿ�������㣬���ɼ�����ADC����ͨ��code_to_real����ת��Ϊʵ��ֵ�����洢��dma_rx_8�����С�
 * 2. ������������д�빲��DDR�У�ÿ��ͨ��������д�벻ͬ�ĵ�ַ��
 * 3. ����ADC_Sampling_ddr�����������ڿ���д��DDR�Ĵ�����
 * 4. ��ADC_Sampling_ddr����1ʱ���ر�AD���ܡ�
 * 5. ��ADC_Sampling_ddr����0ʱ������ADC_Sampling_ddrΪAD_SAMP_CYCLE_NUMBER��������AdcFinish_Flag��־Ϊ1����ʾADC���ݴ�����ɡ�
 */

void Adc_Data_processing()
{
    /************************** ���ݴ��� *****************************/

    int k = 0;
    // ֱ�Ӵ�rx_buffer_ptr�������ݲ�д�뵱ǰDDR����
    for (int i = 0; i < sample_points; i++)
    {
        // ֱ�Ӵ���д��DDR������dma_rx_8����
        Xil_Out32(Share_addr + 0 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k + 1])); // JUA

        Xil_Out32(Share_addr + 1 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k + 3])); // JUB

        Xil_Out32(Share_addr + 2 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k + 5])); // JUC

        Xil_Out32(Share_addr + 3 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k + 7])); // JUX

        Xil_Out32(Share_addr + 4 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k])); // JIA

        Xil_Out32(Share_addr + 5 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k + 2])); // JIB

        Xil_Out32(Share_addr + 6 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k + 4])); // JIC

        Xil_Out32(Share_addr + 7 * sample_points * 4 + (AD_SAMP_CYCLE_NUMBER - ADC_Sampling_ddr) * CHANNL_MAX * sample_points * 4 + i * 4,
                  (u32)code_to_real(rx_buffer_ptr[k + 6])); // JIX

        k += 8; // �ƶ�����һ������
    }

    ADC_Sampling_ddr -= 1;
}

void changePhase(uint16_t NewData[], int Array_Length, float Phase_Degress)
{
    for (int i = 0; i < Array_Length; i++)
    {
        // ������λƫ�ƺ�����Ҳ�ֵ
        double phase = 2 * M_PI * i / Array_Length;                    // �Ƕ�ֵ
        double shifted_phase = phase + (Phase_Degress * M_PI / 180.0); // �����λƫ��
        double sin_value = sin(shifted_phase);                         // ��������ֵ
        // ������ֵӳ�䵽���ʵķ��ȷ�Χ��0��MAX_AMPLITUDE��
        NewData[i] = (uint16_t)((sin_value + 1.0) * 0.5 * Data_Width);
    }
}

void Write_Wave_to_Wave_NewData()
{
    // ��ά����8*DATA_LEN     Wave_NewData�д洢����8��ͨ����ÿ��ͨ��DATA_LEN���㣬���Ҳ�

    for (int i = 0; i < CHANNL_MAX; i++)
    {
        addHarmonics(Wave_NewData[i], DATA_LEN, Phase_shift[i], numHarmonics[i], harmonics[i], harmonics_phases[i]);
    }

    if (DATA_LEN == 2048)
    {
        // 8*1024�ĳ�8*2048
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
        // 8*1024�ĳ�8*2048
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 1024; j++)
            {
                Wave_NewData[i][j + 1024] = Wave_NewData[i][j];
            }
        }

        // 8*2048�ĳ�8*4096
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
    // Ҫ��һά����tx_buffer_ptr��ֵ��tx_buffer_ptr�д����8��ͨ�����������ݣ����Ҳ���ǰ8λ�ֱ��Ӧ8��ͨ���ĵ�һ���㣬�������ƣ�ÿ��ͨ����DATA_LEN����
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
    u16 frequency_divisor; // ��Ƶϵ�� Ĭ��Ϊ1953
    // �޸�ͨ��ʹ�ܺͷ�Ƶϵ��
    frequency_divisor = 100000000 / Wave_Frequency / DATA_LEN; // ��Ƶϵ��

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
    Xil_DCacheFlushRange((UINTPTR)tx_buffer_ptr, DATA_LEN * 16); // ˢ��Data Cache
    XAxiDma_SimpleTransfer(&axidma, (UINTPTR)tx_buffer_ptr, DATA_LEN * 16, XAXIDMA_DMA_TO_DEVICE);
}

// ������д��BRAM
void str_wr_bram(PID_STATE pid_state)
{
    u32 data_2channl; // 32λ���ݣ����2��ͨ��16λ�Ĳ���
    u16 channel_cnt = 0;

    u16 frequency_divisor; // ��Ƶϵ�� Ĭ��Ϊ1953
    // �޸Ĳ���

    // PID������λ
    double Phase_PID_Increment[CHANNL_MAX] = {0};
    if (pid_state == PID_ON)
    {
        for (int i = 0; i < CHANNL_MAX; i++)
        {
            // ����lineAC.phu��lineAC.phi�洢���Ƕ�Ӧͨ����ʵ����λ
            double actual_value = (i < 4) ? lineAC.phu[i] : lineAC.phi[i - 4]; // ����ͨ��ѡ��ʵ��ֵ

            Phase_PID_Increment[i] = PID_adjust_phase(Phase_shift[i], actual_value, &phase_pid[i]);
        }
    }
    else
    {
        // ��PID���ڹر�ʱ�����phase_pid���ۼ�ֵ
        for (int i = 0; i < CHANNL_MAX; i++)
        {
            Phase_PID_Increment[i] = 0; // ���PID�ۼ�ֵ
            phase_pid[i].integral = 0;
            phase_pid[i].prev_error = 0;
        }
    }

    // �޸Ĳ�������
    for (int i = 0; i < CHANNL_MAX; i++)
    {
        // ��ȡ��ǰͨ������������
        int range_idx;
        if (i < 4)
        { // ��ѹͨ��
            range_idx = get_voltage_index_by_value(setACS.Vals[i].UR);
        }
        else
        { // ����ͨ��
            range_idx = get_current_index_by_value(setACS.Vals[i - 4].IR);
        }

        // Ӧ��PID����ֵ����λУ׼����
        addHarmonics(Wave_NewData[i], DATA_LEN,
                     Phase_shift[i] + Phase_PID_Increment[i] + DA_CorrectPhase_100[i][range_idx],
                     numHarmonics[i], harmonics[i], harmonics_phases[i]);
    }

    // �޸�ͨ��ʹ�ܺͷ�Ƶϵ��
    frequency_divisor = 100000000 / Wave_Frequency / DATA_LEN; // ��Ƶϵ��
    /*
     *  dds_enable     (slv_reg0[0])
     *  dds_freq_div   (slv_reg1[15:0])    //1953
     *  dds_channels   (slv_reg2[7:0])
     */
    Xil_Out32(dac_whole_base_addr + 0, 1); // start_ddsд1,��17λ
    Xil_Out32(dac_whole_base_addr + 4, (u32)frequency_divisor);
    Xil_Out32(dac_whole_base_addr + 8, (u32)enable);

    // д��BRAM
    // ����ַΪ32bit*4 *1024 = 16KB
    for (int j = 0; j < DATA_LEN; j++)
    {
        for (int i = 0; i < 4 * BRAM_DATA_BYTE; i += BRAM_DATA_BYTE)
        { // ������8ͨ���ĵ�һ��������
            data_2channl = (Wave_NewData[channel_cnt + 1][j] << 16) + Wave_NewData[channel_cnt][j];
            XBram_WriteReg(XPAR_BRAM_0_BASEADDR, i + j * 4 * BRAM_DATA_BYTE, data_2channl);
            channel_cnt += 2;
        }
        channel_cnt = 0;
    }
}

/*
 *    ���г��
 */
/**
 * @brief �����������г������
 *
 * �ú�����һ��������������г��������ӣ���������洢��ָ���������С�
 *
 * @param NewData �洢���������
 * @param Array_length NewData����ĳ���
 * @param Base_Phase_Degrees �������ε���λƫ�ƣ��Զ�Ϊ��λ��
 * @param numHarmonics Ҫ��ӵ�г������
 * @param harmonics г����ֵ�����飬harmonics[0]Ϊ2��г��
 * @param harmonics_phases г����λƫ�Ƶ����飨�Զ�Ϊ��λ��
 */
void addHarmonics(uint16_t NewData[], int Array_length, float Base_Phase_Degrees, int numHarmonics, float harmonics[], float harmonics_phases[])
{
    // harmonics[0]Ϊ2��г��
    // ���������е�ÿ��Ԫ��
    for (int i = 0; i < Array_length; i++)
    {
        // ����������ε���λ
        double phase = 2 * M_PI * i / Array_length;                         // �������ε���λ
        // ��ӻ�����λƫ��
        double shifted_phase = phase + (Base_Phase_Degrees * M_PI / 180.0); // ��ӻ�����λƫ��
        // ����������Ҳ���ֵ
        double sum = sin(shifted_phase);                                    // �������Ҳ�

        // ���г��
        // ����г������
        for (int j = 0; j < numHarmonics; j++)
        {
            // ����г����λ
            double harmonic_phase = (j + 2) * phase;                                               // ������������
            // ���г����λƫ��
            double shifted_harmonic_phase = harmonic_phase + harmonics_phases[j] * M_PI / 180.0; // ʹ�û�����г������λƫ��
            // ����г��ֵ
            double harmonic_value = sin(shifted_harmonic_phase);

            // ���г����������Ӧ�ķ�ֵ
            sum += harmonic_value * harmonics[j]; // ���г����������Ӧ�ķ�ֵ
        }

        // ��һ����ת��Ϊ uint16_t ����
        // �Լ�����ĺͽ��й�һ������ת��Ϊ uint16_t ����
        NewData[i] = (uint16_t)((sum / (1.0 + sumHarmonics(harmonics, numHarmonics))) * 32768 + 32767);
    }
}

// ��������������г����ֵ�ܺ�
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
 * @brief ���ݵ�ѹֵ��ȡ��ѹ�ȼ�����
 *
 * ���ݸ����ĵ�ѹֵ�����ض�Ӧ�ĵ�ѹ�ȼ�������
 *
 * @param voltage ��ѹֵ����λΪ���أ�V��
 * @return ���ص�ѹ�ȼ�������
 *         - 0����ʾ��ѹֵ���ڵ���6.0V����Ӧ6.5V��
 *         - 1����ʾ��ѹֵ���ڵ���3.0V��С��6.0V����Ӧ3.25V��
 *         - 2����ʾ��ѹֵС��3.0V����Ӧ1.876V��
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
 * @brief ���ݵ���ֵ��ȡ��ǰ����
 *
 * ���ݸ����ĵ���ֵ��current�������ض�Ӧ������ֵ��
 *
 * @param current ����ֵ������Ϊfloat
 * @return ���ض�Ӧ������ֵ������Ϊint���������ֵ���ڵ���3.0���򷵻�0����ʾ5A����
 *         �������ֵ���ڵ���0.5��С��3.0���򷵻�1����ʾ1A����
 *         �������ֵС��0.5���򷵻�2����ʾ0.2A����
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
