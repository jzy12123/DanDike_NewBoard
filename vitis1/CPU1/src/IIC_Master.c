#include "IIC_Master.h"
#include "8025IIC.h" // 引用其中的时钟频率宏定义
#include "xiic_l.h"
#include "xil_printf.h"
#include "sleep.h"

int IIC_Master_Init(void)
{
    u32 BaseAddr = RtcRc64_IIC_BASEADDR;
    u32 CtrlReg;
    u32 Divider;
    volatile u32 Timeout;

    // xil_printf("IIC_Master_Init: Initializing Shared AXI IIC at 0x%08X\r\n", (unsigned int)BaseAddr);

    // 步骤 1: 在配置前确保控制器是失能的
    XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, 0);
    usleep(200);

    // 步骤 2: 设置I2C SCL时钟频率
    if (S_AXI_ACLK_FREQ_HZ == 0 || I2C_SCL_TARGET_FREQ_HZ == 0)
    {
        xil_printf("IIC_Master_Init: Error - Clock frequencies not defined.\r\n");
        return XST_FAILURE;
    }
    Divider = (S_AXI_ACLK_FREQ_HZ / (4 * I2C_SCL_TARGET_FREQ_HZ)) - 1;
    // xil_printf("IIC_Master_Init: AXI CLK=%uHz, Target SCL=%uHz, Divider=0x%X\r\n",
    //            (unsigned int)S_AXI_ACLK_FREQ_HZ, (unsigned int)I2C_SCL_TARGET_FREQ_HZ, (unsigned int)Divider);
    XIic_WriteReg(BaseAddr, AXI_IIC_TX_CLK_REG_OFFSET, Divider & 0x3FF);

    // 步骤 3: 使能控制器，并设为主模式
    CtrlReg = XIIC_CR_ENABLE_DEVICE_MASK | XIIC_CR_MSMS_MASK;
    XIic_WriteReg(BaseAddr, XIIC_CR_REG_OFFSET, CtrlReg);
    usleep(200);

    // 步骤 4: 验证总线是否空闲
    Timeout = IIC_TIMEOUT_COUNT;
    while (XIic_ReadReg(BaseAddr, XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK)
    {
        if (Timeout-- == 0)
        {
            xil_printf("IIC_Master_Init: TIMEOUT - Bus busy after initialization!\r\n");
            return XST_FAILURE;
        }
        usleep(10);
    }

    // xil_printf("IIC_Master_Init: Shared AXI IIC Initialized Successfully.\r\n");
    return XST_SUCCESS;
}