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
 * @brief 初始化 IIC 硬件 (低层级)
 * @return XST_SUCCESS 如果成功，否则返回XST_FAILURE
 * @comment 基于官方示例，不使用高级驱动实例，仅确保控制器使能
 */
int RC64_Init(void)
{
    u32 CtrlReg;

    xil_printf("RC64_Init: Initializing AXI IIC using low-level access...\r\n");

    // 可选：检查控制器是否已使能，如果未使能则使能
    // 这通常由系统启动代码或上层完成，但可以加一层保险
    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
    if (!(CtrlReg & XIIC_CR_ENABLE_DEVICE_MASK))
    {
        xil_printf("RC64_Init: Enabling AXI IIC Controller.\r\n");
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                      CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
    }

    // 低层级初始化通常不需要 SelfTest 或 Start
    // 检查总线是否空闲作为基本测试
    if (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        xil_printf("RC64_Init: Error - IIC Bus is busy upon initialization.\r\n");
        // 尝试复位 TX FIFO 和控制器看是否能恢复
        CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // 重新使能
        // return XST_FAILURE; // 可以选择失败或继续尝试
    }

    xil_printf("RC64_Init: AXI IIC hardware assumed ready (low-level).\r\n");
    return XST_SUCCESS;
}

/**
 * @brief 从EEPROM读取所有校准数据 (函数逻辑不变)
 * @return XST_SUCCESS 如果读取成功，否则返回XST_FAILURE
 * @comment 调用新的 EepromReadData 辅助函数
 */
int RC64_ReadCalibData(void)
{
    int Status;

    xil_printf("Start reading calibration data from EEPROM via AXI IIC (low-level)...\r\n");

    // 读取 DA_Correct_100 数据
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECT_100, (u8 *)DA_Correct_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read DA_Correct_100 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // 读取 DA_Correct_20 数据
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECT_20, (u8 *)DA_Correct_20, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read DA_Correct_20 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // 读取 DA_CorrectPhase_100 数据
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECTPHASE_100, (u8 *)DA_CorrectPhase_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read DA_CorrectPhase_100 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // 读取 AD_Correct 数据
    Status = EepromReadData(EEPROM_ADDR_AD_CORRECT, (u8 *)AD_Correct, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read AD_Correct data from EEPROM\r\n");
        return XST_FAILURE;
    }

    xil_printf("CPU1: All calibration data read successfully via AXI IIC (low-level)\r\n");

    // // 打印读取的校准数据 (保持不变)
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
 * @brief 将所有校准数据写入EEPROM (函数逻辑不变)
 * @return XST_SUCCESS 如果写入成功，否则返回XST_FAILURE
 * @comment 调用新的 EepromWriteData 辅助函数
 */
int RC64_WriteCalibData(void)
{
    int Status;

    xil_printf("Start writing calibration data to EEPROM via AXI IIC (low-level)...\r\n");

    // 写入 DA_Correct_100 数据
    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECT_100, (u8 *)DA_Correct_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_Correct_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // 写入 DA_Correct_20 数据
    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECT_20, (u8 *)DA_Correct_20, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_Correct_20 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // 写入 DA_CorrectPhase_100 数据
    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECTPHASE_100, (u8 *)DA_CorrectPhase_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_CorrectPhase_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // 写入 AD_Correct 数据
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
 * @brief 低层级函数，将缓冲区数据写入IIC EEPROM，处理页写逻辑
 * @param Address EEPROM中的起始地址
 * @param BufferPtr 指向要写入数据的缓冲区
 * @param ByteCount 要写入的总字节数
 * @return XST_SUCCESS 如果成功，XST_FAILURE 如果失败
 * @comment 模仿官方示例 EepromWriteByte，但处理多页写入和ACK轮询
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
    u8 AddressBuffer[sizeof(AddressType)]; // 仅用于发送地址以轮询ACK

    // 准备地址缓冲区，用于轮询ACK
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
        // 计算当前页可以写入多少字节
        u16 PageOffset = CurrentAddress % EEPROM_PAGE_SIZE;
        u16 BytesLeftInPage = EEPROM_PAGE_SIZE - PageOffset;
        BytesToSend = (BytesRemaining < BytesLeftInPage) ? BytesRemaining : BytesLeftInPage;

        // 准备包含地址和数据的写入缓冲区
        if (sizeof(AddressType) == 2)
        {
            LocalWriteBuffer[0] = (u8)(CurrentAddress >> 8);
            LocalWriteBuffer[1] = (u8)(CurrentAddress);
        }
        else
        {
            LocalWriteBuffer[0] = (u8)(CurrentAddress);
            // 注意：如果EEPROM使用IIC地址位来寻址高位地址，这里需要调整 EEPROM_ADDRESS
            // EepromIicAddr = EEPROM_ADDRESS | ((CurrentAddress >> 8) & 0x7); // 示例调整
        }
        memcpy(&LocalWriteBuffer[sizeof(AddressType)], BufferPtr, BytesToSend);

        // --- 发送数据 ---
        // 官方示例在这里会先尝试发送地址探测ACK，然后才发送数据，我们合并一下
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  LocalWriteBuffer, BytesToSend + sizeof(AddressType),
                                  XIIC_STOP);

        if (SentByteCount != (BytesToSend + sizeof(AddressType)))
        {
            xil_printf("EepromWrite: Error sending data (Sent %d, Expected %d)\r\n", SentByteCount, BytesToSend + sizeof(AddressType));
            // 发送中止，复位 TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // 重新使能
            return XST_FAILURE;
        }

        // --- 等待 EEPROM 内部写完成 (通过轮询 ACK) ---
        do
        {
            // 等待总线空闲 (以防万一)
            while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
                ;

            // 尝试发送地址字节，如果设备忙（写内部 Flash），它不会 ACK
            AckByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                     AddressBuffer, sizeof(AddressType), // 只发送地址来探测ACK
                                     XIIC_STOP);

            if (AckByteCount != sizeof(AddressType))
            {
                // 没有收到ACK，EEPROM 可能仍在写入，短暂延时后重试
                usleep(500); // 等待 500 微秒

                // 可选：检查是否有其他总线错误发生
//                if (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & (XIIC_SR_ARB_LOST_MASK | XIIC_SR_TX_ERROR_MASK))
//                {
//                    xil_printf("EepromWrite: Bus error occurred during ACK polling.\r\n");
//                    // 尝试复位并返回错误
//                    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
//                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
//                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
//                    return XST_FAILURE;
//                }

                // 如果发送中止，可能需要复位 Tx FIFO (虽然官方示例只在数据发送失败时复位)
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // 重新使能
            }
            // 添加一个小的延时避免过于频繁地轮询和可能的活锁
            // usleep(100);

        } while (AckByteCount != sizeof(AddressType)); // 收到 ACK 表示 EEPROM 空闲

        // 更新指针和计数器
        BufferPtr += BytesToSend;
        CurrentAddress += BytesToSend;
        BytesRemaining -= BytesToSend;
    }

    return XST_SUCCESS; // 返回 XST_SUCCESS 而不是字节数
}

/**
 * @brief 低层级函数，从IIC EEPROM读取数据到缓冲区
 * @param Address EEPROM中的起始地址
 * @param BufferPtr 指向存储读取数据的缓冲区
 * @param ByteCount 要读取的字节数
 * @return XST_SUCCESS 如果成功，XST_FAILURE 如果失败
 * @comment 模仿官方示例 EepromReadByte
 */
int EepromReadData(AddressType Address, u8 *BufferPtr, u16 ByteCount)
{
    volatile unsigned ReceivedByteCount;
    volatile unsigned SentByteCount;
    u16 StatusReg;
    u32 CtrlReg;
    u8 AddressBuffer[sizeof(AddressType)];

    // 准备地址缓冲区
    if (sizeof(AddressType) == 2)
    {
        AddressBuffer[0] = (u8)(Address >> 8);
        AddressBuffer[1] = (u8)(Address);
    }
    else
    {
        AddressBuffer[0] = (u8)(Address);
        // 注意：如果EEPROM使用IIC地址位来寻址高位地址，这里需要调整 EEPROM_ADDRESS
        // EepromIicAddr = EEPROM_ADDRESS | ((Address >> 8) & 0x7); // 示例调整
    }

    // --- 设置 EEPROM 内部地址指针 ---
    // 循环尝试发送地址，直到成功（收到ACK）
    do
    {
        // 在发送前检查总线是否空闲，避免冲突
        StatusReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET);
        if (!(StatusReg & XIIC_SR_BUS_BUSY_MASK))
        {
            SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                      AddressBuffer, sizeof(AddressType),
                                      XIIC_STOP); // 发送地址后 STOP

            if (SentByteCount != sizeof(AddressType))
            {
                xil_printf("EepromRead: Error sending address (Sent %d)\r\n", SentByteCount);
                // 发送中止，复位 TX FIFO
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // 重新使能
                // 可以选择在这里返回失败或继续尝试
                // return XST_FAILURE;
                usleep(100); // 短暂延时后重试
            }
        }
        else
        {
            xil_printf("EepromRead: Bus busy before sending address, waiting...\r\n");
            usleep(100);       // 总线忙，稍后重试
            SentByteCount = 0; // 确保循环继续
        }

    } while (SentByteCount != sizeof(AddressType));

    // --- 从设置好的地址开始读取数据 ---
    // 确保总线再次空闲 (上一步发送地址后应该会空闲)
    while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
        ;

    ReceivedByteCount = XIic_Recv(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  BufferPtr, ByteCount, XIIC_STOP);

    if (ReceivedByteCount != ByteCount)
    {
        xil_printf("EepromRead: Error receiving data (Received %d, Expected %d)\r\n", ReceivedByteCount, ByteCount);
        return XST_FAILURE;
    }

    return XST_SUCCESS; // 返回 XST_SUCCESS 而不是字节数
}
