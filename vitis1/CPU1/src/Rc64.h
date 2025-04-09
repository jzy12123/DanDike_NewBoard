#ifndef RC64_H
#define RC64_H

#include "xparameters.h"
#include "xiicps.h"
#include "xil_printf.h"
#include "sleep.h"

// EEPROM设备相关常量定义
#define I2C_DEVICE_ID XPAR_XIICPS_0_DEVICE_ID // IIC控制器设备ID
#define EEPROM_SLAVE_ADDR 0x50                // EEPROM设备地址 (Make sure this matches your hardware)
#define IIC_SCLK_RATE 100000                  // IIC时钟速率 (100kHz)
#define EEPROM_PAGE_SIZE 32                   // EEPROM页大小(字节) - Check your EEPROM datasheet!
#define EEPROM_WRITE_DELAY 10000              // EEPROM写入延时(微秒) - Check datasheet, 5ms to 10ms is common

// --- Array Dimensions (Matching eeprom_test.c) ---
#define ROWS 8
#define COLS 3
#define DOUBLE_SIZE sizeof(double) // Typically 8 bytes

// --- Calculated Sizes (Matching eeprom_test.c) ---
// Total number of double elements in one array
#define ARRAY_ELEMENT_COUNT (ROWS * COLS)
// Total number of bytes occupied by one array
#define ARRAY_BYTES (ARRAY_ELEMENT_COUNT * DOUBLE_SIZE) // Should be 8 * 3 * 8 = 192 bytes

// --- EEPROM Address Mapping (Matching eeprom_test.c) ---
// These addresses MUST match the layout used by the Linux eeprom_test.c application
#define EEPROM_ADDR_DA_CORRECT_100 (0)                                            // Start at address 0
#define EEPROM_ADDR_DA_CORRECT_20 (EEPROM_ADDR_DA_CORRECT_100 + ARRAY_BYTES)      // 0 + 192 = 192 (0xC0)
#define EEPROM_ADDR_DA_CORRECTPHASE_100 (EEPROM_ADDR_DA_CORRECT_20 + ARRAY_BYTES) // 192 + 192 = 384 (0x180)
#define EEPROM_ADDR_AD_CORRECT (EEPROM_ADDR_DA_CORRECTPHASE_100 + ARRAY_BYTES)    // 384 + 192 = 576 (0x240)
#define EEPROM_TOTAL_CALIB_BYTES (EEPROM_ADDR_AD_CORRECT + ARRAY_BYTES)           // 576 + 192 = 768 (0x300)

// Compatibility Definitions (If Rc64.c uses these)
#define CALIB_ARRAY_SIZE ARRAY_ELEMENT_COUNT // Keep CALIB_ARRAY_SIZE for compatibility if needed (represents element count)
#define CALIB_DATA_SIZE ARRAY_BYTES          // Keep CALIB_DATA_SIZE for compatibility if needed (represents size in bytes)

// --- Function Declarations ---
int RC64_Init(void);

// Functions to read/write ALL calibration data based on the defined map
int RC64_ReadCalibData(void);
int RC64_WriteCalibData(void);

// Internal helper functions to read/write a single array (now using ARRAY_ELEMENT_COUNT)
int RC64_ReadArrayFromEEPROM(double *array, int elementCount, u16 eepromAddr);
int RC64_WriteArrayToEEPROM(double *array, int elementCount, u16 eepromAddr);

#endif /* RC64_H */