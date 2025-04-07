#include "Rc64.h"

// IIC控制器实例
static XIicPs IicInstance;

// 读写缓冲区
static u8 WriteBuffer[EEPROM_PAGE_SIZE + 2]; // 额外2字节用于EEPROM地址
static u8 ReadBuffer[EEPROM_PAGE_SIZE];

// 外部校准参数数组
extern double DA_Correct_100[8][3];
extern double DA_Correct_20[8][3];
extern double DA_CorrectPhase_100[8][3];
extern double AD_Correct[8][3];

/**
 * @brief 初始化IIC控制器
 *
 * @return XST_SUCCESS 如果初始化成功，否则返回XST_FAILURE
 */
int RC64_Init(void)
{
    XIicPs_Config *ConfigPtr;
    int Status;

    // 查找IIC控制器配置
    ConfigPtr = XIicPs_LookupConfig(I2C_DEVICE_ID);
    if (ConfigPtr == NULL)
    {
        xil_printf("RC64_Init: iic device configuration could not be found\r\n");
        return XST_FAILURE;
    }

    // 初始化IIC控制器
    Status = XIicPs_CfgInitialize(&IicInstance, ConfigPtr, ConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS)
    {
        xil_printf("RC64_Init: IIC controller initialization failed\r\n");
        return XST_FAILURE;
    }

    // 设置IIC时钟速率
    XIicPs_SetSClk(&IicInstance, IIC_SCLK_RATE);

    xil_printf("RC64_Init: IIC controller initialization succeeded\r\n");
    return XST_SUCCESS;
}

/**
 * @brief 从EEPROM读取所有校准数据
 *
 * @return XST_SUCCESS 如果读取成功，否则返回XST_FAILURE
 */
int RC64_ReadCalibData(void)
{
    int Status;

    xil_printf("Start reading calibration data from eeprom...\r\n");

    // 读取DA_Correct_100数据
    Status = RC64_ReadArrayFromEEPROM((double *)DA_Correct_100, CALIB_ARRAY_SIZE, EEPROM_ADDR_DA_CORRECT_100);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read Da_correct_100 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // 读取DA_Correct_20数据
    Status = RC64_ReadArrayFromEEPROM((double *)DA_Correct_20, CALIB_ARRAY_SIZE, EEPROM_ADDR_DA_CORRECT_20);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read Da_correct_20 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // 读取DA_CorrectPhase_100数据
    Status = RC64_ReadArrayFromEEPROM((double *)DA_CorrectPhase_100, CALIB_ARRAY_SIZE, EEPROM_ADDR_DA_CORRECTPHASE_100);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read DA_CorrectPhase_100 data from EEPROM\r\n");
        return XST_FAILURE;
    }

    // 读取AD_Correct数据
    Status = RC64_ReadArrayFromEEPROM((double *)AD_Correct, CALIB_ARRAY_SIZE, EEPROM_ADDR_AD_CORRECT);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to read AD_Correct data from EEPROM\r\n");
        return XST_FAILURE;
    }

    xil_printf("CPU1:All calibration data has been read\r\n");

    // 打印读取的校准数据 全部打印
    for (int i = 0; i < CALIB_ARRAY_SIZE / sizeof(double); i++)
    {
        printf("DA_Correct_100[%d][0] = %f\r\n", i, DA_Correct_100[i][0]);
        printf("DA_Correct_20[%d][0] = %f\r\n", i, DA_Correct_20[i][0]);
        printf("DA_CorrectPhase_100[%d][0] = %f\r\n", i, DA_CorrectPhase_100[i][0]);
        printf("AD_Correct[%d][0] = %f\r\n", i, AD_Correct[i][0]);
    }

    return XST_SUCCESS;
}

/**
 * @brief 将所有校准数据写入EEPROM
 *
 * @return XST_SUCCESS 如果写入成功，否则返回XST_FAILURE
 */
int RC64_WriteCalibData(void)
{
    int Status;

    xil_printf("Start writing calibration data...\r\n");

    // 写入DA_Correct_100数据
    Status = RC64_WriteArrayToEEPROM((double *)DA_Correct_100, CALIB_ARRAY_SIZE, EEPROM_ADDR_DA_CORRECT_100);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_Correct_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // 写入DA_Correct_20数据
    Status = RC64_WriteArrayToEEPROM((double *)DA_Correct_20, CALIB_ARRAY_SIZE, EEPROM_ADDR_DA_CORRECT_20);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_Correct_20 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // 写入DA_CorrectPhase_100数据
    Status = RC64_WriteArrayToEEPROM((double *)DA_CorrectPhase_100, CALIB_ARRAY_SIZE, EEPROM_ADDR_DA_CORRECTPHASE_100);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data DA_CorrectPhase_100 to EEPROM\r\n");
        return XST_FAILURE;
    }

    // 写入AD_Correct数据
    Status = RC64_WriteArrayToEEPROM((double *)AD_Correct, CALIB_ARRAY_SIZE, EEPROM_ADDR_AD_CORRECT);
    if (Status != XST_SUCCESS)
    {
        xil_printf("Failed to write data AD_Correct to EEPROM\r\n");
        return XST_FAILURE;
    }

    xil_printf("CPU1: All calibration data has been written EEPROM\r\n");
    return XST_SUCCESS;
}

/**
 * @brief 从EEPROM读取数组数据
 *
 * @param array 目标数组指针
 * @param arraySize 数组元素数量
 * @param eepromAddr EEPROM起始地址
 * @return XST_SUCCESS 如果读取成功，否则返回XST_FAILURE
 */
int RC64_ReadArrayFromEEPROM(double *array, int arraySize, u16 eepromAddr)
{
    int Status;
    int i, j;
    u8 *byteArray = (u8 *)array;
    int totalBytes = arraySize * DOUBLE_SIZE;
    int bytesRead = 0;

    while (bytesRead < totalBytes)
    {
        int bytesToRead = (totalBytes - bytesRead) > EEPROM_PAGE_SIZE ? EEPROM_PAGE_SIZE : (totalBytes - bytesRead);

        // 设置EEPROM地址
        WriteBuffer[0] = (u8)((eepromAddr + bytesRead) >> 8);   // 高字节
        WriteBuffer[1] = (u8)((eepromAddr + bytesRead) & 0xFF); // 低字节

        // 发送EEPROM地址
        Status = XIicPs_MasterSendPolled(&IicInstance, WriteBuffer, 2, EEPROM_SLAVE_ADDR);
        if (Status != XST_SUCCESS)
        {
            xil_printf("发送EEPROM地址失败\r\n");
            return XST_FAILURE;
        }

        // 等待总线空闲
        while (XIicPs_BusIsBusy(&IicInstance))
            ;

        // 从EEPROM接收数据
        Status = XIicPs_MasterRecvPolled(&IicInstance, ReadBuffer, bytesToRead, EEPROM_SLAVE_ADDR);
        if (Status != XST_SUCCESS)
        {
            xil_printf("从EEPROM接收数据失败\r\n");
            return XST_FAILURE;
        }

        // 等待总线空闲
        while (XIicPs_BusIsBusy(&IicInstance))
            ;

        // 复制数据到目标数组
        for (i = 0; i < bytesToRead; i++)
        {
            byteArray[bytesRead + i] = ReadBuffer[i];
        }

        bytesRead += bytesToRead;
    }

    return XST_SUCCESS;
}

/**
 * @brief 将数组数据写入EEPROM
 *
 * @param array 源数组指针
 * @param arraySize 数组元素数量
 * @param eepromAddr EEPROM起始地址
 * @return XST_SUCCESS 如果写入成功，否则返回XST_FAILURE
 */
int RC64_WriteArrayToEEPROM(double *array, int arraySize, u16 eepromAddr)
{
    int Status;
    int i, j;
    u8 *byteArray = (u8 *)array;
    int totalBytes = arraySize * DOUBLE_SIZE;
    int bytesWritten = 0;

    while (bytesWritten < totalBytes)
    {
        // 计算当前页内可写入的字节数
        // 由于EEPROM要求在同一页内写入，所以需要确保不跨页写入
        u16 currentPage = (eepromAddr + bytesWritten) / EEPROM_PAGE_SIZE;
        u16 nextPageAddr = (currentPage + 1) * EEPROM_PAGE_SIZE;
        int remainingInPage = nextPageAddr - (eepromAddr + bytesWritten);

        int bytesToWrite = (totalBytes - bytesWritten) > remainingInPage ? remainingInPage : (totalBytes - bytesWritten);

        if (bytesToWrite > (EEPROM_PAGE_SIZE - 2))
        {
            bytesToWrite = EEPROM_PAGE_SIZE - 2; // 保留两字节用于EEPROM地址
        }

        // 设置EEPROM地址
        WriteBuffer[0] = (u8)((eepromAddr + bytesWritten) >> 8);   // 高字节
        WriteBuffer[1] = (u8)((eepromAddr + bytesWritten) & 0xFF); // 低字节

        // 复制数据到写缓冲区
        for (i = 0; i < bytesToWrite; i++)
        {
            WriteBuffer[i + 2] = byteArray[bytesWritten + i];
        }

        // 发送数据到EEPROM
        Status = XIicPs_MasterSendPolled(&IicInstance, WriteBuffer, bytesToWrite + 2, EEPROM_SLAVE_ADDR);
        if (Status != XST_SUCCESS)
        {
            xil_printf("向EEPROM发送数据失败\r\n");
            return XST_FAILURE;
        }

        // 等待总线空闲
        while (XIicPs_BusIsBusy(&IicInstance))
            ;

        // EEPROM写入需要时间，等待写入完成
        usleep(EEPROM_WRITE_DELAY);

        bytesWritten += bytesToWrite;
    }

    return XST_SUCCESS;
}
