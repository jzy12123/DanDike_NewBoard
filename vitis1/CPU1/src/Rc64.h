#ifndef RC64_H
#define RC64_H

#include "xparameters.h"
#include "xiicps.h"
#include "xil_printf.h"
#include "sleep.h"
#include "ADDA.h"

// EEPROM�豸��س�������
#define I2C_DEVICE_ID XPAR_XIICPS_0_DEVICE_ID // IIC�������豸ID
#define EEPROM_SLAVE_ADDR 0x50                // EEPROM�豸��ַ
#define IIC_SCLK_RATE 100000                  // IICʱ������
#define EEPROM_PAGE_SIZE 32                   // EEPROMҳ��С(�ֽ�)
#define EEPROM_WRITE_DELAY 10000              // EEPROMд����ʱ(΢��)

// У׼������EEPROM�еĵ�ַӳ��
#define EEPROM_ADDR_DA_CORRECT_100 0x0000      // DA_Correct_100����ʼ��ַ
#define EEPROM_ADDR_DA_CORRECT_20 0x0300       // DA_Correct_20����ʼ��ַ
#define EEPROM_ADDR_DA_CORRECTPHASE_100 0x0600 // DA_CorrectPhase_100����ʼ��ַ
#define EEPROM_ADDR_AD_CORRECT 0x0900          // AD_Correct����ʼ��ַ

// У׼���ݵĳߴ�(˫���ȸ�����ռ8�ֽ�)
#define CALIB_ARRAY_SIZE (8 * 3)                         // ÿ�������Ԫ������
#define DOUBLE_SIZE sizeof(double)                       // ˫���ȸ�������С
#define CALIB_DATA_SIZE (CALIB_ARRAY_SIZE * DOUBLE_SIZE) // ÿ��У׼��������ֽ���

// ��������
int RC64_Init(void);
int RC64_ReadCalibData(void);
int RC64_WriteCalibData(void);

// ��Щ�������ڲ���������
int RC64_ReadArrayFromEEPROM(double *array, int arraySize, u16 eepromAddr);
int RC64_WriteArrayToEEPROM(double *array, int arraySize, u16 eepromAddr);

#endif /* RC64_H */