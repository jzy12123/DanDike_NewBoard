/**
 * @file Rc64.c
 * @brief RC64 EEPROM 读写驱动实现 (低层级 AXI IIC)
 *
 * 使用赛灵思低层级 AXI IIC 驱动 (xiic_l.h) 与 I2C EEPROM 进行通信，
 * 实现了校准数据的读取和写入功能，并加入了超时和重试机制以提高鲁棒性。
 * 适用于 Zynq 7020 平台或其他使用 AXI IIC IP 核的场景。
 */

#include "Rc64.h" // 包含设备定义、地址映射、函数声明和超时/重试宏

// 注意：WriteBuffer 和 ReadBuffer 定义保留在此处作为静态全局变量，
// 如果希望它们在其他文件可见，则需要移到 .h 文件并移除 static。
// 但通常作为内部实现细节放在 .c 文件中更合适。

// 写缓冲区，大小为 EEPROM 页大小 + 地址字节数
static u8 WriteBuffer[EEPROM_PAGE_SIZE + sizeof(AddressType)];
// 读缓冲区，大小至少能容纳单次最大读取量 (这里用页大小示例，但实际可能读更大块)
//static u8 ReadBuffer[EEPROM_PAGE_SIZE]; // 示例，实际 EepromReadData 会直接写到目标 BufferPtr

// 外部校准数组声明 
extern double DA_Correct_100[ROWS][COLS];
extern double DA_Correct_20[ROWS][COLS];
extern double DA_CorrectPhase_100[ROWS][COLS];
extern double AD_Correct[ROWS][COLS];

/**
 * @brief 初始化 IIC 硬件 (低层级)
 * @return XST_SUCCESS 如果成功，否则返回 XST_FAILURE
 * @comment 检查并使能 AXI IIC 控制器，检查总线是否空闲并加入超时。
 */
int RC64_Init(void)
{
    u32 CtrlReg;
    volatile u32 Timeout = IIC_TIMEOUT_COUNT; // 使用头文件中定义的超时计数器

    xil_printf("RC64_Init: Initializing AXI IIC using low-level access...\r\n");

    // 检查控制器是否已使能，如果未使能则使能
    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
    if (!(CtrlReg & XIIC_CR_ENABLE_DEVICE_MASK))
    {
        xil_printf("RC64_Init: Enabling AXI IIC Controller.\r\n");
        XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                      CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
    }

    // 检查总线是否空闲作为基本测试，加入超时
    while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        {
            xil_printf("RC64_Init: Error - Timed out waiting for IIC Bus to be idle.\r\n");
            // 尝试复位看是否能恢复 (可选的恢复尝试)
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK); // 清除复位位
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);  // 重新使能
            return XST_FAILURE;                                                                         // 返回失败
        }
        usleep(1); // 短暂延时避免CPU空转
    }

    xil_printf("RC64_Init: AXI IIC hardware assumed ready (low-level).\r\n");
    return XST_SUCCESS;
}

/**
 * @brief 从 EEPROM 读取所有校准数据
 * @return XST_SUCCESS 如果读取成功，否则返回 XST_FAILURE
 * @comment 依次调用 EepromReadData 读取各个校准数据块。
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
    // 可选：取消下面的注释以打印读取的数据进行验证
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
 * @brief 将所有校准数据写入 EEPROM
 * @return XST_SUCCESS 如果写入成功，否则返回 XST_FAILURE
 * @comment 依次调用 EepromWriteData 写入各个校准数据块。
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
 * @brief 低层级函数，将缓冲区数据写入IIC EEPROM，处理页写逻辑，并包含超时处理
 * @param Address EEPROM中的起始地址
 * @param BufferPtr 指向要写入数据的缓冲区
 * @param ByteCount 要写入的总字节数
 * @return XST_SUCCESS 如果成功，XST_FAILURE 如果失败
 * @comment 处理跨页写入，并在每次页写入后通过轮询ACK等待EEPROM完成内部写操作，
 * 包含对总线繁忙和ACK轮询的超时/重试处理。
 */
int EepromWriteData(AddressType Address, u8 *BufferPtr, u16 ByteCount)
{
    u16 BytesToSend;
    volatile unsigned SentByteCount;
    // 使用静态全局写缓冲区，避免在栈上分配大数组
    // u8 LocalWriteBuffer[sizeof(AddressType) + EEPROM_PAGE_SIZE];
    u16 CurrentAddress = Address;
    u16 BytesRemaining = ByteCount;
    u32 CtrlReg;
    volatile unsigned AckByteCount;
    u8 AddressBuffer[sizeof(AddressType)]; // 仅用于发送地址以轮询ACK
    volatile u32 AckPollRetries;           // 用于ACK轮询的重试计数器
    volatile u32 Timeout;                  // 通用超时计数器

    // 准备用于轮询ACK的地址缓冲区
    // 注意：这里用起始地址初始化，但在循环中每次写完一页后会更新
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
        // 1. 计算当前页可以写入多少字节
        u16 PageOffset = CurrentAddress % EEPROM_PAGE_SIZE;
        u16 BytesLeftInPage = EEPROM_PAGE_SIZE - PageOffset;
        BytesToSend = (BytesRemaining < BytesLeftInPage) ? BytesRemaining : BytesLeftInPage;

        // 2. 准备包含地址和数据的写入缓冲区 (使用静态全局缓冲区 WriteBuffer)
        if (sizeof(AddressType) == 2)
        {
            WriteBuffer[0] = (u8)(CurrentAddress >> 8);
            WriteBuffer[1] = (u8)(CurrentAddress);
        }
        else
        {
            WriteBuffer[0] = (u8)(CurrentAddress);
            // 注意：如果EEPROM使用IIC地址位来寻址高位地址，这里需要调整 EEPROM_ADDRESS
            // 例如: EepromIicAddr = EEPROM_ADDRESS | ((CurrentAddress >> 8) & 0x7);
            // 并将调整后的地址用于 XIic_Send 的第二个参数。本代码假设使用固定 EEPROM_ADDRESS。
        }
        memcpy(&WriteBuffer[sizeof(AddressType)], BufferPtr, BytesToSend);

        // 3. 发送数据前检查总线是否空闲 (带超时)
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

        // 4. 发送地址和数据
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  WriteBuffer, BytesToSend + sizeof(AddressType),
                                  XIIC_STOP); // 发送后产生 STOP 条件

        if (SentByteCount != (BytesToSend + sizeof(AddressType)))
        {
            xil_printf("EepromWrite: Error sending data (Sent %u, Expected %u, Addr: 0x%X)\r\n", SentByteCount, BytesToSend + sizeof(AddressType), CurrentAddress);
            // 尝试复位 TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK); // 清除复位位
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);  // 重新使能
            return XST_FAILURE;
        }

        // 5. 等待 EEPROM 内部写完成 (通过轮询 ACK，带重试限制)
        AckPollRetries = EEPROM_WRITE_ACK_POLL_RETRIES;
        do
        {
            // 5a. 轮询前检查总线是否空闲 (带超时)
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

            // 5b. 尝试发送设备地址，探测 ACK
            // 注意：这里使用的 AddressBuffer 包含的是当前写入页的起始地址
            AckByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                     AddressBuffer, sizeof(AddressType), // 只发送地址用于探测
                                     XIIC_STOP);                         // 同样发送 STOP

            if (AckByteCount != sizeof(AddressType)) // 未收到 ACK
            {
                if (AckPollRetries-- == 0)
                { // 检查重试次数
                    xil_printf("EepromWrite: Error - Timed out waiting for EEPROM ACK after write (Addr: 0x%X, Max retries: %u).\r\n", CurrentAddress, EEPROM_WRITE_ACK_POLL_RETRIES);
                    // 即使超时，也尝试复位一下FIFO
                    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
                    return XST_FAILURE; // 返回失败
                }

                // 等待一段时间后重试
                usleep(EEPROM_WRITE_ACK_POLL_DELAY_US);

                // 在ACK失败后复位TX FIFO，为下一次尝试做准备
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
            }
            // 如果收到ACK (AckByteCount == sizeof(AddressType))，循环将退出

        } while (AckByteCount != sizeof(AddressType)); // 未收到 ACK 则继续循环

        // 6. 更新指针和计数器，为下一页做准备
        BufferPtr += BytesToSend;
        CurrentAddress += BytesToSend;
        BytesRemaining -= BytesToSend;

        // 7. 更新用于下次轮询ACK的地址缓冲区 (如果还需要写下一页)
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

    return XST_SUCCESS; // 所有页写入成功
}

/**
 * @brief 低层级函数，从IIC EEPROM读取数据到缓冲区，并包含超时处理
 * @param Address EEPROM中的起始地址
 * @param BufferPtr 指向存储读取数据的缓冲区
 * @param ByteCount 要读取的字节数
 * @return XST_SUCCESS 如果成功，XST_FAILURE 如果失败
 * @comment 首先发送要读取的 EEPROM 内部地址，然后执行读取操作。
 * 包含对发送地址和读取操作中总线等待的超时处理，以及发送地址的重试机制。
 */
int EepromReadData(AddressType Address, u8 *BufferPtr, u16 ByteCount)
{
    volatile unsigned ReceivedByteCount;
    volatile unsigned SentByteCount;
    u16 StatusReg;
    u32 CtrlReg;
    u8 AddressBuffer[sizeof(AddressType)];
    volatile u32 AddrSendRetries; // 发送地址重试计数器
    volatile u32 Timeout;         // 通用超时计数器

    // 1. 准备要发送的 EEPROM 内部地址
    if (sizeof(AddressType) == 2)
    {
        AddressBuffer[0] = (u8)(Address >> 8);
        AddressBuffer[1] = (u8)(Address);
    }
    else
    {
        AddressBuffer[0] = (u8)(Address);
        // 同样注意 EEPROM 地址位调整逻辑 (如果需要)
    }

    // 2. 发送 EEPROM 内部地址 (带重试和超时)
    AddrSendRetries = EEPROM_ADDR_SEND_RETRIES;
    do
    {
        // 2a. 发送前检查总线是否空闲 (带超时)
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

        // 2b. 尝试发送地址
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  AddressBuffer, sizeof(AddressType),
                                  XIIC_STOP); // 发送地址后 STOP

        if (SentByteCount != sizeof(AddressType)) // 发送失败 (可能无 ACK)
        {
            if (AddrSendRetries-- == 0)
            { // 检查重试次数
                xil_printf("EepromRead: Error sending address (Sent %u, Expected %u, Addr: 0x%X) - Max retries reached.\r\n", SentByteCount, sizeof(AddressType), Address);
                // 尝试复位 TX FIFO
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
                return XST_FAILURE; // 返回失败
            }

            xil_printf("EepromRead: Error sending address (Sent %u, Addr: 0x%X), retrying...\r\n", SentByteCount, Address);
            // 尝试复位 TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);

            usleep(EEPROM_ADDR_SEND_DELAY_US); // 短暂延时后重试
        }
        // 如果发送成功 (SentByteCount == sizeof(AddressType))，循环退出

    } while (SentByteCount != sizeof(AddressType));

    // 3. 从设置好的地址开始读取数据
    // 3a. 读取前确保总线再次空闲 (带超时)
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

    // 3b. 执行读取操作
    ReceivedByteCount = XIic_Recv(IIC_BASE_ADDRESS, EEPROM_ADDRESS,
                                  BufferPtr,             // 直接写入用户提供的缓冲区
                                  ByteCount, XIIC_STOP); // 读取指定字节数后 STOP

    if (ReceivedByteCount != ByteCount)
    {
        xil_printf("EepromRead: Error receiving data (Received %u, Expected %u, Addr: 0x%X)\r\n", ReceivedByteCount, ByteCount, Address);
        // 读取失败通常意味着从设备未发送足够数据或总线错误
        // 可以考虑在此处添加 RX FIFO 复位逻辑，如果问题持续存在
        // CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_RX_FIFO_RESET_MASK);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_RX_FIFO_RESET_MASK);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
        return XST_FAILURE;
    }

    return XST_SUCCESS; // 读取成功
}
