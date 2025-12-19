#include "8025IIC.h" // 使用上面定义的头文件名称
#include <stdio.h>   // For sprintf if used for debug, better use xil_printf

// Helper to convert 0-6 (Sun-Sat) week to RX8025 bitmask
static u8 week_to_rx8025_weekmask(u8 week_0_6)
{
    if (week_0_6 > 6)
        week_0_6 = 0; // Default to Sunday if invalid
    return (1 << week_0_6);
}

// Helper to convert RX8025 bitmask to 0-6 (Sun-Sat) week
static u8 rx8025_weekmask_to_week(u8 weekmask)
{
    for (u8 i = 0; i < 7; i++)
    {
        if ((weekmask >> i) & 0x01)
        {
            return i;
        }
    }
    return 0; // Default to Sunday if no bit is set or error
}

// int Rtc8025_Init(u32 BaseAddr)
// {
//     u32 CtrlReg;
//     u32 Divider;
//     volatile u32 Timeout;

//     // xil_printf("Rtc8025_Init: Initializing AXI IIC at 0x%08X\r\n", (unsigned int)BaseAddr);

//     // 1. Disable the controller
//     XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, 0x00);
//     usleep(200);

//     // 2. Reset TX FIFO (Bit 2 of CR). AXI IIC Lite typically doesn't have separate RX FIFO reset in CR.
//     XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, XIIC_CR_TX_FIFO_RESET_MASK);
//     // The TX FIFO reset bit is self-clearing for AXI IIC Lite.
//     usleep(200); // Allow time for reset

//     // 3. Set SCL clock frequency
//     if (S_AXI_ACLK_FREQ_HZ == 0 || I2C_SCL_TARGET_FREQ_HZ == 0)
//     { /* ... error handling ... */
//         return XST_FAILURE;
//     }
//     if (S_AXI_ACLK_FREQ_HZ < (4 * I2C_SCL_TARGET_FREQ_HZ))
//     { /* ... error handling ... */
//         return XST_FAILURE;
//     }
//     Divider = (S_AXI_ACLK_FREQ_HZ / (4 * I2C_SCL_TARGET_FREQ_HZ)) - 1;
//     // xil_printf(" Rtc8025_Init: AXI CLK=%uHz, Target SCL=%uHz, Calculated Divider=0x%X (%u)\r\n",
//     //            (unsigned int)S_AXI_ACLK_FREQ_HZ, (unsigned int)I2C_SCL_TARGET_FREQ_HZ,
//     //            (unsigned int)Divider, (unsigned int)Divider);
//     XIic_WriteReg(BaseAddr, AXI_IIC_TX_CLK_REG_OFFSET, Divider & 0x3FF); // Assuming 0x0120 is correct for your IP

//     // 4. AXI IIC Lite doesn't have a separate IISR. Errors are in SR.
//     //    The NAK bit in SR should be cleared before starting a new transaction
//     //    or by the transaction itself. We'll ensure it's clear by reading SR
//     //    or the start of a new transaction will clear it.
//     //    (No explicit "clear all interrupts" register like IISR for AXI IIC Lite via xiic_l.h)

//     // 5. Enable the controller and set to Master mode.
//     CtrlReg = XIIC_CR_ENABLE_DEVICE_MASK | XIIC_CR_MSMS_MASK; // MSMS (Master) + EN (Enable)
//     XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, CtrlReg);
//     usleep(200);

//     // 6. Verify bus is idle
//     Timeout = IIC_TIMEOUT_COUNT;
//     while (XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
//     {
//         if (Timeout-- == 0)
//         {
//             xil_printf(" Rtc8025_Init: TIMEOUT - Bus busy after initialization! SR=0x%08X, CR=0x%08X\r\n",
//                        (unsigned int)XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET),
//                        (unsigned int)XIic_ReadReg(BaseAddr, XIIC_CR_REG_OFFSET));
//             return XST_FAILURE;
//         }
//         usleep(10);
//     }

//     // xil_printf(" Rtc8025_Init: AXI IIC Initialized. CR=0x%02X, SR=0x%02X\r\n",
//     //            (unsigned int)XIic_ReadReg(BaseAddr, XIIC_CR_REG_OFFSET) & 0xFF,
//     //            (unsigned int)XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET) & 0xFF);
//     return XST_SUCCESS;
// }

int Rtc8025_WriteReg(u32 BaseAddr, u8 RegAddr, u8 Value)
{
    u8 WriteBuffer[2];
    unsigned BytesSent;
    volatile u32 Timeout;
    u32 sr_before, sr_after;

    WriteBuffer[0] = RegAddr;
    WriteBuffer[1] = Value;

    // It's good practice to ensure NAK from previous op is cleared.
    // Reading SR can sometimes clear NAK, or starting a new transaction.
    sr_before = XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET);
    if (sr_before & XIIC_SR_NAK_MASK)
    {
        // xil_printf("Rtc8025_WriteReg: NAK was present before write (SR=0x%X). Attempting to clear by re-read or new op.\r\n", sr_before);
        // For AXI IIC Lite, NAK is often cleared by initiating a new transfer or by a stop.
        // We can try to write to CR to clear it if it's sticky, but usually it's not.
        // Or reset TX FIFO if it was a TX NAK.
        u32 cr_val = XIic_ReadReg(BaseAddr, XIIC_CR_REG_OFFSET);
        XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, cr_val | XIIC_CR_TX_FIFO_RESET_MASK);
        XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, cr_val & ~XIIC_CR_TX_FIFO_RESET_MASK);
    }

    Timeout = IIC_TIMEOUT_COUNT;
    while (XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        { /* ... timeout handling ... */
            return XST_FAILURE;
        }
        usleep(1);
    }

    BytesSent = XIic_Send(BaseAddr, RTC8025_SLAVE_ADDR, WriteBuffer, 2, XIIC_STOP);
    sr_after = XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET); // Read SR immediately after send

    if (BytesSent != 2)
    {
        xil_printf("Rtc8025_WriteReg(0x%02X): Send error, sent %u. Expected 2. SR_after=0x%X\r\n", RegAddr, BytesSent, (unsigned int)sr_after);
        if (sr_after & XIIC_SR_NAK_MASK)
        {
            xil_printf("  NAK received during send.\r\n");
        }
        if (sr_after & XIIC_SR_ARB_LOST_MASK)
        {
            xil_printf("  Arbitration lost during send.\r\n");
        }
        return XST_FAILURE;
    }
    // If BytesSent == 2, it implies ACKs were received for slave addr and data bytes.
    // The NAK bit in SR should NOT be set if the send was successful up to the point XIic_Send relinquishes control.
    // For XIIC_STOP, XIic_Send waits for the STOP to be generated.

    Timeout = IIC_TIMEOUT_COUNT;
    while (XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        { /* ... timeout handling ... */
            return XST_FAILURE;
        }
        usleep(1);
    }

    return XST_SUCCESS;
}

int Rtc8025_ReadReg(u32 BaseAddr, u8 RegAddr, u8 *ValuePtr)
{
    u8 AddressBuffer[1];
    unsigned BytesSent;
    unsigned BytesReceived;
    volatile u32 Timeout;
    u32 sr_after_addr_send, sr_after_recv;

    AddressBuffer[0] = RegAddr;

    // Clear NAK from previous op if any
    sr_after_addr_send = XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET);
    if (sr_after_addr_send & XIIC_SR_NAK_MASK)
    {
        u32 cr_val = XIic_ReadReg(BaseAddr, XIIC_CR_REG_OFFSET);
        XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, cr_val | XIIC_CR_TX_FIFO_RESET_MASK);
        XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, cr_val & ~XIIC_CR_TX_FIFO_RESET_MASK);
    }

    Timeout = IIC_TIMEOUT_COUNT;
    while (XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        { /* ... timeout handling ... */
            return XST_FAILURE;
        }
        usleep(1);
    }

    // Send the register address with a REPEATED START option
    BytesSent = XIic_Send(BaseAddr, RTC8025_SLAVE_ADDR, AddressBuffer, 1, XIIC_REPEATED_START);
    sr_after_addr_send = XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET);

    if (BytesSent != 1)
    {
        xil_printf("Rtc8025_ReadReg(0x%02X): Address Send error, sent %u. SR_after_addr=0x%X\r\n", RegAddr, BytesSent, (unsigned int)sr_after_addr_send);
        if (sr_after_addr_send & XIIC_SR_NAK_MASK)
            xil_printf("  NAK on address send.\r\n");
        if (sr_after_addr_send & XIIC_SR_ARB_LOST_MASK)
            xil_printf("  ARB_LOST on address send.\r\n");
        // Attempt to issue a STOP to free the bus
        XIic_Recv(BaseAddr, RTC8025_SLAVE_ADDR, ValuePtr, 0, XIIC_STOP); // Dummy read of 0 bytes with STOP
        return XST_FAILURE;
    }
    // If BytesSent == 1, slave ACKed its address and the register address.

    // Now receive the data
    BytesReceived = XIic_Recv(BaseAddr, RTC8025_SLAVE_ADDR, ValuePtr, 1, XIIC_STOP);
    sr_after_recv = XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET);

    if (BytesReceived != 1)
    {
        xil_printf("Rtc8025_ReadReg(0x%02X): Receive error, received %u. SR_after_recv=0x%X\r\n", RegAddr, BytesReceived, (unsigned int)sr_after_recv);
        if (sr_after_recv & XIIC_SR_NAK_MASK)
            xil_printf("  NAK during receive (master should NAK last byte, this is unusual for slave NAK).\r\n");
        // XIic_Recv with XIIC_STOP should handle NAKing the last byte from master side.
        // If slave NAKs here, it's an issue.
        return XST_FAILURE;
    }
    // If BytesReceived == 1, data was received.

    Timeout = IIC_TIMEOUT_COUNT;
    while (XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        { /* ... timeout handling ... */
            return XST_FAILURE;
        }
        usleep(1);
    }

    return XST_SUCCESS;
}

int Rtc8025_SetTime(u32 BaseAddr, const RTC_Time_t *TimePtr)
{
    if (!TimePtr)
        return XST_FAILURE;
    int Status;

    // Optional: Could read CTRL2, set STOP bit, then clear it after setting time.
    // For RX8025, direct writes are usually fine.

    Status = Rtc8025_WriteReg(BaseAddr, RTC8025_REG_SEC, DEC_TO_BCD(TimePtr->sec % 60));
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = Rtc8025_WriteReg(BaseAddr, RTC8025_REG_MIN, DEC_TO_BCD(TimePtr->min % 60));
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = Rtc8025_WriteReg(BaseAddr, RTC8025_REG_HOUR, DEC_TO_BCD(TimePtr->hour % 24)); // Assuming 24hr mode
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = Rtc8025_WriteReg(BaseAddr, RTC8025_REG_WEEK, week_to_rx8025_weekmask(TimePtr->week));
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = Rtc8025_WriteReg(BaseAddr, RTC8025_REG_DAY, DEC_TO_BCD(TimePtr->day)); // Add validation for day (1-31)
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = Rtc8025_WriteReg(BaseAddr, RTC8025_REG_MONTH, DEC_TO_BCD(TimePtr->month)); // Add validation for month (1-12)
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = Rtc8025_WriteReg(BaseAddr, RTC8025_REG_YEAR, DEC_TO_BCD(TimePtr->year % 100));
    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    // xil_printf("Rtc8025_SetTime: %02d-%02d-20%02d W:%d %02d:%02d:%02d\r\n",
    //            TimePtr->day, TimePtr->month, TimePtr->year % 100,
    //            TimePtr->week, TimePtr->hour, TimePtr->min, TimePtr->sec);
    return XST_SUCCESS;
}

int Rtc8025_GetTime(u32 BaseAddr, RTC_Time_t *TimePtr)
{
    if (!TimePtr)
        return XST_FAILURE;
    int Status;
    u8 temp;

    Status = Rtc8025_ReadReg(BaseAddr, RTC8025_REG_SEC, &temp);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    TimePtr->sec = BCD_TO_DEC(temp & 0x7F); // Mask VL bit

    Status = Rtc8025_ReadReg(BaseAddr, RTC8025_REG_MIN, &temp);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    TimePtr->min = BCD_TO_DEC(temp);

    Status = Rtc8025_ReadReg(BaseAddr, RTC8025_REG_HOUR, &temp);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    TimePtr->hour = BCD_TO_DEC(temp & 0x3F); // Assuming 24hr mode, mask off top bits

    Status = Rtc8025_ReadReg(BaseAddr, RTC8025_REG_WEEK, &temp);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    TimePtr->week = rx8025_weekmask_to_week(temp);

    Status = Rtc8025_ReadReg(BaseAddr, RTC8025_REG_DAY, &temp);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    TimePtr->day = BCD_TO_DEC(temp);

    Status = Rtc8025_ReadReg(BaseAddr, RTC8025_REG_MONTH, &temp);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    TimePtr->month = BCD_TO_DEC(temp);

    Status = Rtc8025_ReadReg(BaseAddr, RTC8025_REG_YEAR, &temp);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    TimePtr->year = BCD_TO_DEC(temp);

    return XST_SUCCESS;
}