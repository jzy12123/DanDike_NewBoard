#ifndef IIC_MASTER_H_
#define IIC_MASTER_H_

#include "xparameters.h" // 用于获取硬件参数，如基地址和设备ID
#include "xil_types.h"   // 定义 u8, u16, u32 等类型
#include "xil_printf.h"  // 用于打印调试信息
#include "xstatus.h"     // 定义 XST_SUCCESS, XST_FAILURE 等状态码
#include "sleep.h"       // 提供 usleep 函数 (用于延时)
#include "xiic_l.h"      // 低层级 AXI IIC 驱动寄存器定义
#include <string.h>      // 提供 memcpy 函数

// 声明共享IIC控制器的基地址
#define RtcRc64_IIC_BASEADDR XPAR_RTC_EEPROM_AXI_IIC_0_BASEADDR

/**
 * @brief 初始化共享的AXI IIC控制器
 * @return XST_SUCCESS 如果成功, 否则返回 XST_FAILURE
 * @comment 此函数对IIC控制器进行一次完整的、健壮的初始化。
 */
int IIC_Master_Init(void);

#endif /* IIC_MASTER_H_ */