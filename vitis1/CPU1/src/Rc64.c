/**
 * @file Rc64.c
 * @brief RC64 EEPROM ��д����ʵ�� (�Ͳ㼶 AXI IIC)
 *
 * ʹ������˼�Ͳ㼶 AXI IIC ���� (xiic_l.h) �� I2C EEPROM ����ͨ�ţ�
 * ʵ����У׼���ݵĶ�ȡ��д�빦�ܣ��������˳�ʱ�����Ի��������³���ԡ�
 * ������ Zynq 7020 ƽ̨������ʹ�� AXI IIC IP �˵ĳ�����
 */

#include "Rc64.h" // �����豸���塢��ַӳ�䡢���������ͳ�ʱ/���Ժ�

// ע�⣺WriteBuffer �� ReadBuffer ���屣���ڴ˴���Ϊ��̬ȫ�ֱ�����
// ���ϣ�������������ļ��ɼ�������Ҫ�Ƶ� .h �ļ����Ƴ� static��
// ��ͨ����Ϊ�ڲ�ʵ��ϸ�ڷ��� .c �ļ��и����ʡ�

// д����������СΪ EEPROM ҳ��С + ��ַ�ֽ���
static u8 WriteBuffer[EEPROM_PAGE_SIZE + sizeof(AddressType)];
// ������������С���������ɵ�������ȡ�� (������ҳ��Сʾ������ʵ�ʿ��ܶ������)
//static u8 ReadBuffer[EEPROM_PAGE_SIZE]; // ʾ����ʵ�� EepromReadData ��ֱ��д��Ŀ�� BufferPtr

// �ⲿУ׼�������� 
extern double DA_Correct_100[ROWS][COLS];
extern double DA_Correct_20[ROWS][COLS];
extern double DA_CorrectPhase_100[ROWS][COLS];
extern double AD_Correct[ROWS][COLS];

/**
 * @brief ��ʼ�� IIC Ӳ�� (�Ͳ㼶)
 * @return XST_SUCCESS ����ɹ������򷵻� XST_FAILURE
 * @comment ��鲢ʹ�� AXI IIC ����������������Ƿ���в����볬ʱ��
 */
int RC64_Init(void)
{
    u32 CtrlReg;
    volatile u32 Timeout = IIC_TIMEOUT_COUNT; // ʹ��ͷ�ļ��ж���ĳ�ʱ������

    xil_printf("RC64_Init: Initializing AXI IIC using low-level access...\r\n");

    // ���������Ƿ���ʹ�ܣ����δʹ����ʹ��
    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
    if (!(CtrlReg & XIIC_CR_ENABLE_DEVICE_MASK))
    {
        xil_printf("RC64_Init: Enabling AXI IIC Controller.\r\n");
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                      CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
    }

    // ��������Ƿ������Ϊ�������ԣ����볬ʱ
    while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        {
            xil_printf("RC64_Init: Error - Timed out waiting for IIC Bus to be idle.\r\n");
            // ���Ը�λ���Ƿ��ָܻ� (��ѡ�Ļָ�����)
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK); // �����λλ
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);  // ����ʹ��
            return XST_FAILURE;                                                                         // ����ʧ��
        }
        usleep(1); // ������ʱ����CPU��ת
    }

    xil_printf("RC64_Init: AXI IIC hardware assumed ready (low-level).\r\n");
    return XST_SUCCESS;
}

/**
 * @brief �� EEPROM ��ȡ����У׼����
 * @return XST_SUCCESS �����ȡ�ɹ������򷵻� XST_FAILURE
 * @comment ���ε��� EepromReadData ��ȡ����У׼���ݿ顣
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
    // ��ѡ��ȡ�������ע���Դ�ӡ��ȡ�����ݽ�����֤
    /*
    xil_printf("--- Read Calibration Data Dump ---\r\n");
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("DA_Correct_100[%d][%d] = %f\r\n", i, j, DA_Correct_100[i][j]);
            printf("DA_Correct_20[%d][%d] = %f\r\n", i, j, DA_Correct_20[i][j]);
            printf("DA_CorrectPhase_100[%d][%d] = %f\r\n", i, j, DA_CorrectPhase_100[i][j]);
            printf("AD_Correct[%d][%d] = %f\r\n", i, j, AD_Correct[i][j]);
        }
    }
    xil_printf("--- End Calibration Data Dump ---\r\n");
    */
    return XST_SUCCESS;
}

/**
 * @brief ������У׼����д�� EEPROM
 * @return XST_SUCCESS ���д��ɹ������򷵻� XST_FAILURE
 * @comment ���ε��� EepromWriteData д�����У׼���ݿ顣
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
 * @brief �Ͳ㼶������������������д��IIC EEPROM������ҳд�߼�����������ʱ����
 * @param Address EEPROM�е���ʼ��ַ
 * @param BufferPtr ָ��Ҫд�����ݵĻ�����
 * @param ByteCount Ҫд������ֽ���
 * @return XST_SUCCESS ����ɹ���XST_FAILURE ���ʧ��
 * @comment �����ҳд�룬����ÿ��ҳд���ͨ����ѯACK�ȴ�EEPROM����ڲ�д������
 * ���������߷�æ��ACK��ѯ�ĳ�ʱ/���Դ���
 */
int EepromWriteData(AddressType Address, u8 *BufferPtr, u16 ByteCount)
{
    u16 BytesToSend;
    volatile unsigned SentByteCount;
    // ʹ�þ�̬ȫ��д��������������ջ�Ϸ��������
    // u8 LocalWriteBuffer[sizeof(AddressType) + EEPROM_PAGE_SIZE];
    u16 CurrentAddress = Address;
    u16 BytesRemaining = ByteCount;
    u32 CtrlReg;
    volatile unsigned AckByteCount;
    u8 AddressBuffer[sizeof(AddressType)]; // �����ڷ��͵�ַ����ѯACK
    volatile u32 AckPollRetries;           // ����ACK��ѯ�����Լ�����
    volatile u32 Timeout;                  // ͨ�ó�ʱ������

    // ׼��������ѯACK�ĵ�ַ������
    // ע�⣺��������ʼ��ַ��ʼ��������ѭ����ÿ��д��һҳ������
    if (sizeof(AddressType) == 2)
    {
        AddressBuffer[0] = (u8)(CurrentAddress >> 8);
        AddressBuffer[1] = (u8)(CurrentAddress);
    }
    else
    {
        AddressBuffer[0] = (u8)(CurrentAddress);
    }

    while (BytesRemaining > 0)
    {
        // 1. ���㵱ǰҳ����д������ֽ�
        u16 PageOffset = CurrentAddress % EEPROM_PAGE_SIZE;
        u16 BytesLeftInPage = EEPROM_PAGE_SIZE - PageOffset;
        BytesToSend = (BytesRemaining < BytesLeftInPage) ? BytesRemaining : BytesLeftInPage;

        // 2. ׼��������ַ�����ݵ�д�뻺���� (ʹ�þ�̬ȫ�ֻ����� WriteBuffer)
        if (sizeof(AddressType) == 2)
        {
            WriteBuffer[0] = (u8)(CurrentAddress >> 8);
            WriteBuffer[1] = (u8)(CurrentAddress);
        }
        else
        {
            WriteBuffer[0] = (u8)(CurrentAddress);
            // ע�⣺���EEPROMʹ��IIC��ַλ��Ѱַ��λ��ַ��������Ҫ���� EEPROM_ADDRESS
            // ����: EepromIicAddr = EEPROM_ADDRESS | ((CurrentAddress >> 8) & 0x7);
            // ����������ĵ�ַ���� XIic_Send �ĵڶ������������������ʹ�ù̶� EEPROM_ADDRESS��
        }
        memcpy(&WriteBuffer[sizeof(AddressType)], BufferPtr, BytesToSend);

        // 3. ��������ǰ��������Ƿ���� (����ʱ)
        Timeout = IIC_TIMEOUT_COUNT;
        while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
        {
            if (Timeout-- == 0)
            {
                xil_printf("EepromWrite: Error - Timed out waiting for bus idle before sending data (Addr: 0x%X).\r\n", CurrentAddress);
                return XST_FAILURE;
            }
            usleep(1);
        }

        // 4. ���͵�ַ������
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  WriteBuffer, BytesToSend + sizeof(AddressType),
                                  XIIC_STOP); // ���ͺ���� STOP ����

        if (SentByteCount != (BytesToSend + sizeof(AddressType)))
        {
            xil_printf("EepromWrite: Error sending data (Sent %u, Expected %u, Addr: 0x%X)\r\n", SentByteCount, BytesToSend + sizeof(AddressType), CurrentAddress);
            // ���Ը�λ TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK); // �����λλ
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);  // ����ʹ��
            return XST_FAILURE;
        }

        // 5. �ȴ� EEPROM �ڲ�д��� (ͨ����ѯ ACK������������)
        AckPollRetries = EEPROM_WRITE_ACK_POLL_RETRIES;
        do
        {
            // 5a. ��ѯǰ��������Ƿ���� (����ʱ)
            Timeout = IIC_TIMEOUT_COUNT;
            while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
            {
                if (Timeout-- == 0)
                {
                    xil_printf("EepromWrite: Error - Timed out waiting for bus idle before ACK polling (Addr: 0x%X).\r\n", CurrentAddress);
                    return XST_FAILURE;
                }
                usleep(1);
            }

            // 5b. ���Է����豸��ַ��̽�� ACK
            // ע�⣺����ʹ�õ� AddressBuffer �������ǵ�ǰд��ҳ����ʼ��ַ
            AckByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                     AddressBuffer, sizeof(AddressType), // ֻ���͵�ַ����̽��
                                     XIIC_STOP);                         // ͬ������ STOP

            if (AckByteCount != sizeof(AddressType)) // δ�յ� ACK
            {
                if (AckPollRetries-- == 0)
                { // ������Դ���
                    xil_printf("EepromWrite: Error - Timed out waiting for EEPROM ACK after write (Addr: 0x%X, Max retries: %u).\r\n", CurrentAddress, EEPROM_WRITE_ACK_POLL_RETRIES);
                    // ��ʹ��ʱ��Ҳ���Ը�λһ��FIFO
                    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
                    return XST_FAILURE; // ����ʧ��
                }

                // �ȴ�һ��ʱ�������
                usleep(EEPROM_WRITE_ACK_POLL_DELAY_US);

                // ��ACKʧ�ܺ�λTX FIFO��Ϊ��һ�γ�����׼��
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
            }
            // ����յ�ACK (AckByteCount == sizeof(AddressType))��ѭ�����˳�

        } while (AckByteCount != sizeof(AddressType)); // δ�յ� ACK �����ѭ��

        // 6. ����ָ��ͼ�������Ϊ��һҳ��׼��
        BufferPtr += BytesToSend;
        CurrentAddress += BytesToSend;
        BytesRemaining -= BytesToSend;

        // 7. ���������´���ѯACK�ĵ�ַ������ (�������Ҫд��һҳ)
        if (BytesRemaining > 0)
        {
            if (sizeof(AddressType) == 2)
            {
                AddressBuffer[0] = (u8)(CurrentAddress >> 8);
                AddressBuffer[1] = (u8)(CurrentAddress);
            }
            else
            {
                AddressBuffer[0] = (u8)(CurrentAddress);
            }
        }

    } // end while(BytesRemaining > 0)

    return XST_SUCCESS; // ����ҳд��ɹ�
}

/**
 * @brief �Ͳ㼶��������IIC EEPROM��ȡ���ݵ�����������������ʱ����
 * @param Address EEPROM�е���ʼ��ַ
 * @param BufferPtr ָ��洢��ȡ���ݵĻ�����
 * @param ByteCount Ҫ��ȡ���ֽ���
 * @return XST_SUCCESS ����ɹ���XST_FAILURE ���ʧ��
 * @comment ���ȷ���Ҫ��ȡ�� EEPROM �ڲ���ַ��Ȼ��ִ�ж�ȡ������
 * �����Է��͵�ַ�Ͷ�ȡ���������ߵȴ��ĳ�ʱ�����Լ����͵�ַ�����Ի��ơ�
 */
int EepromReadData(AddressType Address, u8 *BufferPtr, u16 ByteCount)
{
    volatile unsigned ReceivedByteCount;
    volatile unsigned SentByteCount;
    u16 StatusReg;
    u32 CtrlReg;
    u8 AddressBuffer[sizeof(AddressType)];
    volatile u32 AddrSendRetries; // ���͵�ַ���Լ�����
    volatile u32 Timeout;         // ͨ�ó�ʱ������

    // 1. ׼��Ҫ���͵� EEPROM �ڲ���ַ
    if (sizeof(AddressType) == 2)
    {
        AddressBuffer[0] = (u8)(Address >> 8);
        AddressBuffer[1] = (u8)(Address);
    }
    else
    {
        AddressBuffer[0] = (u8)(Address);
        // ͬ��ע�� EEPROM ��ַλ�����߼� (�����Ҫ)
    }

    // 2. ���� EEPROM �ڲ���ַ (�����Ժͳ�ʱ)
    AddrSendRetries = EEPROM_ADDR_SEND_RETRIES;
    do
    {
        // 2a. ����ǰ��������Ƿ���� (����ʱ)
        Timeout = IIC_TIMEOUT_COUNT;
        while ((StatusReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET)) & XIIC_SR_BUS_BUSY_MASK)
        {
            if (Timeout-- == 0)
            {
                xil_printf("EepromRead: Error - Timed out waiting for bus idle before sending address (Addr: 0x%X).\r\n", Address);
                return XST_FAILURE;
            }
            usleep(1);
        }

        // 2b. ���Է��͵�ַ
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  AddressBuffer, sizeof(AddressType),
                                  XIIC_STOP); // ���͵�ַ�� STOP

        if (SentByteCount != sizeof(AddressType)) // ����ʧ�� (������ ACK)
        {
            if (AddrSendRetries-- == 0)
            { // ������Դ���
                xil_printf("EepromRead: Error sending address (Sent %u, Expected %u, Addr: 0x%X) - Max retries reached.\r\n", SentByteCount, sizeof(AddressType), Address);
                // ���Ը�λ TX FIFO
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
                return XST_FAILURE; // ����ʧ��
            }

            xil_printf("EepromRead: Error sending address (Sent %u, Addr: 0x%X), retrying...\r\n", SentByteCount, Address);
            // ���Ը�λ TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);

            usleep(EEPROM_ADDR_SEND_DELAY_US); // ������ʱ������
        }
        // ������ͳɹ� (SentByteCount == sizeof(AddressType))��ѭ���˳�

    } while (SentByteCount != sizeof(AddressType));

    // 3. �����úõĵ�ַ��ʼ��ȡ����
    // 3a. ��ȡǰȷ�������ٴο��� (����ʱ)
    Timeout = IIC_TIMEOUT_COUNT;
    while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        {
            xil_printf("EepromRead: Error - Timed out waiting for bus idle before receiving data (Addr: 0x%X).\r\n", Address);
            return XST_FAILURE;
        }
        usleep(1);
    }

    // 3b. ִ�ж�ȡ����
    ReceivedByteCount = XIic_Recv(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  BufferPtr,             // ֱ��д���û��ṩ�Ļ�����
                                  ByteCount, XIIC_STOP); // ��ȡָ���ֽ����� STOP

    if (ReceivedByteCount != ByteCount)
    {
        xil_printf("EepromRead: Error receiving data (Received %u, Expected %u, Addr: 0x%X)\r\n", ReceivedByteCount, ByteCount, Address);
        // ��ȡʧ��ͨ����ζ�Ŵ��豸δ�����㹻���ݻ����ߴ���
        // ���Կ����ڴ˴���� RX FIFO ��λ�߼�����������������
        // CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_RX_FIFO_RESET_MASK);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_RX_FIFO_RESET_MASK);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
        return XST_FAILURE;
    }

    return XST_SUCCESS; // ��ȡ�ɹ�
}
