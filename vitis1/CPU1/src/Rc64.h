#ifndef RC64_H
#define RC64_H

#include "xparameters.h"
#include "xiic.h" // Keep AXI IIC header
#include "xil_printf.h"
#include "sleep.h"     // Keep sleep for potential delays if needed, though example polls
#include "xil_types.h" // Include for types like u8, u16

// --- Base Address Definition ---
// Define the base address using the value from xparameters.h
#define IIC_BASE_ADDRESS XPAR_IIC_0_BASEADDR // <-- Use Base Address from parameters

// --- EEPROM Device Definitions ---
#define I2C_DEVICE_ID XPAR_IIC_0_DEVICE_ID // Keep AXI IIC Device ID
#define EEPROM_ADDRESS 0x50                // EEPROM 7-bit Slave Address (Matches your original setting)
#define EEPROM_PAGE_SIZE 32                // EEPROM Page Size (Bytes) - Matches your original setting
// #define EEPROM_WRITE_DELAY 10000            // Optional: Can be removed if using ACK polling exclusively

// --- Array Dimensions (Matching original Rc64.h) ---
#define ROWS 8
#define COLS 3
#define DOUBLE_SIZE sizeof(double)

// --- Calculated Sizes (Matching original Rc64.h) ---
#define ARRAY_ELEMENT_COUNT (ROWS * COLS)
#define ARRAY_BYTES (ARRAY_ELEMENT_COUNT * DOUBLE_SIZE) // 192 bytes

// --- EEPROM Address Mapping (Matching original Rc64.h) ---
// These define the layout within the EEPROM for the calibration arrays
#define EEPROM_ADDR_DA_CORRECT_100 (0)
#define EEPROM_ADDR_DA_CORRECT_20 (EEPROM_ADDR_DA_CORRECT_100 + ARRAY_BYTES)
#define EEPROM_ADDR_DA_CORRECTPHASE_100 (EEPROM_ADDR_DA_CORRECT_20 + ARRAY_BYTES)
#define EEPROM_ADDR_AD_CORRECT (EEPROM_ADDR_DA_CORRECTPHASE_100 + ARRAY_BYTES)
#define EEPROM_TOTAL_CALIB_BYTES (EEPROM_ADDR_AD_CORRECT + ARRAY_BYTES)

// --- Type Definition for EEPROM Internal Address ---
// Your EEPROM likely uses 2-byte internal addressing
typedef u16 AddressType; // <-- Define AddressType as u16

// --- Function Declarations ---
int RC64_Init(void);

// Read/Write all calibration data functions (Prototypes remain the same)
int RC64_ReadCalibData(void);
int RC64_WriteCalibData(void);

// Internal helper functions to read/write a single array
// Use low-level functions based on the example
int EepromWriteData(AddressType Address, u8 *BufferPtr, u16 ByteCount);
int EepromReadData(AddressType Address, u8 *BufferPtr, u16 ByteCount);

#endif /* RC64_H */