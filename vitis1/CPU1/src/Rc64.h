#ifndef RC64_H
#define RC64_H

#include "xparameters.h"
#include "xiicps.h"
#include "xil_printf.h"
#include "sleep.h"
#include "ADDA.h"

// EEPROM设备相关常量定义
#define I2C_DEVICE_ID XPAR_XIICPS_0_DEVICE_ID // IIC控制器设备ID
#define EEPROM_SLAVE_ADDR 0x50                // EEPROM设备地址
#define IIC_SCLK_RATE 100000                  // IIC时钟速率
#define EEPROM_PAGE_SIZE 32                   // EEPROM页大小(字节)
#define EEPROM_WRITE_DELAY 10000              // EEPROM写入延时(微秒)

// 校准数据在EEPROM中的地址映射
#define EEPROM_ADDR_DA_CORRECT_100 0x0000      // DA_Correct_100的起始地址
#define EEPROM_ADDR_DA_CORRECT_20 0x0300       // DA_Correct_20的起始地址
#define EEPROM_ADDR_DA_CORRECTPHASE_100 0x0600 // DA_CorrectPhase_100的起始地址
#define EEPROM_ADDR_AD_CORRECT 0x0900          // AD_Correct的起始地址

// 校准数据的尺寸(双精度浮点数占8字节)
#define CALIB_ARRAY_SIZE (8 * 3)                         // 每个数组的元素数量
#define DOUBLE_SIZE sizeof(double)                       // 双精度浮点数大小
#define CALIB_DATA_SIZE (CALIB_ARRAY_SIZE * DOUBLE_SIZE) // 每个校准数组的总字节数

// 函数声明
int RC64_Init(void);
int RC64_ReadCalibData(void);
int RC64_WriteCalibData(void);

// 这些函数是内部辅助函数
int RC64_ReadArrayFromEEPROM(double *array, int arraySize, u16 eepromAddr);
int RC64_WriteArrayToEEPROM(double *array, int arraySize, u16 eepromAddr);

#endif /* RC64_H */