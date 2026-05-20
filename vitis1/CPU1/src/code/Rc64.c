/**
 * @file Rc64.c
 * @brief RC64 EEPROM 读写驱动实现 (低层级 AXI IIC)
 *
 * 使用赛灵思低层级 AXI IIC 驱动 (xiic_l.h) 与 I2C EEPROM 进行通信，
 * 实现了校准数据的读取和写入功能，并加入了超时和重试机制以提高鲁棒性。
 * 适用于 Zynq 7020 平台或其他使用 AXI IIC IP 核的场景。
 *
 * CRC 校验机制说明：
 *   - 算法: CRC-16/CCITT (多项式 0x1021，初始值 0xFFFF)
 *   - 写入时: 计算全部校准数据 (4×ARRAY_BYTES) 的 CRC，附加在数据末尾写入
 *   - 读出时: 读取数据后重新计算 CRC，与 EEPROM 存储的 CRC 比对
 *   - 不匹配: 打印错误日志，加载默认校准参数，函数返回 XST_FAILURE
 */

#include "Rc64.h" // 包含设备定义、地址映射、函数声明和超时/重试宏

// 注意：WriteBuffer 和 ReadBuffer 定义保留在此处作为静态全局变量，
// 如果希望它们在其他文件可见，则需要移到 .h 文件并移除 static。
// 但通常作为内部实现细节放在 .c 文件中更合适。

// 写缓冲区，大小为 EEPROM 页大小 + 地址字节数
static u8 WriteBuffer[EEPROM_PAGE_SIZE + sizeof(AddressType)];
// 读缓冲区，大小至少能容纳单次最大读取量 (这里用页大小示例，但实际可能读更大块)
// static u8 ReadBuffer[EEPROM_PAGE_SIZE]; // 示例，实际 EepromReadData
// 会直接写到目标 BufferPtr

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
// int RC64_Init(void)
// {
//     u32 CtrlReg;
//     volatile u32 Timeout = IIC_TIMEOUT_COUNT; // 使用头文件中定义的超时计数器

//     // xil_printf("CPU1: RC64_Init: Initializing AXI IIC using low-level
//     access...\r\n");

//     // 检查控制器是否已使能，如果未使能则使能
//     CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
//     if (!(CtrlReg & XIIC_CR_ENABLE_DEVICE_MASK))
//     {
//         xil_printf("RC64_Init: Enabling AXI IIC Controller.\r\n");
//         XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
//                       CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
//     }

//     // 检查总线是否空闲作为基本测试，加入超时
//     while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) &
//     XIIC_SR_BUS_BUSY_MASK)
//     {
//         if (Timeout-- == 0)
//         {
//             xil_printf("RC64_Init: Error - Timed out waiting for IIC Bus to
//             be idle.\r\n");
//             // 尝试复位看是否能恢复 (可选的恢复尝试)
//             CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
//             XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg |
//             XIIC_CR_TX_FIFO_RESET_MASK); XIic_WriteReg(IIC_BASE_ADDRESS,
//             XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK); //
//             清除复位位 XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
//             CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);  // 重新使能 return
//             XST_FAILURE; // 返回失败
//         }
//         usleep(1); // 短暂延时避免CPU空转
//     }

//     xil_printf("CPU1: RC64_Init: AXI IIC hardware ready.\r\n");
//     return XST_SUCCESS;
// }

/**
 * @brief 计算 CRC-16/CCITT 校验值
 * @param data   指向数据的指针
 * @param length 数据长度（字节）
 * @return 计算得到的 CRC-16 值
 * @comment 多项式 0x1021，初始值 0xFFFF，无反转（适合嵌入式小数据块校验）。
 */
u16 RC64_CalcCRC16(const u8 *data, u32 length)
{
    u16 crc = CALIB_CRC_INIT;
    u32 i, j;
    for (i = 0; i < length; i++)
    {
        crc ^= ((u16)data[i] << 8);
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x8000U)
            {
                crc = (u16)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc = (u16)(crc << 1);
            }
        }
    }
    return crc;
}

/**
 * @brief 从 EEPROM 读取所有校准数据，并进行 CRC-16 完整性校验
 * @return XST_SUCCESS 如果读取且 CRC 校验通过，否则返回 XST_FAILURE
 * @comment 先依次读取四块校准数据，再读取存储的 CRC 并与重新计算的值比对。
 *          若 CRC 不匹配，将回退到默认校准参数并报错。
 */
int RC64_ReadCalibData(void)
{
    int Status;
    u8 crc_buf[2];  // 从 EEPROM 读回的 CRC（大端序）
    u16 stored_crc; // 解析后的存储 CRC
    u16 calc_crc;   // 根据读出数据重新计算的 CRC

    xil_printf(
        "CPU1: Start reading calibration data from EEPROM via AXI IIC...\r\n");

    /* ---- 1. 依次读取四块校准数据 ---- */

    // 读取 DA_Correct_100
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECT_100, (u8 *)DA_Correct_100,
                            ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Read: Failed to read DA_Correct_100 from EEPROM\r\n");
        goto use_default;
    }

    // 读取 DA_Correct_20
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECT_20, (u8 *)DA_Correct_20,
                            ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Read: Failed to read DA_Correct_20 from EEPROM\r\n");
        goto use_default;
    }

    // 读取 DA_CorrectPhase_100
    Status = EepromReadData(EEPROM_ADDR_DA_CORRECTPHASE_100,
                            (u8 *)DA_CorrectPhase_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Read: Failed to read DA_CorrectPhase_100 from EEPROM\r\n");
        goto use_default;
    }

    // 读取 AD_Correct
    Status = EepromReadData(EEPROM_ADDR_AD_CORRECT, (u8 *)AD_Correct, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Read: Failed to read AD_Correct from EEPROM\r\n");
        goto use_default;
    }

    /* ---- 2. 读取 EEPROM 存储的 CRC ---- */
    Status = EepromReadData(EEPROM_ADDR_CALIB_CRC, crc_buf, 2);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Read: Failed to read CRC from EEPROM\r\n");
        goto use_default;
    }
    /* 大端序还原 u16 */
    stored_crc = ((u16)crc_buf[0] << 8) | (u16)crc_buf[1];

    /* ---- 3. 重新计算并比对 CRC ---- */
    /* 对四块数据做连续滚动 CRC（等价于拼接后整体计算，初始值 0xFFFF） */
    {
        u16 crc_tmp = CALIB_CRC_INIT;
        u8 *p;
        u32 i;
        /* 按字节依次累积四块数据 */
        p = (u8 *)DA_Correct_100;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            int j;
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        p = (u8 *)DA_Correct_20;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            int j;
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        p = (u8 *)DA_CorrectPhase_100;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            int j;
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        p = (u8 *)AD_Correct;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            int j;
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        calc_crc = crc_tmp;
    }

    if (calc_crc != stored_crc)
    {
        xil_printf("RC64 Read: CRC mismatch! stored=0x%04X calc=0x%04X\r\n",
                   (unsigned)stored_crc, (unsigned)calc_crc);
        xil_printf("RC64 Read: EEPROM data may be corrupt or uninitialized."
                   " Loading default calibration parameters.\r\n");
        goto use_default;
    }

    xil_printf("CPU1: All calibration data read and CRC verified (0x%04X).\r\n",
               (unsigned)calc_crc);
    return XST_SUCCESS;

use_default:
    /* CRC 校验失败或 I2C 读取失败时，回退到编译期内置的默认参数 */
    xil_printf("RC64 Read: Loading default calibration parameters.\r\n");
    memcpy(DA_Correct_100, DA_CorrectConst_100, sizeof(DA_CorrectConst_100));
    memcpy(DA_Correct_20, DA_CorrectConst_20, sizeof(DA_CorrectConst_20));
    memcpy(DA_CorrectPhase_100, DA_CorrectPhaseConst_100,
           sizeof(DA_CorrectPhaseConst_100));
    memcpy(AD_Correct, ADConst_Correct, sizeof(ADConst_Correct));
    return XST_FAILURE;
}

/**
 * @brief 将所有校准数据写入 EEPROM，并在末尾附加 CRC-16 校验值
 * @return XST_SUCCESS 如果写入成功，否则返回 XST_FAILURE
 * @comment 先写入四块校准数据，再计算全部数据的 CRC 并将 2 字节大端序 CRC 写到
 *          EEPROM_ADDR_CALIB_CRC 处。若任一步失败则返回错误。
 */
int RC64_WriteCalibData(void)
{
    int Status;
    u16 crc;
    u8 crc_buf[2]; /* 大端序存储 CRC */

    xil_printf("CPU1: Start writing calibration data to EEPROM via AXI IIC "
               "(low-level)...\r\n");

    /* ---- 1. 写入四块校准数据 ---- */

    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECT_100, (u8 *)DA_Correct_100,
                             ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Write: Failed to write DA_Correct_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECT_20, (u8 *)DA_Correct_20,
                             ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Write: Failed to write DA_Correct_20 to EEPROM\r\n");
        return XST_FAILURE;
    }

    Status = EepromWriteData(EEPROM_ADDR_DA_CORRECTPHASE_100,
                             (u8 *)DA_CorrectPhase_100, ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Write: Failed to write DA_CorrectPhase_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    Status = EepromWriteData(EEPROM_ADDR_AD_CORRECT, (u8 *)AD_Correct,
                             ARRAY_BYTES);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Write: Failed to write AD_Correct to EEPROM\r\n");
        return XST_FAILURE;
    }

    /* ---- 2. 计算 CRC-16 并写入 ---- */
    /* 对四块数据做连续滚动 CRC（等价于拼接后整体计算） */
    {
        u16 crc_tmp = CALIB_CRC_INIT;
        u8 *p;
        u32 i;
        int j;

        p = (u8 *)DA_Correct_100;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        p = (u8 *)DA_Correct_20;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        p = (u8 *)DA_CorrectPhase_100;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        p = (u8 *)AD_Correct;
        for (i = 0; i < ARRAY_BYTES; i++)
        {
            crc_tmp ^= ((u16)p[i] << 8);
            for (j = 0; j < 8; j++)
            {
                crc_tmp = (crc_tmp & 0x8000U) ? (u16)((crc_tmp << 1) ^ 0x1021U)
                                              : (u16)(crc_tmp << 1);
            }
        }
        crc = crc_tmp;
    }

    /* 大端序写入 2 字节 CRC */
    crc_buf[0] = (u8)(crc >> 8);
    crc_buf[1] = (u8)(crc);
    Status = EepromWriteData(EEPROM_ADDR_CALIB_CRC, crc_buf, 2);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64 Write: Failed to write CRC to EEPROM\r\n");
        return XST_FAILURE;
    }

    xil_printf("CPU1: All calibration data written successfully, CRC=0x%04X\r\n",
               (unsigned)crc);
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
        BytesToSend =
            (BytesRemaining < BytesLeftInPage) ? BytesRemaining : BytesLeftInPage;

        // 2. 准备包含地址和数据的写入缓冲区 (使用静态全局缓冲区 WriteBuffer)
        if (sizeof(AddressType) == 2)
        {
            WriteBuffer[0] = (u8)(CurrentAddress >> 8);
            WriteBuffer[1] = (u8)(CurrentAddress);
        }
        else
        {
            WriteBuffer[0] = (u8)(CurrentAddress);
            // 注意：如果EEPROM使用IIC地址位来寻址高位地址，这里需要调整
            // EEPROM_ADDRESS 例如: EepromIicAddr = EEPROM_ADDRESS | ((CurrentAddress
            // >> 8) & 0x7); 并将调整后的地址用于 XIic_Send
            // 的第二个参数。本代码假设使用固定 EEPROM_ADDRESS。
        }
        memcpy(&WriteBuffer[sizeof(AddressType)], BufferPtr, BytesToSend);

        // 3. 发送数据前检查总线是否空闲 (带超时)
        Timeout = IIC_TIMEOUT_COUNT;
        while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) &
               XIIC_SR_BUS_BUSY_MASK)
        {
            if (Timeout-- == 0)
            {
                xil_printf("EepromWrite: Error - Timed out waiting for bus idle before "
                           "sending data (Addr: 0x%X).\r\n",
                           CurrentAddress);
                return XST_FAILURE;
            }
            usleep(1);
        }

        // 4. 发送地址和数据
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS, WriteBuffer,
                                  BytesToSend + sizeof(AddressType),
                                  XIIC_STOP); // 发送后产生 STOP 条件

        if (SentByteCount != (BytesToSend + sizeof(AddressType)))
        {
            xil_printf("EepromWrite: Error sending data (Sent %u, Expected %u, Addr: "
                       "0x%X)\r\n",
                       SentByteCount, BytesToSend + sizeof(AddressType),
                       CurrentAddress);
            // 尝试复位 TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                          CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                          CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK); // 清除复位位
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                          CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK); // 重新使能
            return XST_FAILURE;
        }

        // 5. 等待 EEPROM 内部写完成 (通过轮询 ACK，带重试限制)
        AckPollRetries = EEPROM_WRITE_ACK_POLL_RETRIES;
        do
        {
            // 5a. 轮询前检查总线是否空闲 (带超时)
            Timeout = IIC_TIMEOUT_COUNT;
            while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) &
                   XIIC_SR_BUS_BUSY_MASK)
            {
                if (Timeout-- == 0)
                {
                    xil_printf("EepromWrite: Error - Timed out waiting for bus idle "
                               "before ACK polling (Addr: 0x%X).\r\n",
                               CurrentAddress);
                    return XST_FAILURE;
                }
                usleep(1);
            }

            // 5b. 尝试发送设备地址，探测 ACK
            // 注意：这里使用的 AddressBuffer 包含的是当前写入页的起始地址
            AckByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS, AddressBuffer,
                                     sizeof(AddressType), // 只发送地址用于探测
                                     XIIC_STOP);          // 同样发送 STOP

            if (AckByteCount != sizeof(AddressType)) // 未收到 ACK
            {
                if (AckPollRetries-- == 0)
                { // 检查重试次数
                    xil_printf("EepromWrite: Error - Timed out waiting for EEPROM ACK "
                               "after write (Addr: 0x%X, Max retries: %u).\r\n",
                               CurrentAddress, EEPROM_WRITE_ACK_POLL_RETRIES);
                    // 即使超时，也尝试复位一下FIFO
                    CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                                  CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                                  CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                    XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                                  CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
                    return XST_FAILURE; // 返回失败
                }

                // 等待一段时间后重试
                usleep(EEPROM_WRITE_ACK_POLL_DELAY_US);

                // 在ACK失败后复位TX FIFO，为下一次尝试做准备
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                              CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                              CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                              CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
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
        while ((StatusReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET)) &
               XIIC_SR_BUS_BUSY_MASK)
        {
            if (Timeout-- == 0)
            {
                xil_printf("EepromRead: Error - Timed out waiting for bus idle before "
                           "sending address (Addr: 0x%X).\r\n",
                           Address);
                return XST_FAILURE;
            }
            usleep(1);
        }

        // 2b. 尝试发送地址
        SentByteCount = XIic_Send(IIC_BASE_ADDRESS, EEPROM_ADDRESS, AddressBuffer,
                                  sizeof(AddressType),
                                  XIIC_STOP); // 发送地址后 STOP

        if (SentByteCount != sizeof(AddressType)) // 发送失败 (可能无 ACK)
        {
            if (AddrSendRetries-- == 0)
            { // 检查重试次数
                xil_printf("EepromRead: Error sending address (Sent %u, Expected %u, "
                           "Addr: 0x%X) - Max retries reached.\r\n",
                           SentByteCount, sizeof(AddressType), Address);
                // 尝试复位 TX FIFO
                CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                              CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                              CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
                XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                              CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);
                return XST_FAILURE; // 返回失败
            }

            xil_printf("EepromRead: Error sending address (Sent %u, Addr: 0x%X), "
                       "retrying...\r\n",
                       SentByteCount, Address);
            // 尝试复位 TX FIFO
            CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                          CtrlReg | XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                          CtrlReg & ~XIIC_CR_TX_FIFO_RESET_MASK);
            XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET,
                          CtrlReg | XIIC_CR_ENABLE_DEVICE_MASK);

            usleep(EEPROM_ADDR_SEND_DELAY_US); // 短暂延时后重试
        }
        // 如果发送成功 (SentByteCount == sizeof(AddressType))，循环退出

    } while (SentByteCount != sizeof(AddressType));

    // 3. 从设置好的地址开始读取数据
    // 3a. 读取前确保总线再次空闲 (带超时)
    Timeout = IIC_TIMEOUT_COUNT;
    while (XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_SR_REG_OFFSET) &
           XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        {
            xil_printf("EepromRead: Error - Timed out waiting for bus idle before "
                       "receiving data (Addr: 0x%X).\r\n",
                       Address);
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
        xil_printf("EepromRead: Error receiving data (Received %u, Expected %u, "
                   "Addr: 0x%X)\r\n",
                   ReceivedByteCount, ByteCount, Address);
        // 读取失败通常意味着从设备未发送足够数据或总线错误
        // 可以考虑在此处添加 RX FIFO 复位逻辑，如果问题持续存在
        // CtrlReg = XIic_ReadReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg |
        // XIIC_CR_RX_FIFO_RESET_MASK); XIic_WriteReg(IIC_BASE_ADDRESS,
        // XIIC_CR_REG_OFFSET, CtrlReg & ~XIIC_CR_RX_FIFO_RESET_MASK);
        // XIic_WriteReg(IIC_BASE_ADDRESS, XIIC_CR_REG_OFFSET, CtrlReg |
        // XIIC_CR_ENABLE_DEVICE_MASK);
        return XST_FAILURE;
    }

    return XST_SUCCESS; // 读取成功
}
