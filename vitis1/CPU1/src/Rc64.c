#include "Rc64.h"
#include "xiic_l.h" // <-- Include low-level register definitions

// No static XIic instance needed when using low-level functions
// static XIic IicInstance;

// Buffer Definitions (Keep as before)
static u8 WriteBuffer[EEPROM_PAGE_SIZE + sizeof(AddressType)]; // Size depends on AddressType
static u8 ReadBuffer[EEPROM_PAGE_SIZE];                        // Example uses PAGE_SIZE for reads, but we read ARRAY_BYTES

// External Calibration Arrays (Keep as before)
extern double DA_Correct_100[8][3];
extern double DA_Correct_20[8][3];
extern double DA_CorrectPhase_100[8][3];
extern double AD_Correct[8][3];

/**
 * @brief ��ʼ�� IIC Ӳ�� (�Ͳ㼶)
 * @return XST_SUCCESS ����ɹ������򷵻�XST_FAILURE
 * @comment ���ڹٷ�ʾ������ʹ�ø߼�����ʵ������ȷ��������ʹ��
 */
int RC64_Init(void)
{
    u32 CtrlReg;

    xil_printf("RC64_Init: Initializing AXI IIC using low-level access...\r\n");

    // ��ѡ�����������Ƿ���ʹ�ܣ����δʹ����ʹ��
    // ��ͨ����ϵͳ����������ϲ���ɣ������Լ�һ�㱣��
    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
    if (!(CtrlReg & XIIC_CR_ENABLE_DEVICE_MASK))
    {
        xil_printf("RC64_Init: Enabling AXI IIC Controller.\r\n");
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                      CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
    }

    // �Ͳ㼶��ʼ��ͨ������Ҫ SelfTest �� Start
    // ��������Ƿ������Ϊ��������
    if (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        xil_printf("RC64_Init: Error - IIC Bus is busy upon initialization.\r\n");
        // ���Ը�λ TX FIFO �Ϳ��������Ƿ��ָܻ�
        CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // ����ʹ��
        // return XST_FAILURE; // ����ѡ��ʧ�ܻ��������
    }

    xil_printf("RC64_Init: AXI IIC hardware assumed ready (low-level).\r\n");
    return XST_SUCCESS;
}

/**
 * @brief ��EEPROM��ȡ����У׼���� (�����߼�����)
 * @return XST_SUCCESS �����ȡ�ɹ������򷵻�XST_FAILURE
 * @comment �����µ� EepromReadData ��������
 */
int RC64_ReadCalibData(void)
{
    int Status;

    xil_printf("Start reading calibration data from EEPROM via AXI IIC (low-level)...\r\n");

    // ��ȡ DA_Correct_100 ����
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECT_100, (u8 *)DA_Correct_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read DA_Correct_100 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // ��ȡ DA_Correct_20 ����
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECT_20, (u8 *)DA_Correct_20, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read DA_Correct_20 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // ��ȡ DA_CorrectPhase_100 ����
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECTPHASE_100, (u8 *)DA_CorrectPhase_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read DA_CorrectPhase_100 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // ��ȡ AD_Correct ����
    Status = EepromReadData(EEPROM_ADDR_AD_CORRECT, (u8 *)AD_Correct, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read AD_Correct data from EEPROM\r\n");
        return XST_FAILURE;
    }

    xil_printf("CPU1: All calibration data read successfully via AXI IIC (low-level)\r\n");

    // // ��ӡ��ȡ��У׼���� (���ֲ���)
    // xil_printf("--- Read Calibration Data Dump ---\r\n");
    // for (int i = 0; i < ROWS; i++)
    // {
    //     for (int j = 0; j < COLS; j++)
    //     {
    //         printf("DA_Correct_100[%d][%d] = %f\r\n", i, j, DA_Correct_100[i][j]);
    //         printf("DA_Correct_20[%d][%d] = %f\r\n", i, j, DA_Correct_20[i][j]);
    //         printf("DA_CorrectPhase_100[%d][%d] = %f\r\n", i, j, DA_CorrectPhase_100[i][j]);
    //         printf("AD_Correct[%d][%d] = %f\r\n", i, j, AD_Correct[i][j]);
    //     }
    // }
    // xil_printf("--- End Calibration Data Dump ---\r\n");

    return XST_SUCCESS;
}

/**
 * @brief ������У׼����д��EEPROM (�����߼�����)
 * @return XST_SUCCESS ���д��ɹ������򷵻�XST_FAILURE
 * @comment �����µ� EepromWriteData ��������
 */
int RC64_WriteCalibData(void)
{
    int Status;

    xil_printf("Start writing calibration data to EEPROM via AXI IIC (low-level)...\r\n");

    // д�� DA_Correct_100 ����
    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECT_100, (u8 *)DA_Correct_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_Correct_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // д�� DA_Correct_20 ����
    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECT_20, (u8 *)DA_Correct_20, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_Correct_20 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // д�� DA_CorrectPhase_100 ����
    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECTPHASE_100, (u8 *)DA_CorrectPhase_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_CorrectPhase_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // д�� AD_Correct ����
    Status = EepromWriteData(EEPROM_ADDR_AD_CORRECT, (u8 *)AD_Correct, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data AD_Correct to EEPROM\r\n");
        return XST_FAILURE;
    }

    xil_printf("CPU1: All calibration data written successfully via AXI IIC (low-level)\r\n");
    return XST_SUCCESS;
}

/**
 * @brief �Ͳ㼶������������������д��IIC EEPROM������ҳд�߼�
 * @param Address EEPROM�е���ʼ��ַ
 * @param BufferPtr ָ��Ҫд�����ݵĻ�����
 * @param ByteCount Ҫд������ֽ���
 * @return XST_SUCCESS ����ɹ���XST_FAILURE ���ʧ��
 * @comment ģ�¹ٷ�ʾ�� EepromWriteByte���������ҳд���ACK��ѯ
 */
int EepromWriteData(AddressType Address, u8 *BufferPtr, u16 ByteCount)
{
    u16 BytesToSend;
    volatile unsigned SentByteCount;
    u8 LocalWriteBuffer[sizeof(AddressType) + EEPROM_PAGE_SIZE];
    u16 CurrentAddress = Address;
    u16 BytesRemaining = ByteCount;
    u32 CtrlReg;
    volatile unsigned AckByteCount;
    u8 AddressBuffer[sizeof(AddressType)]; // �����ڷ��͵�ַ����ѯACK

    // ׼����ַ��������������ѯACK
    if (sizeof(AddressType) == 2)
    {
        AddressBuffer[0] = (u8)(Address >> 8); // Use initial address for polling
        AddressBuffer[1] = (u8)(Address);
    }
    else
    {
        AddressBuffer[0] = (u8)(Address);
    }

    while (BytesRemaining > 0)
    {
        // ���㵱ǰҳ����д������ֽ�
        u16 PageOffset = CurrentAddress % EEPROM_PAGE_SIZE;
        u16 BytesLeftInPage = EEPROM_PAGE_SIZE - PageOffset;
        BytesToSend = (BytesRemaining < BytesLeftInPage) ? BytesRemaining : BytesLeftInPage;

        // ׼��������ַ�����ݵ�д�뻺����
        if (sizeof(AddressType) == 2)
        {
            LocalWriteBuffer[0] = (u8)(CurrentAddress >> 8);
            LocalWriteBuffer[1] = (u8)(CurrentAddress);
        }
        else
        {
            LocalWriteBuffer[0] = (u8)(CurrentAddress);
            // ע�⣺���EEPROMʹ��IIC��ַλ��Ѱַ��λ��ַ��������Ҫ���� EEPROM_ADDRESS
            // EepromIicAddr = EEPROM_ADDRESS | ((CurrentAddress >> 8) & 0x7); // ʾ������
        }
        memcpy(&LocalWriteBuffer[sizeof(AddressType)], BufferPtr, BytesToSend);

        // --- �������� ---
        // �ٷ�ʾ����������ȳ��Է��͵�ַ̽��ACK��Ȼ��ŷ������ݣ����Ǻϲ�һ��
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  LocalWriteBuffer, BytesToSend + sizeof(AddressType),
                                  XIIC_STOP);

        if (SentByteCount != (BytesToSend + sizeof(AddressType)))
        {
            xil_printf("EepromWrite: Error sending data (Sent %d, Expected %d)\r\n", SentByteCount, BytesToSend + sizeof(AddressType));
            // ������ֹ����λ TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // ����ʹ��
            return XST_FAILURE;
        }

        // --- �ȴ� EEPROM �ڲ�д��� (ͨ����ѯ ACK) ---
        do
        {
            // �ȴ����߿��� (�Է���һ)
            while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
                ;

            // ���Է��͵�ַ�ֽڣ�����豸æ��д�ڲ� Flash���������� ACK
            AckByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                     AddressBuffer, sizeof(AddressType), // ֻ���͵�ַ��̽��ACK
                                     XIIC_STOP);

            if (AckByteCount != sizeof(AddressType))
            {
                // û���յ�ACK��EEPROM ��������д�룬������ʱ������
                usleep(500); // �ȴ� 500 ΢��

                // ��ѡ������Ƿ����������ߴ�����
//                if (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & (XIIC_SR_ARB_LOST_MASK | XIIC_SR_TX_ERROR_MASK))
//                {
//                    xil_printf("EepromWrite: Bus error occurred during ACK polling.\r\n");
//                    // ���Ը�λ�����ش���
//                    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
//                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
//                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
//                    return XST_FAILURE;
//                }

                // ���������ֹ��������Ҫ��λ Tx FIFO (��Ȼ�ٷ�ʾ��ֻ�����ݷ���ʧ��ʱ��λ)
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // ����ʹ��
            }
            // ���һ��С����ʱ�������Ƶ������ѯ�Ϳ��ܵĻ���
            // usleep(100);

        } while (AckByteCount != sizeof(AddressType)); // �յ� ACK ��ʾ EEPROM ����

        // ����ָ��ͼ�����
        BufferPtr += BytesToSend;
        CurrentAddress += BytesToSend;
        BytesRemaining -= BytesToSend;
    }

    return XST_SUCCESS; // ���� XST_SUCCESS �������ֽ���
}

/**
 * @brief �Ͳ㼶��������IIC EEPROM��ȡ���ݵ�������
 * @param Address EEPROM�е���ʼ��ַ
 * @param BufferPtr ָ��洢��ȡ���ݵĻ�����
 * @param ByteCount Ҫ��ȡ���ֽ���
 * @return XST_SUCCESS ����ɹ���XST_FAILURE ���ʧ��
 * @comment ģ�¹ٷ�ʾ�� EepromReadByte
 */
int EepromReadData(AddressType Address, u8 *BufferPtr, u16 ByteCount)
{
    volatile unsigned ReceivedByteCount;
    volatile unsigned SentByteCount;
    u16 StatusReg;
    u32 CtrlReg;
    u8 AddressBuffer[sizeof(AddressType)];

    // ׼����ַ������
    if (sizeof(AddressType) == 2)
    {
        AddressBuffer[0] = (u8)(Address >> 8);
        AddressBuffer[1] = (u8)(Address);
    }
    else
    {
        AddressBuffer[0] = (u8)(Address);
        // ע�⣺���EEPROMʹ��IIC��ַλ��Ѱַ��λ��ַ��������Ҫ���� EEPROM_ADDRESS
        // EepromIicAddr = EEPROM_ADDRESS | ((Address >> 8) & 0x7); // ʾ������
    }

    // --- ���� EEPROM �ڲ���ַָ�� ---
    // ѭ�����Է��͵�ַ��ֱ���ɹ����յ�ACK��
    do
    {
        // �ڷ���ǰ��������Ƿ���У������ͻ
        StatusReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET);
        if (!(StatusReg & XIIC_SR_BUS_BUSY_MASK))
        {
            SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                      AddressBuffer, sizeof(AddressType),
                                      XIIC_STOP); // ���͵�ַ�� STOP

            if (SentByteCount != sizeof(AddressType))
            {
                xil_printf("EepromRead: Error sending address (Sent %d)\r\n", SentByteCount);
                // ������ֹ����λ TX FIFO
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // ����ʹ��
                // ����ѡ�������ﷵ��ʧ�ܻ��������
                // return XST_FAILURE;
                usleep(100); // ������ʱ������
            }
        }
        else
        {
            xil_printf("EepromRead: Bus busy before sending address, waiting...\r\n");
            usleep(100);       // ����æ���Ժ�����
            SentByteCount = 0; // ȷ��ѭ������
        }

    } while (SentByteCount != sizeof(AddressType));

    // --- �����úõĵ�ַ��ʼ��ȡ���� ---
    // ȷ�������ٴο��� (��һ�����͵�ַ��Ӧ�û����)
    while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
        ;

    ReceivedByteCount = XIic_Recv(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  BufferPtr, ByteCount, XIIC_STOP);

    if (ReceivedByteCount != ByteCount)
    {
        xil_printf("EepromRead: Error receiving data (Received %d, Expected %d)\r\n", ReceivedByteCount, ByteCount);
        return XST_FAILURE;
    }

    return XST_SUCCESS; // ���� XST_SUCCESS �������ֽ���
}
