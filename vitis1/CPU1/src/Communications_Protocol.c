/*
 * 此文件里的函数用来解析linux发出的JSON指令
 * 同时定义并上报UDP结构体
 * 【结构体的组成】
    (
        帧头区：（16字节）
            同步头：			4字节		固定为：D1D2D3D4
            数据区总长度1：		4字节（值为n）
            数据区总长度2：		4字节（值为n，重复）
            版本信息： 			   1字节
            保留：				       3字节

        数据区：（n字节）	具体结构由多个结构体链式组成。
            结构体类型标识：	4字节
            结构体内容长度：	4字节
            结构体内容：		n1字节

            结构体类型标识：	4字节
            结构体内容长度：	4字节
            结构体内容：		n2字节
            ...

        帧尾区：（4字节）
            结束符：			4字节		固定为：E1E2E3E4
    )
 *
 */
#include "Communications_Protocol.h"

/*
*版本信息
*/
const char FPGA_Ver_Full[] = "[Ver]=V1.250418.1106";
const char ARM_Ver_Full[] = "[Ver]=V1.250515.1934";



void extractContentBetweenPipes(char *buffer)
{
    int len = strlen(buffer);
    int start = -1, end = -1;

    // 遍历字符串，找到第一个和最后一个 '|' 的位置
    for (int i = 0; i < len; i++)
    {
        if (buffer[i] == '|')
        {
            if (start == -1)
            {
                start = i; // 找到第一个 '|'
            }
            else
            {
                end = i; // 更新最后一个 '|' 的位置
            }
        }
    }

    // 如果找到了两个 '|'，并且它们不在同一个位置
    if (start != -1 && end != -1 && start != end)
    {
        // 计算中间内容的长度
        int contentLength = end - start - 1;

        // 将中间内容移动到 buffer 的开头
        memmove(buffer, buffer + start + 1, contentLength);

        // 在末尾添加字符串结束符
        buffer[contentLength] = '\0';

        // printf("CPU1: Remove | \n");
    }
    else
    {
        printf("CPU1: NotFound | \n");
    }
}

/*
 * 解析JSON指令顶层函数
 */
int Parse_JsonCommand(char *buffer)
{
    FunCodeMap funCodeMap[] = {
        {"GetFunCodeList", handle_GetFunCodeList},
        {"GetDevBaseInfo", handle_GetDevBaseInfo},
        {"GetDevState", handle_GetDevState},
        {"SetReportEnable", handle_SetReportEnable},
        {"SetDCS", handle_SetDCS},
        {"SetDCM", handle_SetDCM},
        {"SetACS", handle_SetACS},
        {"SetACM", handle_SetACM},
        {"SetHarm", handle_SetHarm},
        {"SetInterHarm", handle_SetInterHarm},
        {"SetDO", handle_SetDO},
        {"StopDCS", handle_StopDCS},
        {"StopAC", handle_StopAC},
        {"ClearHarm", handle_ClearHarm},
        {"ClearInterHarm", handle_ClearInterHarm},
        {"SetCalibrateAC", handle_SetCalibrateAC},
        {"WriteCalibrateAC", handle_WriteCalibrateAC}};
    const int funCodeMapSize = sizeof(funCodeMap) / sizeof(funCodeMap[0]);

    // 检查输入字符串首尾的 '|'
    extractContentBetweenPipes(buffer);

    // 打印buffer里的数据
    // printf("CPU1: Recived_JSON: %s\n", buffer);

    // 使用cJSON解析JSON字符串
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        xil_printf("Failed to parse JSON.\n");
        return 1;
    }

    cJSON *funCodeItem = cJSON_GetObjectItem(json, "FunCode");
    if (funCodeItem == NULL)
    {
        xil_printf("No FunCode found in JSON.\n");
        cJSON_Delete(json);
        return 1;
    }

    const char *funCode = cJSON_GetStringValue(funCodeItem);
    xil_printf("CPU1: FunCode: %s\r\n", funCode);
    xil_printf("\r\n");

    // 解析Data
    cJSON *data = cJSON_GetObjectItem(json, "Data");

    // 根据 FunCode 跳转到对应的处理函数
    for (int i = 0; i < funCodeMapSize; i++)
    {
        if (strcmp(funCode, funCodeMap[i].funCode) == 0)
        {
            funCodeMap[i].handler(data);
            break;
        }
    }

    cJSON_Delete(json);

    return 0;
}

/**
 * @brief 处理 GetFunCodeList 请求
 *
 * 该函数用于处理 GetFunCodeList 请求，并返回包含支持的指令列表、报告列表和任务事件列表的 JSON 响应。
 *
 * @param data 指向包含请求数据的 cJSON 对象的指针
 */
void handle_GetFunCodeList(cJSON *data)
{
    // 处理 GetFunCodeList 的逻辑
    //    xil_printf("Handling handle_GetFunCodeList...\n");

    // 定义 FunCode 列表
    const char *CmdList[] = {
        "GetFunCodeList", "GetDevBaseInfo", "GetDevState", "SetReportEnable",
        "SetDCS", "SetDCM", "SetACS", "SetACM", "SetHarm", "SetInterHarm", "SetDO",
        "StopDCS", "StopACS", "ClearHarm", "ClearInterHarm"};

    const char *ReportList[] = {
        "ProtectEvent", "DISOE", "BaseDataAC", "BaseDataDC", "BaseDataIO", "HarmData", "InterHarmData"};

    const char *TaskEventList[] = {
        "SetSyncMode", "SetProgress"};

    // 创建上行指令的 JSON 对象
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "GetFunCodeList");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *dataObj = cJSON_CreateObject();

    cJSON *cmdList = cJSON_CreateStringArray(CmdList, sizeof(CmdList) / sizeof(CmdList[0]));
    cJSON_AddItemToObject(dataObj, "CmdList", cmdList);

    cJSON *reportList = cJSON_CreateStringArray(ReportList, sizeof(ReportList) / sizeof(ReportList[0]));
    cJSON_AddItemToObject(dataObj, "ReportList", reportList);

    cJSON *taskEventList = cJSON_CreateStringArray(TaskEventList, sizeof(TaskEventList) / sizeof(TaskEventList[0]));
    cJSON_AddItemToObject(dataObj, "TaskEventList", taskEventList);

    cJSON_AddItemToObject(reply, "Data", dataObj);

    // 将生成的 JSON 转换为字符串
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(reply);
        return;
    }

    // 在字符串首尾加上 '|' 字符
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 为了首尾 '|' 和结束符
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // 将生成的 JSON 写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // 清理
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}


// 辅助函数，用于提取版本号
const char *get_version_string(const char *full_version_str)
{
    const char *prefix = "[Ver]=";
    const char *version_ptr = strstr(full_version_str, prefix); // 查找前缀
    if (version_ptr != NULL)
    {
        return version_ptr + strlen(prefix); // 返回前缀之后的部分
    }
    return full_version_str; // 如果找不到前缀，返回原始字符串 (或可以返回 NULL 或错误提示)
}

void handle_GetDevBaseInfo(cJSON *data)
{
    // 处理 GetDevBaseInfo 的逻辑
    //    xil_printf("Handling handle_GetDevBaseInfo...\n");

    // 提取版本号
    const char *fpga_version = get_version_string(FPGA_Ver_Full);
    const char *arm_version = get_version_string(ARM_Ver_Full);

    // 创建上行指令的 JSON 对象
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "GetDevBaseInfo");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *dataObj = cJSON_CreateObject();
    cJSON_AddStringToObject(dataObj, "Model", "xxx");
    cJSON_AddStringToObject(dataObj, "InnerModel", "DK-34B1");
    // 使用提取出的版本号字符串
    cJSON_AddStringToObject(dataObj, "FPGA_Ver", fpga_version);
    cJSON_AddStringToObject(dataObj, "ARM_Ver", arm_version);

    const char *syncModes[] = {"GPS", "BD", "IRIG-B", "SNTP", "Manual"};
    cJSON *syncMode = cJSON_CreateStringArray(syncModes, 5);
    cJSON_AddItemToObject(dataObj, "SyncMode", syncMode);

    // DI 信息
    cJSON *di = cJSON_CreateObject();
    cJSON_AddNumberToObject(di, "ChnCount", ChnsBI);
    double diResolution[] = {0.1, 1, 10}; // 开入分辨率
    cJSON *diResolutions = cJSON_CreateDoubleArray(diResolution, 3);
    cJSON_AddItemToObject(di, "DI_Resolution", diResolutions);
    cJSON_AddItemToObject(dataObj, "DI", di);

    // DO 信息
    cJSON *doInfo = cJSON_CreateObject();
    cJSON_AddNumberToObject(doInfo, "ChnCount", ChnsBO);
    cJSON_AddItemToObject(dataObj, "DO", doInfo);

    // AC 信息
    cJSON *ac = cJSON_CreateObject();
    const char *acModes[] = {"S", "M"};
    cJSON *acMode = cJSON_CreateStringArray(acModes, 2);
    cJSON_AddItemToObject(ac, "Mode", acMode);
    cJSON_AddNumberToObject(ac, "MaxHarmNumber", HarmNumberMax);
    cJSON_AddNumberToObject(ac, "LineCount", LinesAC);
    cJSON_AddNumberToObject(ac, "ChnCount", ChnsAC);

    cJSON *acChns = cJSON_CreateArray();
    for (int line = 1; line <= LinesAC; line++)
    {
        for (int chn = 1; chn <= ChnsAC; chn++)
        {
            cJSON *chnObj = cJSON_CreateObject();
            cJSON_AddNumberToObject(chnObj, "Line", line);
            cJSON_AddNumberToObject(chnObj, "Chn", chn);

            double urValues[] = {6.5, 3.25, 1.876};
            cJSON *urArray = cJSON_CreateDoubleArray(urValues, 3);
            cJSON_AddItemToObject(chnObj, "URs", urArray);

            double irValues[] = {5, 1, 0.2};
            cJSON *irArray = cJSON_CreateDoubleArray(irValues, 3);
            cJSON_AddItemToObject(chnObj, "IRs", irArray);

            cJSON_AddItemToArray(acChns, chnObj);
        }
    }
    cJSON_AddItemToObject(ac, "Chns", acChns);
    cJSON_AddItemToObject(dataObj, "AC", ac);

    // DCS 信息
    cJSON *dcs = cJSON_CreateObject();
    cJSON_AddNumberToObject(dcs, "ChnCount", ChnsDCS);

    cJSON *dcsChns = cJSON_CreateArray();
    for (int chn = 1; chn <= ChnsDCS; chn++)
    {
        cJSON *chnObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(chnObj, "Chn", chn);

        double urValues[] = {6.5, 3.25, 1.876};
        cJSON *urArray = cJSON_CreateDoubleArray(urValues, 3);
        cJSON_AddItemToObject(chnObj, "URs", urArray);

        double irValues[] = {5, 1, 0.2};
        cJSON *irArray = cJSON_CreateDoubleArray(irValues, 3);
        cJSON_AddItemToObject(chnObj, "IRs", irArray);

        cJSON_AddItemToArray(dcsChns, chnObj);
    }
    cJSON_AddItemToObject(dcs, "Chns", dcsChns);
    cJSON_AddItemToObject(dataObj, "DCS", dcs);

    // DCM 信息
    cJSON *dcm = cJSON_CreateObject();
    cJSON_AddNumberToObject(dcm, "ChnCount", ChnsDCM);

    cJSON *dcmChns = cJSON_CreateArray();
    for (int chn = 1; chn <= ChnsDCM; chn++)
    {
        cJSON *chnObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(chnObj, "Chn", chn);

        double urValues[] = {6.5, 3.25, 1.876};
        cJSON *urArray = cJSON_CreateDoubleArray(urValues, 3);
        cJSON_AddItemToObject(chnObj, "URs", urArray);

        double irValues[] = {5, 1, 0.2};
        cJSON *irArray = cJSON_CreateDoubleArray(irValues, 3);
        cJSON_AddItemToObject(chnObj, "IRs", irArray);

        cJSON_AddItemToArray(dcmChns, chnObj);
    }
    cJSON_AddItemToObject(dcm, "Chns", dcmChns);
    cJSON_AddItemToObject(dataObj, "DCM", dcm);

    cJSON_AddItemToObject(reply, "Data", dataObj);

    // 将生成的 JSON 转换为字符串
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(reply);
        return;
    }

    // 在字符串首尾加上 '|' 字符
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 为了首尾 '|' 和结束符
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // 将生成的 JSON 写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // 清理
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

void handle_GetDevState(cJSON *data)
{
    // 处理 handle_GetDevState 的逻辑
    //	xil_printf("Handling handle_GetDevState...\n");

    // 创建上行指令的 JSON 对象
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "GetDevState");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *dataObj = cJSON_CreateObject();
    cJSON_AddStringToObject(dataObj, "DevTime", "2024-01-31 00:00:00.123");
    cJSON_AddNumberToObject(dataObj, "Temperature", 24);
    cJSON_AddStringToObject(dataObj, "SyncMode", "GPS");
    cJSON_AddNumberToObject(dataObj, "Longitude", 12.2);
    cJSON_AddNumberToObject(dataObj, "Latitude", 12.2);
    cJSON_AddNumberToObject(dataObj, "DI_Resolution", 0.1);

    cJSON *task = cJSON_CreateObject();
    cJSON_AddStringToObject(task, "FunCode", "");
    cJSON_AddStringToObject(task, "StartTime", "2024-01-31 00:00:00.123");
    cJSON_AddItemToObject(dataObj, "Task", task);

    // AC 信息
    cJSON *ac = cJSON_CreateObject();
    cJSON_AddStringToObject(ac, "Mode", "S");
    cJSON_AddBoolToObject(ac, "ClosedLoop", true);

    cJSON *acChns = cJSON_CreateArray();
    for (int line = 1; line <= LinesAC; line++)
    {
        for (int chn = 1; chn <= ChnsAC; chn++)
        {
            cJSON *chnObj = cJSON_CreateObject();
            cJSON_AddNumberToObject(chnObj, "Line", line);
            cJSON_AddNumberToObject(chnObj, "Chn", chn);
            cJSON_AddNumberToObject(chnObj, "UR", lineAC.ur[chn - 1]);
            cJSON_AddNumberToObject(chnObj, "IR", lineAC.ir[chn - 1]);
            cJSON_AddItemToArray(acChns, chnObj);
        }
    }
    cJSON_AddItemToObject(ac, "Chns", acChns);
    cJSON_AddItemToObject(dataObj, "AC", ac);

    // DCS 信息
    cJSON *dcs = cJSON_CreateArray();
    for (int chn = 1; chn <= ChnsDCS; chn++)
    {
        cJSON *chnObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(chnObj, "Chn", chn);
        cJSON_AddNumberToObject(chnObj, "UR", lineAC.ur[chn - 1]);
        cJSON_AddNumberToObject(chnObj, "IR", lineAC.ir[chn - 1]);
        cJSON_AddItemToArray(dcs, chnObj);
    }
    cJSON_AddItemToObject(dataObj, "DCS", dcs);

    // DCM 信息
    cJSON *dcm = cJSON_CreateArray();
    for (int chn = 1; chn <= ChnsDCM; chn++)
    {
        cJSON *chnObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(chnObj, "Chn", chn);
        cJSON_AddNumberToObject(chnObj, "UR", lineAC.ur[chn - 1]);
        cJSON_AddNumberToObject(chnObj, "IR", lineAC.ir[chn - 1]);
        cJSON_AddItemToArray(dcm, chnObj);
    }
    cJSON_AddItemToObject(dataObj, "DCM", dcm);

    // DO 信息
    cJSON *doInfo = cJSON_CreateArray();
    for (int i = 0; i < ChnsBO; i++)
    {
        cJSON_AddItemToArray(doInfo, cJSON_CreateNumber(lineDO.DO[i].v));
    }
    cJSON_AddItemToObject(dataObj, "DO", doInfo);

    // DI 信息
    cJSON *diInfo = cJSON_CreateArray();
    for (int i = 0; i < ChnsBI; i++)
    {
        cJSON_AddItemToArray(diInfo, cJSON_CreateNumber(lineDI.DI[i].v));
    }
    cJSON_AddItemToObject(dataObj, "DI", diInfo);

    cJSON_AddItemToObject(reply, "Data", dataObj);

    // 映射到硬件

    // 将生成的 JSON 转换为字符串
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(reply);
        return;
    }

    // 在字符串首尾加上 '|' 字符
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 为了首尾 '|' 和结束符
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // 将生成的 JSON 写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // 清理
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

ReportEnableStatus reportStatus = {true, true, true, true, true, true, true, true, true, true, true};
/**
 * @brief 处理 handle_SetReportEnable 的逻辑
 *
 * 该函数从输入的 JSON 数据中提取各报告项的使能状态，并更新到 reportStatus 结构体中。
 * 然后，构造一个回复 JSON 对象，其中包含处理结果和更新后的报告项使能状态，
 * 将该 JSON 对象转换为字符串，并在字符串首尾加上 '|' 字符，最后将字符串写入共享内存。
 *
 * @param data 输入的 JSON 数据，包含需要更新的报告项使能状态
 */
void handle_SetReportEnable(cJSON *data)
{
    // 处理 handle_SetReportEnable 的逻辑
    // xil_printf("Handling handle_SetReportEnable...\n");

    // 更新使能状态
    // 获取 BaseDataAC 并更新使能状态
    cJSON *BaseDataAC = cJSON_GetObjectItem(data, "BaseDataAC");
    if (BaseDataAC != NULL)
    {
        // BaseDataAC 使能状态更新
        reportStatus.BaseDataAC = cJSON_IsTrue(BaseDataAC);
    }

    // 获取 HarmData 并更新使能状态
    cJSON *HarmData = cJSON_GetObjectItem(data, "HarmData");
    if (HarmData != NULL)
    {
        // HarmData 使能状态更新
        reportStatus.HarmData = cJSON_IsTrue(HarmData);
    }

    // 获取 InterHarmData 并更新使能状态
    cJSON *InterHarmData = cJSON_GetObjectItem(data, "InterHarmData");
    if (InterHarmData != NULL)
    {
        // InterHarmData 使能状态更新
        reportStatus.InterHarmData = cJSON_IsTrue(InterHarmData);
    }

    // 获取 BaseDataDCS 并更新使能状态
    cJSON *BaseDataDCS = cJSON_GetObjectItem(data, "BaseDataDCS");
    if (BaseDataDCS != NULL)
    {
        // BaseDataDCS 使能状态更新
        reportStatus.BaseDataDCS = cJSON_IsTrue(BaseDataDCS);
    }

    // 获取 BaseDataDCM 并更新使能状态
    cJSON *BaseDataDCM = cJSON_GetObjectItem(data, "BaseDataDCM");
    if (BaseDataDCM != NULL)
    {
        // BaseDataDCM 使能状态更新
        reportStatus.BaseDataDCM = cJSON_IsTrue(BaseDataDCM);
    }

    // 获取 DI 并更新使能状态
    cJSON *DI = cJSON_GetObjectItem(data, "DI");
    if (DI != NULL)
    {
        // DI 使能状态更新
        reportStatus.DI = cJSON_IsTrue(DI);
    }

    // 获取 DISOE 并更新使能状态
    cJSON *DISOE = cJSON_GetObjectItem(data, "DISOE");
    if (DISOE != NULL)
    {
        // DISOE 使能状态更新
        reportStatus.DISOE = cJSON_IsTrue(DISOE);
    }

    // 获取 DO 并更新使能状态
    cJSON *DO = cJSON_GetObjectItem(data, "DO");
    if (DO != NULL)
    {
        // DO 使能状态更新
        reportStatus.DO = cJSON_IsTrue(DO);
    }

    // 获取 InnerBattery 并更新使能状态
    cJSON *InnerBattery = cJSON_GetObjectItem(data, "InnerBattery");
    if (InnerBattery != NULL)
    {
        // InnerBattery 使能状态更新
        reportStatus.InnerBattery = cJSON_IsTrue(InnerBattery);
    }

    // 获取 VMData 并更新使能状态
    cJSON *VMData = cJSON_GetObjectItem(data, "VMData");
    if (VMData != NULL)
    {
        // VMData 使能状态更新
        reportStatus.VMData = cJSON_IsTrue(VMData);
    }

    /*创建上行指令的 JSON 对象*/
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "SetReportEnable");
    cJSON_AddStringToObject(reply, "Result", "Success");

    // 创建 dataObj JSON 对象
    cJSON *dataObj = cJSON_CreateObject();
    cJSON_AddBoolToObject(dataObj, "BaseDataAC", reportStatus.BaseDataAC);
    cJSON_AddBoolToObject(dataObj, "HarmData", reportStatus.HarmData);
    cJSON_AddBoolToObject(dataObj, "InterHarmData", reportStatus.InterHarmData);
    cJSON_AddBoolToObject(dataObj, "BaseDataDCS", reportStatus.BaseDataDCS);
    cJSON_AddBoolToObject(dataObj, "BaseDataDCM", reportStatus.BaseDataDCM);
    cJSON_AddBoolToObject(dataObj, "DI", reportStatus.DI);
    cJSON_AddBoolToObject(dataObj, "DISOE", reportStatus.DISOE);
    cJSON_AddBoolToObject(dataObj, "DO", reportStatus.DO);
    cJSON_AddBoolToObject(dataObj, "InnerBattery", reportStatus.InnerBattery);
    cJSON_AddBoolToObject(dataObj, "VMData", reportStatus.VMData);

    // 将 dataObj 添加到 reply 中
    cJSON_AddItemToObject(reply, "Data", dataObj);

    // 将生成的 JSON 转换为字符串
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        // 打印错误信息
        xil_printf("Failed to print JSON.\n");
        // 删除 reply 对象
        cJSON_Delete(reply);
        return;
    }

    // 在字符串首尾加上 '|' 字符
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 为了首尾 '|' 和结束符
    if (finalString == NULL)
    {
        // 打印内存分配失败信息
        xil_printf("Failed to allocate memory for final JSON string.\n");
        // 删除 reply 对象
        cJSON_Delete(reply);
        // 释放 string 内存
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // 将生成的 JSON 写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        // 打印写入消息队列失败信息
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        // 打印写入消息队列成功信息
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // 清理
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

void handle_SetDCS(cJSON *data)
{
    // 处理 handle_SetDCS 的逻辑
    //    xil_printf("CPU1: Handling handle_SetDCS...\r\n");

    SetDCS setDCS;
    cJSON *closedLoop = cJSON_GetObjectItem(data, "ClosedLoop");
    if (closedLoop != NULL)
    {
        setDCS.ClosedLoop = cJSON_IsTrue(closedLoop);
    }
    else
    {
        setDCS.ClosedLoop = true; // 默认值
    }

    // 读取Chns
    cJSON *Chns = cJSON_GetObjectItem(data, "Chns");
    if (Chns == NULL)
    {
        xil_printf("No Chns found in JSON.\n");
        return;
    }

    int ChnsCount = cJSON_GetArraySize(Chns);
    for (int i = 0; i < ChnsCount; i++)
    {
        cJSON *Vals = cJSON_GetArrayItem(Chns, i);

        setDCS.Vals[i].Chn = cJSON_GetObjectItem(Vals, "Chn")->valueint;
        setDCS.Vals[i].UR = (float)cJSON_GetObjectItem(Vals, "UR")->valuedouble;
        setDCS.Vals[i].U = (float)cJSON_GetObjectItem(Vals, "U")->valuedouble;
        setDCS.Vals[i].URipple = (float)cJSON_GetObjectItem(Vals, "URipple")->valuedouble;
        setDCS.Vals[i].IR = (float)cJSON_GetObjectItem(Vals, "IR")->valuedouble;
        setDCS.Vals[i].I_ = (float)cJSON_GetObjectItem(Vals, "I")->valuedouble;
        setDCS.Vals[i].IRipple = (float)cJSON_GetObjectItem(Vals, "IRipple")->valuedouble;
    }

    // 打印解析结果以验证
    //     xil_printf("ClosedLoop: %d\r\n", setDCS.ClosedLoop);
    for (int i = 0; i < ChnsDCS; i++)
    {
        printf("CHn: %d, UR: %.2f, U: %.2f, URipple: %.2f, IR: %.2f, I: %.2f, IRipple: %.2f\r\n",
               setDCS.Vals[i].Chn,
               setDCS.Vals[i].UR, setDCS.Vals[i].U, setDCS.Vals[i].URipple,
               setDCS.Vals[i].IR, setDCS.Vals[i].I_, setDCS.Vals[i].IRipple);
    }

    // 回报JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetDCS");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = true;
    replyData.ClosedLoop = setDCS.ClosedLoop;

    // 写入回报指令到共享内存
    write_reply_to_shared_memory(&replyData);
}

void handle_SetDCM(cJSON *data)
{
    // 处理 handle_SetDCM 的逻辑
    //    xil_printf("CPU1: Handling handle_SetDCM...\r\n");

    SetDCM setDCM;
    // 读取Chns
    cJSON *Chns = cJSON_GetObjectItem(data, "Chns");
    if (Chns == NULL)
    {
        xil_printf("No Chns found in JSON.\n");
        return;
    }
    int ChnsCount = cJSON_GetArraySize(Chns);

    for (int i = 0; i < ChnsCount; i++)
    {
        cJSON *Vals = cJSON_GetArrayItem(Chns, i);

        setDCM.Vals[i].Chn = cJSON_GetObjectItem(Vals, "Chn")->valueint;
        setDCM.Vals[i].UR = (float)cJSON_GetObjectItem(Vals, "UR")->valuedouble;
        setDCM.Vals[i].IR = (float)cJSON_GetObjectItem(Vals, "IR")->valuedouble;
    }
    // 打印解析结果以验证

    for (int i = 0; i < ChnsDCM; i++)
    {
        printf("CHn: %d, UR: %.2f,IR: %.2f, \r\n",
               setDCM.Vals[i].Chn,
               setDCM.Vals[i].UR,
               setDCM.Vals[i].IR);
    }

    // 回报JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetDCM");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    // 写入回报指令到共享内存
    write_reply_to_shared_memory(&replyData);
}

SetACS setACS;
/**
 * @brief 处理 SetACS 请求
 *
 * 该函数解析传入的 JSON 数据，并根据数据更新相应的变量，并将解析结果打印出来。
 * 最后，生成回复数据并写入共享内存，同时映射数据到硬件以生成交流信号。
 *
 * @param data 包含 SetACS 请求数据的 JSON 对象指针
 */
void handle_SetACS(cJSON *data)
{
    // xil_printf("CPU1: Handling handle_SetACS...\r\n");

    bool onlyRangeFieldsFound = true; // 标记是否只找到了量程相关字段
    bool valsPresent = false;         // 标记 vals 数组是否存在
    bool closedLoopPresent = false;   // 标记 ClosedLoop 是否存在

    // 处理 ClosedLoop，如果存在则更新，否则保留原值
    cJSON *closedLoop = cJSON_GetObjectItem(data, "ClosedLoop");
    if (closedLoop != NULL)
    {
        setACS.ClosedLoop = cJSON_IsTrue(closedLoop);
        onlyRangeFieldsFound = false; // 发现非量程字段
        closedLoopPresent = true;
        // printf("CPU1: Debug: ClosedLoop found in JSON.\n");
    }
    // 注意：如果 ClosedLoop 未在 JSON 中提供，setACS.ClosedLoop 会保持其之前的值

    // 获取 vals 项
    cJSON *vals = cJSON_GetObjectItem(data, "vals");
    if (vals != NULL)
    {
        valsPresent = true;
        // printf("CPU1: Debug: 'vals' array found in JSON.\n");
        // 获取 vals 数组的大小
        int valsCount = cJSON_GetArraySize(vals);
        for (int i = 0; i < valsCount; i++)
        {
            // 获取 vals 数组中的每个元素
            cJSON *val = cJSON_GetArrayItem(vals, i);
            if (!val)
                continue; // 跳过无效元素

            // --- 解析并更新 setACS 结构体，仅当 JSON 中存在对应项时 ---

            // 获取 Line 项 (通常不需要标记为非量程字段)
            cJSON *line = cJSON_GetObjectItem(val, "Line");
            if (line && cJSON_IsNumber(line))
            {
                setACS.Vals[i].Line = line->valueint;
            }
            // 获取 Chn 项 (通常不需要标记为非量程字段)
            cJSON *chn = cJSON_GetObjectItem(val, "Chn");
            if (chn && cJSON_IsNumber(chn))
            {
                setACS.Vals[i].Chn = chn->valueint;
            }

            // 获取 F 项
            cJSON *f = cJSON_GetObjectItem(val, "F");
            if (f && cJSON_IsNumber(f))
            {
                setACS.Vals[i].F = (float)f->valuedouble;
                onlyRangeFieldsFound = false; // 发现非量程字段 F
                // printf("CPU1: Debug: Field 'F' found in vals[%d].\n", i);
            }

            // 获取 UR 项 (量程字段)
            cJSON *ur = cJSON_GetObjectItem(val, "UR");
            if (ur && cJSON_IsNumber(ur))
            {
                //如果ur不为0
                if (ur->valuedouble != 0)
                {
                    setACS.Vals[i].UR = (float)ur->valuedouble;
                    // printf("CPU1: Debug: Field 'UR' found in vals[%d].\n", i);
                }
            }

            // 获取 U 项
            cJSON *u = cJSON_GetObjectItem(val, "U");
            if (u && cJSON_IsNumber(u))
            {
                setACS.Vals[i].U = (float)u->valuedouble;
                onlyRangeFieldsFound = false; // 发现非量程字段 U
                // printf("CPU1: Debug: Field 'U' found in vals[%d].\n", i);
            }

            // 获取 PhU 项
            cJSON *phU = cJSON_GetObjectItem(val, "PhU");
            if (phU && cJSON_IsNumber(phU))
            {
                setACS.Vals[i].PhU = (float)phU->valuedouble;
                onlyRangeFieldsFound = false; // 发现非量程字段 PhU
                // printf("CPU1: Debug: Field 'PhU' found in vals[%d].\n", i);
            }

            // 获取 IR 项 (量程字段)
            cJSON *ir = cJSON_GetObjectItem(val, "IR");
            if (ir && cJSON_IsNumber(ir))
            {
                //如果ir不为0
                if (ir->valuedouble != 0)
                {
                    setACS.Vals[i].IR = (float)ir->valuedouble;
                    // printf("CPU1: Debug: Field 'IR' found in vals[%d].\n", i);
                }
            }

            // 获取 I 项 (注意 JSON 键名为 "I")
            cJSON *i_json = cJSON_GetObjectItem(val, "I");
            if (i_json && cJSON_IsNumber(i_json))
            {
                setACS.Vals[i].I_ = (float)i_json->valuedouble;
                onlyRangeFieldsFound = false; // 发现非量程字段 I
                // printf("CPU1: Debug: Field 'I' found in vals[%d].\n", i);
            }

            // 获取 PhI 项
            cJSON *phI = cJSON_GetObjectItem(val, "PhI");
            if (phI && cJSON_IsNumber(phI))
            {
                setACS.Vals[i].PhI = (float)phI->valuedouble;
                onlyRangeFieldsFound = false; // 发现非量程字段 PhI
                // printf("CPU1: Debug: Field 'PhI' found in vals[%d].\n", i);
            }
        }
    }
    else
    {
        // printf("CPU1: Debug: 'vals' array not found in JSON.\n");
        // 如果 vals 数组不存在，则不能是“仅量程切换”指令（除非只改变 ClosedLoop）
        if (closedLoopPresent)
        {
            onlyRangeFieldsFound = false;
        }
        else
        {
            // 如果 vals 和 ClosedLoop 都不存在，则认为不是量程切换（而是无效或空指令）
            onlyRangeFieldsFound = false; // 或者可以保持 true 并依赖 valsPresent 判断
        }
    }

    // // --- 打印最终的 setACS 状态 (用于调试) ---
    // xil_printf("CPU1: Final setACS State:\r\n");
    // xil_printf("ClosedLoop: %d\r\n", setACS.ClosedLoop);
    // for (int i = 0; i < LinesAC * ChnsAC; i++)
    // {
    //     printf("  vals[%d]: Line=%d, Chn=%d, F=%.2f, UR=%.2f, U=%.2f, PhU=%.2f, IR=%.2f, I=%.2f, PhI=%.2f\n",
    //            i, setACS.Vals[i].Line, setACS.Vals[i].Chn, setACS.Vals[i].F,
    //            setACS.Vals[i].UR, setACS.Vals[i].U, setACS.Vals[i].PhU,
    //            setACS.Vals[i].IR, setACS.Vals[i].I_, setACS.Vals[i].PhI);
    // }
    // xil_printf("CPU1: Debug: valsPresent=%d, onlyRangeFieldsFound=%d\n", valsPresent, onlyRangeFieldsFound);

    // --- 回报 JSON ---
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetACS");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = true; // SetACS 的回复总是包含 ClosedLoop 状态
    replyData.ClosedLoop = setACS.ClosedLoop;
    write_reply_to_shared_memory(&replyData);

    // --- 数据映射到硬件 ---

    // 更新全局变量 (这些变量会被 power_amplifier_control 和 str_wr_bram 使用)
    Wave_Frequency = setACS.Vals[0].F; // 频率 (假设所有通道频率一致)
    // 如果 Wave_Frequency 不在 45 到 65Hz，添加报错提示并修正
    if (Wave_Frequency < 45.0f || Wave_Frequency > 65.0f)
    {
        xil_printf("CPU1 Warning: Frequency out of range (%.2f Hz). Expected 45-65 Hz. Setting to 50 Hz.\n", Wave_Frequency);
        Wave_Frequency = 50.0f;
        // 同步修正 setACS 内部的值，以便下次读取时一致
        for (int i = 0; i < LinesAC * ChnsAC; i++)
        {
            setACS.Vals[i].F = 50.0f;
        }
    }

    // 更新相位、幅值、量程
    for (int i = 0; i < 4; i++) // 假设 ChnsAC 是 4
    {
        Phase_shift[i] = setACS.Vals[i].PhU;     // 电压相位
        Phase_shift[i + 4] = setACS.Vals[i].PhI; // 电流相位

        // 幅值计算 (%): (设定值 / 档位) * 100
        // 防止除零错误
        if (fabs(setACS.Vals[i].UR) > 1e-6)
        {
            Wave_Amplitude[i] = (setACS.Vals[i].U / setACS.Vals[i].UR) * 100.0f;
        }
        else
        {
            Wave_Amplitude[i] = 0.0f; // 如果档位为0，幅值百分比也为0
                                      // xil_printf("CPU1 Warning: Voltage range UR for channel %d is zero or close to zero.\n", i);
        }

        if (fabs(setACS.Vals[i].IR) > 1e-6)
        {
            Wave_Amplitude[i + 4] = (setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100.0f;
        }
        else
        {
            Wave_Amplitude[i + 4] = 0.0f; // 如果档位为0，幅值百分比也为0
                                          // xil_printf("CPU1 Warning: Current range IR for channel %d is zero or close to zero.\n", i);
        }

        // 量程代码转换
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }

    // 清除谐波相关设置 (因为 SetACS 只处理基波)
    memset(numHarmonics, 0, sizeof(numHarmonics));
    memset(harmonics, 0, sizeof(harmonics));
    memset(harmonics_phases, 0, sizeof(harmonics_phases));

    // --- 新的量程切换/输出控制逻辑 ---
    if (valsPresent && onlyRangeFieldsFound) // 确保 vals 存在且只包含量程字段
    {
        // 判定为仅切换量程指令
        printf("CPU1: Range switching command detected. Disabling output.\n");
        enable = 0x00;        // 关闭所有通道输出
        str_wr_bram(PID_OFF); // 确保PID关闭 (即使不开环也要生成波形数据到 BRAM)
        // power_amplifier_control 需要最新的 Wave_Amplitude (即使是0) 和 Wave_Range 来设置硬件
        power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF); // 关闭功放

        // 更新设备状态 (UDP 结构体)
        devState.bACMeterMode = 0;                // 量程切换通常意味着处于源模式
        devState.bACRunning = 0;                  // 切换量程时停止运行状态
        devState.bClosedLoop = setACS.ClosedLoop; // 保持 JSON 中指定的 (或之前的) 闭环状态
    }
    else
    {
        // 正常设置指令 (包含幅值/相位/频率等) 或仅改变 ClosedLoop
        uint8_t powamp_state = POWAMP_OFF; // 默认关闭功放
        enable = 0x00;                     // 默认关闭所有通道

        // 检查是否有任何通道需要输出 (幅值大于一个很小的阈值)
        for (int i = 0; i < 8; i++)
        {
            if (Wave_Amplitude[i] > 0.001) // 幅值百分比 > 0.001% 才认为需要输出
            {
                enable |= (1 << i);       // 使能该通道
                powamp_state = POWAMP_ON; // 只要有一个通道输出，就打开功放
            }
        }

        if (powamp_state == POWAMP_ON)
        {
            // printf("CPU1: Normal ACS setting. Enabling output based on amplitude. Enable Mask: 0x%X\n", enable);
            // 根据 setACS.ClosedLoop 决定 PID 状态
            PID_STATE pid_state = setACS.ClosedLoop ? PID_ON : PID_OFF;
            str_wr_bram(pid_state);
            power_amplifier_control(Wave_Amplitude, Wave_Range, pid_state, powamp_state);

            // 更新设备状态 (UDP 结构体)
            devState.bACMeterMode = 0; // 正常设置通常是源模式
            devState.bACRunning = 1;   // 运行状态
            devState.bClosedLoop = setACS.ClosedLoop;
        }
        else
        {
            // 所有通道幅值都为 0 或指令无效/仅改变ClosedLoop且幅值为0
            printf("CPU1: Normal ACS setting but all amplitudes are zero. Disabling output.\n");
            enable = 0x00;
            str_wr_bram(PID_OFF);
            power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);

            // 更新设备状态 (UDP 结构体)
            devState.bACMeterMode = 0;
            devState.bACRunning = 0; // 停止运行状态
            devState.bClosedLoop = setACS.ClosedLoop;
        }
    }
}

SetACM setACM;
void handle_SetACM(cJSON *data)
{
    // 处理 handle_SetACM 的逻辑
    //    xil_printf("CPU1: Handling handle_SetACM...\r\n");

    int dataCount = cJSON_GetArraySize(data);
    for (int i = 0; i < dataCount; i++)
    {
        cJSON *Vals = cJSON_GetArrayItem(data, i);

        setACM.Vals[i].Line = cJSON_GetObjectItem(Vals, "Line")->valueint;
        setACM.Vals[i].Chn = cJSON_GetObjectItem(Vals, "Chn")->valueint;
        setACM.Vals[i].UR = (float)cJSON_GetObjectItem(Vals, "UR")->valuedouble;
        setACM.Vals[i].IR = (float)cJSON_GetObjectItem(Vals, "IR")->valuedouble;
    }
    // 打印解析结果以验证
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        printf("Line: %d, CHn: %d, UR: %.2f,IR: %.2f, \n",
               setACM.Vals[i].Line,
               setACM.Vals[i].Chn,
               setACM.Vals[i].UR,
               setACM.Vals[i].IR);
    }

    // 回报JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetACM");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    // 写入回报指令到共享内存
    write_reply_to_shared_memory(&replyData);

    // 映射到硬件
    // 更改UDP结构体100DevState的运行状态
    devState.bACMeterMode = 1;
    devState.bACRunning = 1;
}

SetHarm setHarm;
void handle_SetHarm(cJSON *data)
{
    // printf("CPU1: 处理谐波设置命令...\r\n");

    // 解析传入的数据
    int dataCount = cJSON_GetArraySize(data);
    printf("CPU1: %d harmonic Settings received\n", dataCount);

    // 处理请求中的所有谐波
    for (int i = 0; i < dataCount; i++)
    {
        cJSON *item = cJSON_GetArrayItem(data, i);
        if (item == NULL)
        {
            continue;
        }

        // 获取必要的参数：线路和通道编号和谐波次数
        cJSON *line = cJSON_GetObjectItem(item, "Line");
        cJSON *chn = cJSON_GetObjectItem(item, "Chn");
        cJSON *hn = cJSON_GetObjectItem(item, "HN");

        // 至少需要通道号和谐波次数
        if (!chn || !hn)
        {
            printf("CPU1: Missing required Chn or HN parameter, skipping\n");
            continue;
        }

        // 获取可选参数
        cJSON *u = cJSON_GetObjectItem(item, "U");
        cJSON *phU = cJSON_GetObjectItem(item, "PhU");
        cJSON *iField = cJSON_GetObjectItem(item, "I");
        cJSON *phI = cJSON_GetObjectItem(item, "PhI");

        // 计算通道索引和谐波索引（从0开始）
        int channelIndex = chn->valueint - 1; // JSON中通道从1开始
        int harmonicIndex = hn->valueint - 2; // 谐波索引从0开始，对应第2次谐波

        // 检查数组边界以防溢出
        if (channelIndex < 0 || channelIndex >= CHANNL_MAX ||
            harmonicIndex < 0 || harmonicIndex >= MAX_HARMONICS)
        {
            printf("CPU1: Invalid Chn: %d, HN: %d - Index out of range\n",
                   chn->valueint, hn->valueint);
            continue;
        }

        // 存储在setHarm中用于记录
        int setHarmIndex = i;
        if (setHarmIndex < LinesAC * ChnsAC * HarmNumberMax)
        {
            if (line)
                setHarm.Vals[setHarmIndex].Line = line->valueint;
            setHarm.Vals[setHarmIndex].Chn = chn->valueint;
            setHarm.Vals[setHarmIndex].HN = hn->valueint;

            // 只更新提供的参数，其他保持不变
            if (u)
                setHarm.Vals[setHarmIndex].U = (float)u->valuedouble;
            if (phU)
                setHarm.Vals[setHarmIndex].PhU = (float)phU->valuedouble;
            if (iField)
                setHarm.Vals[setHarmIndex].I_ = (float)iField->valuedouble;
            if (phI)
                setHarm.Vals[setHarmIndex].PhI = (float)phI->valuedouble;
        }

        // 更新电压(U)谐波
        if (u != NULL)
        {
            float uValue = (float)u->valuedouble;

            // 如果需要，更新最大谐波次数
            if (hn->valueint > numHarmonics[channelIndex])
            {
                numHarmonics[channelIndex] = hn->valueint;
            }

            // 直接存储，因为harmonics中0.1表示10%
            harmonics[channelIndex][harmonicIndex] = uValue / 100.0f;

            // 如果提供了相位
            if (phU != NULL)
            {
                harmonics_phases[channelIndex][harmonicIndex] = (float)phU->valuedouble;
            }

            // printf("CPU1: Updated voltage harmonic - Chn: %d, HN: %d, Amplitude: %.2f%% (stored as %.3f)",
            //        chn->valueint, hn->valueint, uValue, uValue / 100.0f);

            if (phU != NULL)
            {
                // printf(", Phase: %.2f", (float)phU->valuedouble);
            }
            // printf("\n");
        }

        // 更新电流(I)谐波（通道偏移4）
        if (iField != NULL)
        {
            float iValue = (float)iField->valuedouble;

            // 如果需要，更新最大谐波次数
            if (hn->valueint > numHarmonics[channelIndex + 4])
            {
                numHarmonics[channelIndex + 4] = hn->valueint;
            }

            // 直接存储，因为harmonics中0.1表示10%
            harmonics[channelIndex + 4][harmonicIndex] = iValue / 100.0f;

            // 如果提供了相位
            if (phI != NULL)
            {
                harmonics_phases[channelIndex + 4][harmonicIndex] = (float)phI->valuedouble;
            }

            // printf("CPU1: Updated current harmonic - Chn: %d, HN: %d, Amplitude: %.2f%% (stored as %.3f)",
            //        chn->valueint, hn->valueint, iValue, iValue / 100.0f);

            if (phI != NULL)
            {
                // printf(", Phase: %.2f", (float)phI->valuedouble);
            }
            // printf("\n");
        }
    }

    // 使用更新后的谐波生成波形
    printf("CPU1: Generating waveform with updated harmonics\n");
    str_wr_bram(PID_OFF);

    // 准备并发送回复
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetHarm");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

void handle_SetInterHarm(cJSON *data)
{
    // 处理 handle_SetInterHarm 的逻辑
    //    xil_printf("CPU1: Handling handle_SetInterHarm...\r\n");
}

SetDO setDO;
void handle_SetDO(cJSON *data)
{
    // 处理 handle_SetDO 的逻辑
    //    xil_printf("CPU1: Handling handle_SetDO...\r\n");

    int dataCount = cJSON_GetArraySize(data);
    for (int i = 0; i < dataCount; i++)
    {
        cJSON *Vals = cJSON_GetArrayItem(data, i);

        setDO.Vals[i].Chn = cJSON_GetObjectItem(Vals, "Chn")->valueint;
        setDO.Vals[i].val = cJSON_GetObjectItem(Vals, "val")->valueint;
    }
    // 打印解析结果以验证

    for (int i = 0; i < ChnsBO; i++)
    {
        printf("CHn: %d, val: %d\n",
               setDO.Vals[i].Chn,
               setDO.Vals[i].val);
    }

    // 映射到硬件
    // 开关量 下次做
    //	Write_Read_Switch(bit_8 , 0xf0000000);//读写开关量模块 可配置8、16、24、32位 //8位模式下，0xf0000000为高4位写1

    // 回报JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetDO");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    // 写入回报指令到共享内存
    write_reply_to_shared_memory(&replyData);
}

void handle_StopDCS(cJSON *data)
{
    // 处理 handle_StopDCS 的逻辑
    //	xil_printf("CPU1: Handling handle_StopDCS...\r\n");

    // 回报JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "StopDCS");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false; // 不需要 ClosedLoop 字段

    // 写入回报指令到共享内存
    write_reply_to_shared_memory(&replyData);
}
void handle_StopAC(cJSON *data)
{
    // 交流表/源关闭（同时清空所有基波、谐波、间谐波输出）
    //  处理 handle_StopACS 的逻辑
    //    xil_printf("CPU1: Handling handle_StopAC...\r\n");

    // 回报JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "StopAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false; // 不需要 ClosedLoop 字段
    // 写入回报指令到共享内存
    write_reply_to_shared_memory(&replyData);

    // 映射到硬件

    Wave_Frequency = 50;                         // 频率
    memset(Phase_shift, 0, sizeof(Phase_shift)); // 清空基波相位

    memset(numHarmonics, 0, sizeof(numHarmonics));         // 清除谐波
    memset(harmonics, 0, sizeof(harmonics));               // 清除谐波幅值
    memset(harmonics_phases, 0, sizeof(harmonics_phases)); // 清除谐波相位
    enable = 0x00;                                         // 关闭通道输出
    str_wr_bram(PID_OFF);                                  // 生成交流信号

    power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
    // 更改UDP结构体100DevState的运行状态
    devState.bACMeterMode = 0;
    devState.bACRunning = 0;
}

void handle_ClearHarm(cJSON *data)
{
    // 处理 handle_ClearHarm 的逻辑
    //    xil_printf("CPU1: Handling handle_ClearHarm...\r\n");

    // 回报JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "ClearHarm");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false; // 不需要 ClosedLoop 字段

    // 写入回报指令到共享内存
    write_reply_to_shared_memory(&replyData);

    // 映射到硬件
    memset(numHarmonics, 0, sizeof(numHarmonics)); // 清除谐波
    memset(harmonics, 0, sizeof(harmonics));
    memset(harmonics_phases, 0, sizeof(harmonics_phases));
    // 生成交流信号
    str_wr_bram(PID_OFF);
}

void handle_ClearInterHarm(cJSON *data)
{
    // 处理 handle_ClearInterHarm 的逻辑
    //    xil_printf("CPU1: Handling handle_ClearInterHarm...\r\n");
}

/**
 * @brief 处理设置校准点命令
 *
 * 该函数根据输入的校准点参数设置设备输出相应的幅值
 * 支持100%和20%两个校准点，如果不符合任一校准点则返回失败
 *
 * @param data 包含校准点数据的 JSON 对象
 */
void handle_SetCalibrateAC(cJSON *data)
{
    // 从JSON中提取校准点信息
    cJSON *mode = cJSON_GetObjectItem(data, "Mode");
    // 提取点信息
    cJSON *point = cJSON_GetObjectItem(data, "Point");

    if (!mode || !point)
    {
        printf("CPU1: SetCalibrateAC: The mode or point data is missing\r\n");

        // 返回失败响应
        ReplyData replyData;
        strcpy(replyData.FunCode, "SetCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // 提取点信息
    cJSON *ur = cJSON_GetObjectItem(point, "UR");
    cJSON *ir = cJSON_GetObjectItem(point, "IR");
    cJSON *u = cJSON_GetObjectItem(point, "U");
    cJSON *i = cJSON_GetObjectItem(point, "I");

    if (!ur || !ir || !u || !i)
    {
        printf("CPU1: SetCalibrateAC: Missing point data value\r\n");

        // 返回失败响应
        ReplyData replyData;
        strcpy(replyData.FunCode, "SetCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    float urValue = (float)ur->valuedouble;
    float irValue = (float)ir->valuedouble;
    float uValue = (float)u->valuedouble;
    float iValue = (float)i->valuedouble;

    // 判断装置是否有UR IR这个档位
    // 定义浮点数比较的误差范围
    const float epsilon = 0.01f;

    // 检查电压档位
    bool validURFound = false;
    float voltageRanges[] = {6.5f, 3.25f, 1.876f};
    for (int i = 0; i < 3; i++)
    {
        if (fabsf(urValue - voltageRanges[i]) < epsilon)
        {
            validURFound = true;
            break;
        }
    }

    // 检查电流档位
    bool validIRFound = false;
    float currentRanges[] = {5.0f, 1.0f, 0.2f};
    for (int i = 0; i < 3; i++)
    {
        if (fabsf(irValue - currentRanges[i]) < epsilon)
        {
            validIRFound = true;
            break;
        }
    }

    if (!validURFound || !validIRFound)
    {
        printf("CPU1: SetCalibrateAC: Invalid ur or ir gear value\r\n");
        printf("CPU1: UR=%f (Effective value: 6.5, 3.25, 1.876), IR=%f (Effective value: 5.0, 1.0, 0.2)\r\n",
               urValue, irValue);

        // 返回失败响应
        ReplyData replyData;
        strcpy(replyData.FunCode, "SetCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // 检查是否为100%或20%校准点
    bool is100PercentPoint = fabs(uValue - urValue) < epsilon && fabs(iValue - irValue) < epsilon;
    bool is20PercentPoint = fabs(uValue - urValue * 0.2f) < epsilon && fabs(iValue - irValue * 0.2f) < epsilon;

    if (!is100PercentPoint && !is20PercentPoint)
    {
        printf("CPU1: SetCalibrateAC: Not a valid calibration point\r\n");
        printf("CPU1: The calibration point must be 100%% point(U=%f, I=%f)Or 20%% point(U=%f, I=%f)\r\n",
               urValue, irValue, urValue * 0.2f, irValue * 0.2f);

        // 返回失败响应
        ReplyData replyData;
        strcpy(replyData.FunCode, "SetCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    printf("CPU1: SetCalibrateAC: Set calibration points: UR=%f, IR=%f, U=%f, I=%f (Calibration point type: %s)\r\n",
           urValue, irValue, uValue, iValue, is100PercentPoint ? "100%" : "20%");

    // 设置所有通道的输出
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        setACS.Vals[i].UR = urValue;
        setACS.Vals[i].IR = irValue;
        setACS.Vals[i].U = uValue;
        setACS.Vals[i].I_ = iValue;
    }

    // 更新波形幅度和范围
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = (float)(setACS.Vals[i].U / setACS.Vals[i].UR) * 100;
        Wave_Amplitude[i + 4] = (float)(setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }
    // 应用设置
    enable = 0xff;
    devState.bACMeterMode = 0; // 源模式
    devState.bACRunning = 1;   // AC运行
    devState.bClosedLoop = 0;  // 开环
    str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);
    power_amplifier_control(Wave_Amplitude, Wave_Range, devState.bClosedLoop == 1 ? PID_ON : PID_OFF, POWAMP_ON);

    // 返回成功响应
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetCalibrateAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

/**
 * @brief 处理回写校准点标准值命令
 *
 * 该函数根据设定值和实测值更新校准参数
 * 支持100%和20%两个校准点，以及相位校准
 *
 * @param data 包含校准数据的 JSON 对象
 */
void handle_WriteCalibrateAC(cJSON *data)
{
    // 从JSON中提取校准信息
    cJSON *mode = cJSON_GetObjectItem(data, "Mode");
    cJSON *point = cJSON_GetObjectItem(data, "Point");
    cJSON *chns = cJSON_GetObjectItem(data, "Chns");

    if (!mode || !point || !chns)
    {
        printf("CPU1: WriteCalibrateAC: lack Mode, Point or Chns \r\n");

        // 返回失败响应
        ReplyData replyData;
        strcpy(replyData.FunCode, "WriteCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // 提取点信息
    cJSON *ur = cJSON_GetObjectItem(point, "UR");
    cJSON *ir = cJSON_GetObjectItem(point, "IR");
    cJSON *u = cJSON_GetObjectItem(point, "U");
    cJSON *i = cJSON_GetObjectItem(point, "I");

    // 检查是否有足够的电压校准或电流校准参数
    bool hasVoltageCalibration = (ur != NULL && u != NULL);
    bool hasCurrentCalibration = (ir != NULL && i != NULL);

    if (!hasVoltageCalibration && !hasCurrentCalibration)
    {
        printf("CPU1: WriteCalibrateAC:  Lack of necessary calibration parameter combination (ur+ u or ir+ i required)\r\n");

        // 返回失败响应
        ReplyData replyData;
        strcpy(replyData.FunCode, "WriteCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // 初始化校准值变量，并根据可用参数进行赋值
    float urValue = 0.0f, irValue = 0.0f, uValue = 0.0f, iValue = 0.0f;
    int ur_idx = -1, ir_idx = -1;

    if (hasVoltageCalibration)
    {
        urValue = (float)ur->valuedouble;
        uValue = (float)u->valuedouble;
        ur_idx = get_voltage_index_by_value(urValue);
    }

    if (hasCurrentCalibration)
    {
        irValue = (float)ir->valuedouble;
        iValue = (float)i->valuedouble;
        ir_idx = get_current_index_by_value(irValue);
    }

    // 获取校准通道数量
    int chnsCount = cJSON_GetArraySize(chns);
    if (chnsCount <= 0)
    {
        printf("CPU1: WriteCalibrateAC: No channel data is provided\r\n");

        // 返回失败响应
        ReplyData replyData;
        strcpy(replyData.FunCode, "WriteCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // 判断校准点类型（分别检查电压和电流部分）
    const float epsilon = 0.01f;
    bool isVoltage100PercentPoint = false, isVoltage20PercentPoint = false;
    bool isCurrent100PercentPoint = false, isCurrent20PercentPoint = false;

    if (hasVoltageCalibration)
    {
        isVoltage100PercentPoint = fabs(uValue - urValue) < epsilon;
        isVoltage20PercentPoint = fabs(uValue - urValue * 0.2f) < epsilon;

        if (!isVoltage100PercentPoint && !isVoltage20PercentPoint)
        {
            printf("CPU1: WriteCalibrateAC: Invalid voltage calibration point U=%f, UR=%f\r\n", uValue, urValue);
            printf("CPU1: Voltage calibration point must be 100%% point (u≈%f) or 20%% point (u≈%f)\r\n", urValue, urValue * 0.2f);

            // 取消电压校准
            hasVoltageCalibration = false;
        }
    }

    if (hasCurrentCalibration)
    {
        isCurrent100PercentPoint = fabs(iValue - irValue) < epsilon;
        isCurrent20PercentPoint = fabs(iValue - irValue * 0.2f) < epsilon;

        if (!isCurrent100PercentPoint && !isCurrent20PercentPoint)
        {
            printf("CPU1: WriteCalibrateAC: Invalid current calibration point I=%f, IR=%f\r\n", iValue, irValue);
            printf("CPU1: The current calibration point must be 100%%point(I≈%f)or 20%%point(I≈%f)\r\n", irValue, irValue * 0.2f);

            // 取消电流校准
            hasCurrentCalibration = false;
        }
    }

    // 如果没有有效的校准点，则返回失败
    if (!hasVoltageCalibration && !hasCurrentCalibration)
    {
        ReplyData replyData;
        strcpy(replyData.FunCode, "WriteCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // 打印将执行的校准信息
    printf("CPU1: WriteCalibrateAC: Execution calibration - ");
    if (hasVoltageCalibration)
    {
        printf("Voltage calibration (UR=%f, U=%f, type:%s) ||",
               urValue, uValue, isVoltage100PercentPoint ? "100%" : "20%");
    }
    if (hasCurrentCalibration)
    {
        printf(" Current calibration (IR=%f, I=%f, type:%s)",
               irValue, iValue, isCurrent100PercentPoint ? "100%" : "20%");
    }
    printf("\r\n");

    // 对每个通道进行校准
    for (int j = 0; j < chnsCount; j++)
    {
        cJSON *chn = cJSON_GetArrayItem(chns, j);

        if (!chn)
            continue; // 跳过无效项

        cJSON *line_json = cJSON_GetObjectItem(chn, "Line");
        cJSON *channel_json = cJSON_GetObjectItem(chn, "Chn");
        cJSON *actual_u_json = cJSON_GetObjectItem(chn, "U");
        cJSON *actual_i_json = cJSON_GetObjectItem(chn, "I");
        cJSON *actual_phu_json = cJSON_GetObjectItem(chn, "PhU");
        cJSON *actual_phi_json = cJSON_GetObjectItem(chn, "PhI");

        if (!line_json || !channel_json)
        {
            printf("CPU1: WriteCalibrateAC: channel %d Missing data\r\n", j);
            continue; // 跳过无效项
        }

        int line = line_json->valueint;
        int channel = channel_json->valueint;

        // 映射通道索引
        int u_idx = channel - 1;     // 0-3 对应 UA, UB, UC, UX
        int i_idx = channel - 1 + 4; // 4-7 对应 IA, IB, IC, IX

        printf("CPU1: Processing channel Line=%d, Chn=%d:", line, channel);

        // --- 开始 AD 校准 ---
        // AD 电压校准
        if (hasVoltageCalibration && actual_u_json && ur_idx >= 0) // 确保有电压校准请求、目标值和有效范围索引
        {
            float target_ad_u = (float)actual_u_json->valuedouble; // 目标值 (外部仪器测量)
            float current_ad_u = lineAC.u[u_idx];                  // 当前值 (内部AD测量)

            printf(" AD U Target=%.4f, Current=%.4f |", target_ad_u, current_ad_u);

            // 避免除零错误，并确保目标值和当前值有意义
            if (fabs(target_ad_u) > 0.001f && fabs(current_ad_u) > 0.001f)
            {
                float ad_u_ratio = current_ad_u / target_ad_u;           // 计算比例: 当前值 / 目标值
                float old_ad_correct = AD_Correct[u_idx][ur_idx];        // 获取旧系数
                AD_Correct[u_idx][ur_idx] = old_ad_correct * ad_u_ratio; // 更新系数
                printf(" AD U Calib: Ratio=%.4f, OldCorr=%.6f -> NewCorr=%.6f |",
                       ad_u_ratio, old_ad_correct, AD_Correct[u_idx][ur_idx]);
            }
            else
            {
                printf(" AD U Calib: Skipped (Target or Current too small) |");
            }
        }
        else if (hasVoltageCalibration)
        {
            printf(" AD U Calib: Skipped (No actual_u or invalid ur_idx) |");
        }

        // AD 电流校准
        if (hasCurrentCalibration && actual_i_json && ir_idx >= 0) // 确保有电流校准请求、目标值和有效范围索引
        {
            float target_ad_i = (float)actual_i_json->valuedouble; // 目标值 (外部仪器测量)
            // *** 注意: 假设 lineAC.i[0..3] 对应 IA..IX ***
            // *** 所以电流索引应该是 channel - 1 ***
            float current_ad_i = lineAC.i[channel - 1]; // 当前值 (内部AD测量)

            printf(" AD I Target=%.4f, Current=%.4f |", target_ad_i, current_ad_i);

            // 避免除零错误，并确保目标值和当前值有意义
            if (fabs(target_ad_i) > 0.001f && fabs(current_ad_i) > 0.001f)
            {
                float ad_i_ratio = current_ad_i / target_ad_i; // 计算比例: 当前值 / 目标值
                // 使用 i_idx (4-7) 访问 AD_Correct
                float old_ad_correct = AD_Correct[i_idx][ir_idx];        // 获取旧系数
                AD_Correct[i_idx][ir_idx] = old_ad_correct * ad_i_ratio; // 更新系数
                printf(" AD I Calib: Ratio=%.4f, OldCorr=%.6f -> NewCorr=%.6f |",
                       ad_i_ratio, old_ad_correct, AD_Correct[i_idx][ir_idx]);
            }
            else
            {
                printf(" AD I Calib: Skipped (Target or Current too small) |");
            }
        }
        else if (hasCurrentCalibration)
        {
            printf(" AD I Calib: Skipped (No actual_i or invalid ir_idx) |");
        }
        // --- 结束 AD 校准 ---

        // --- 开始 DA 校准 ---
        if (hasVoltageCalibration && actual_u_json)
        {
            float actualU = (float)actual_u_json->valuedouble;
            printf(" Voltage measured value=%f", actualU);

            if (actualU > 0.001)
            { // 避免除以接近零的值
                if (isVoltage100PercentPoint)
                {
                    // 100%校准点更新
                    float ratio = uValue / actualU;
                    DA_Correct_100[u_idx][ur_idx] = DA_Correct_100[u_idx][ur_idx] * ratio;
                    printf(" (Updated 100%% calibration coefficient DA_Correct_100[%d][%d]=%f)",
                           u_idx, ur_idx, DA_Correct_100[u_idx][ur_idx]);
                }
                else if (isVoltage20PercentPoint)
                {
                    // 20%校准点更新
                    float ratio = uValue / actualU;
                    DA_Correct_20[u_idx][ur_idx] = DA_Correct_20[u_idx][ur_idx] * ratio;
                    printf(" (Updated 20%% calibration coefficient DA_Correct_20[%d][%d]=%f)",
                           u_idx, ur_idx, DA_Correct_20[u_idx][ur_idx]);
                }
            }
        }

        // 只在100%校准点和提供相位值时更新相位校准
        if (isVoltage100PercentPoint && actual_phu_json)
        {
            float actualPhU = (float)actual_phu_json->valuedouble;
            float phaseOffset = Phase_shift[u_idx] - actualPhU;
            DA_CorrectPhase_100[u_idx][ur_idx] = phaseOffset;
            printf(" Voltage phase=%f (Update phase calibration DA_CorrectPhase_100[%d][%d]=%f)",
                   actualPhU, u_idx, ur_idx, DA_CorrectPhase_100[u_idx][ur_idx]);
        }

        // 处理电流校准
        if (hasCurrentCalibration && actual_i_json)
        {
            float actualI = (float)actual_i_json->valuedouble;
            printf(" Current measurement value=%f", actualI);

            if (actualI > 0.001)
            { // 避免除以接近零的值
                if (isCurrent100PercentPoint)
                {
                    // 100%校准点更新
                    float ratio = iValue / actualI;
                    DA_Correct_100[i_idx][ir_idx] = DA_Correct_100[i_idx][ir_idx] * ratio;
                    printf(" (Updated 100%% calibration coefficient DA_Correct_100[%d][%d]=%f)",
                           i_idx, ir_idx, DA_Correct_100[i_idx][ir_idx]);
                }
                else if (isCurrent20PercentPoint)
                {
                    // 20%校准点更新
                    float ratio = iValue / actualI;
                    DA_Correct_20[i_idx][ir_idx] = DA_Correct_20[i_idx][ir_idx] * ratio;
                    printf(" (Updated 20%% calibration coefficient DA_Correct_20[%d][%d]=%f)",
                           i_idx, ir_idx, DA_Correct_20[i_idx][ir_idx]);
                }
            }
        }
        // 只在100%校准点和提供相位值时更新相位校准
        if (isCurrent100PercentPoint && actual_phi_json)
        {
            float actualPhI = (float)actual_phi_json->valuedouble;
            float phaseOffset = Phase_shift[i_idx] - actualPhI;
            DA_CorrectPhase_100[i_idx][ir_idx] = phaseOffset;
            printf(" Current phase=%f (Update phase calibration DA_CorrectPhase_100[%d][%d]=%f)",
                   actualPhI, i_idx, ir_idx, DA_CorrectPhase_100[i_idx][ir_idx]);
        }

        printf("\r\n");
    }

    // 应用更新后的校准参数
    // 更新波形幅度和范围
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = (float)(setACS.Vals[i].U / setACS.Vals[i].UR) * 100;
        Wave_Amplitude[i + 4] = (float)(setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }

    // 应用设置
    devState.bACMeterMode = 0; // 源模式
    devState.bACRunning = 1;   // AC运行
    devState.bClosedLoop = 0;  // 开环
    str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);
    power_amplifier_control(Wave_Amplitude, Wave_Range, devState.bClosedLoop == 1 ? PID_ON : PID_OFF, POWAMP_ON);
    // 打印成功标识
    printf("CPU1: WriteCalibrateAC: Calibration data updated successfully.\r\n");
    // 将校准参数保存到EEPROM
    // RC64_WriteCalibData();

    // 返回成功响应
    ReplyData replyData;
    strcpy(replyData.FunCode, "WriteCalibrateAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

// 生成JSON字符串并写入共享内存的函数
void write_reply_to_shared_memory(ReplyData *replyData)
{

    // 确保写入完成，使用内存屏障
    __sync_synchronize();
    // 创建JSON对象
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "FunType", "Reply");
    cJSON_AddStringToObject(json, "FunCode", replyData->FunCode);
    cJSON_AddStringToObject(json, "Result", replyData->Result);

    // 创建Data对象
    cJSON *data = cJSON_CreateObject();
    if (replyData->hasClosedLoop)
    {
        cJSON_AddBoolToObject(data, "ClosedLoop", replyData->ClosedLoop);
    }
    cJSON_AddItemToObject(json, "Data", data);

    // 将JSON对象转换为字符串
    char *jsonString = cJSON_PrintUnformatted(json);
    if (jsonString == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(json);
        return;
    }

    // 在字符串首尾加上'|'字符
    size_t jsonStringLength = strlen(jsonString);
    char *finalString = (char *)malloc(jsonStringLength + 3); // +3 是为首尾 '|' 和终止符 '\0'
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_free(jsonString);
        cJSON_Delete(json);
        return;
    }
    snprintf(finalString, jsonStringLength + 3, "|%s|", jsonString);

    // 将字符串写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //        xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, jsonString);
    }

    // 释放JSON字符串
    free(finalString);
    cJSON_free(jsonString);
    // 删除JSON对象
    cJSON_Delete(json);
}

/**
 * @brief 报告保护事件
 *
 * 根据输入的故障信息，生成并报告保护事件的JSON对象。
 *
 * @param ProectFault u8类型，包含故障信息的字节。每一位代表一个可能的故障，
 *                  0表示故障存在，1表示无故障。
 */
void report_protection_event(u8 ProectFault)
{
    // Only report if there is a fault (any bit is 0)
    if (ProectFault == 0xFF)
    {
        return; // No faults detected
    }

    // Create the JSON object for the protection event
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "Report");
    cJSON_AddStringToObject(report, "FunCode", "ProtectedEvent");

    // Create the Data object
    cJSON *data = cJSON_CreateObject();

    // Check for IOpen
    if ((ProectFault & 0x0F) != 0x0F)
    {
        cJSON *IOpen = cJSON_CreateArray();

        // Correct bit mapping: Bit 0 for IX, Bit 1 for IC, Bit 2 for IB, Bit 3 for IA
        for (int i = 0; i < 4; i++)
        {
            if ((ProectFault & (1 << i)) == 0) // Bit is 0 means fault
            {
                cJSON *fault = cJSON_CreateObject();
                cJSON_AddStringToObject(fault, "Type", "AC");
                cJSON_AddNumberToObject(fault, "Line", 1);

                int channelMap[4] = {4, 3, 2, 1}; // IX IC IB IA

                cJSON_AddNumberToObject(fault, "Chn", channelMap[i]);
                cJSON_AddItemToArray(IOpen, fault);
            }
        }

        if (cJSON_GetArraySize(IOpen) > 0)
        {
            cJSON_AddItemToObject(data, "IOpen", IOpen);
        }
        else
        {
            cJSON_Delete(IOpen);
        }
    }

    // Check for UShort
    if ((ProectFault & 0xF0) != 0xF0)
    {
        cJSON *UShort = cJSON_CreateArray();

        for (int i = 0; i < 4; i++)
        {
            if ((ProectFault & (1 << (i + 4))) == 0) // Bit is 0 means fault
            {
                cJSON *fault = cJSON_CreateObject();
                cJSON_AddStringToObject(fault, "Type", "AC");
                cJSON_AddNumberToObject(fault, "Line", 1);

                int channelMap[4] = {4, 3, 2, 1}; // UX UC UB UA
                cJSON_AddNumberToObject(fault, "Chn", channelMap[i]);
                cJSON_AddItemToArray(UShort, fault);
            }
        }

        if (cJSON_GetArraySize(UShort) > 0)
        {
            cJSON_AddItemToObject(data, "UShort", UShort);
        }
        else
        {
            cJSON_Delete(UShort);
        }
    }

    // Add the Data object to the report if it has children
    if (cJSON_GetObjectItem(data, "UShort") != NULL || cJSON_GetObjectItem(data, "IOpen") != NULL)
    {
        cJSON_AddItemToObject(report, "Data", data);
    }
    else
    {
        cJSON_Delete(data);
        cJSON_Delete(report);
        return; // No faults to report
    }

    // Convert the JSON to a string
    char *string = cJSON_Print(report);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(report);
        return;
    }

    // Add the pipe characters
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 for the two '|' and null terminator
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_Delete(report);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // Write to message queue
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1: Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        xil_printf("CPU1: Protection event reported successfully\n");
    }

    // 打印最终的JSON字符串用于测试
    // xil_printf("CPU1: Protection event JSON: %s\n", finalString);

    // 硬件操作
    //  关闭DA
    Wave_Frequency = 50;                                   // 频率
    memset(Phase_shift, 0, sizeof(Phase_shift));           // 清空基波相位
    enable = 0x00;                                         // 关闭通道输出
    memset(numHarmonics, 0, sizeof(numHarmonics));         // 清除谐波
    memset(harmonics, 0, sizeof(harmonics));               // 清除谐波幅值
    memset(harmonics_phases, 0, sizeof(harmonics_phases)); // 清除谐波相位
    str_wr_bram(PID_OFF);                                  // 生成交流信号

    // 关闭功放
    // 修改二级DA 波形幅度 量程
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = 0.0;
        Wave_Amplitude[i + 4] = 0.0;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }
    power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
    // 更改UDP结构体100DevState的运行状态
    devState.bACMeterMode = 0;
    devState.bACRunning = 0;

    // Clean up
    free(finalString);
    cJSON_Delete(report);
    free(string);
}

void write_command_to_shared_memory()
{
    // 1.1
    //	const char* command = "{"
    //			"\"FunType\": \"Cmd\","
    //			"\"FunCode\": \"GetFunCodeList\""
    //			"}";

    // 1.2
    //	const char* command = "{"
    //			"\"FunType\": \"Cmd\","
    //			"\"FunCode\": \"GetDevBaseInfo\""
    //			"}";

    // 1.3
    //	const char* command = "{"
    //			"\"FunType\": \"Cmd\","
    //			"\"FunCode\": \"GetDevState\""
    //			"}";

    // 1.5
    //	const char* command = "{"
    //			"\"FunType\": \"Cmd\","
    //			"\"FunCode\": \"SetReportEnable\","
    //			"\"Data\": {"
    //			"\"BaseDataAC\": true,"
    //			"\"HarmData\": true,"
    //			"\"InterHarmData\": false,"
    //			"\"BaseDataDC\": false,"
    //			"\"BaseDataIO\": false,"
    //			"\"DISOE\": false,"
    //			"\"VMData\": false"
    //			"}"
    //			"}";

    // 3.1.1
    //     const char* command = "{"
    //         "\"FunType\": \"Cmd\","
    //         "\"FunCode\": \"SetDCS\","
    //         "\"Data\": {"
    //             "\"ClosedLoop\": true,"
    //             "\"Chns\": ["
    //                 "{\"Chn\": 1, \"UR\": 57.7, \"U\": 57.7, \"URipple\": 0.01, \"IR\": 1.1, \"I\": 1.1, \"IRipple\": 0.01},"
    //                 "{\"Chn\": 2, \"UR\": 57.7, \"U\": 57.7, \"URipple\": 0.02, \"IR\": 2.2, \"I\": 2.2, \"IRipple\": 0.01},"
    //                 "{\"Chn\": 3, \"UR\": 57.7, \"U\": 57.7, \"URipple\": 0.03, \"IR\": 3.3, \"I\": 3.3, \"IRipple\": 0.01},"
    //                 "{\"Chn\": 4, \"UR\": 57.7, \"U\": 57.7, \"URipple\": 0.04, \"IR\": 4.4, \"I\": 4.4, \"IRipple\": 0.01}"
    //             "]"
    //         "}"
    //     "}";

    // 3.1.2
    //     const char* command = "{"
    //         "\"FunType\": \"Cmd\","
    //         "\"FunCode\": \"StopDCS\""
    //     "}";

    // 3.2.1
    //	    const char* command = "{"
    //	        "\"FunType\": \"Cmd\","
    //	        "\"FunCode\": \"SetDCM\","
    //	        "\"Data\": ["
    //	    		"{\"Chn\": 1, \"UR\": 100.0, \"IR\": 1.1},"
    //	    		"{\"Chn\": 2, \"UR\": 57.7,  \"IR\": 2.2},"
    //	    		"{\"Chn\": 3, \"UR\": 57.7,  \"IR\": 3.3},"
    //	    		"{\"Chn\": 4, \"UR\": 57.7,  \"IR\": 4.4}"
    //	        "]"
    //	    "}";

    // 3.3.1
    //     const char* command = "{"
    //         "\"FunType\": \"Cmd\","
    //         "\"FunCode\": \"SetACS\","
    //         "\"Data\": {"
    //             "\"ClosedLoop\": true,"
    //             "\"vals\": ["
    //                 "{\"Line\": 1, \"Chn\": 1, \"F\": 60.0, \"UR\": 57.7, \"U\": 57.7, \"PhU\": 0, \"IR\": 5.1, \"I\": 1.1, \"PhI\": 30.4},"
    //                 "{\"Line\": 1, \"Chn\": 2, \"F\": 60.0, \"UR\": 57.7, \"U\": 57.7, \"PhU\": 120, \"IR\": 5, \"I\": 2, \"PhI\": 120},"
    //                 "{\"Line\": 1, \"Chn\": 3, \"F\": 60.0, \"UR\": 57.7, \"U\": 57.7, \"PhU\": 240, \"IR\": 5, \"I\": 3, \"PhI\": 240},"
    //                 "{\"Line\": 1, \"Chn\": 4, \"F\": 60.0, \"UR\": 57.7, \"U\": 10, \"PhU\": 0, \"IR\": 5, \"I\": 4, \"PhI\": 0},"
    //                 "{\"Line\": 2, \"Chn\": 1, \"F\": 60.0, \"UR\": 57.7, \"U\": 57.7, \"PhU\": 0, \"IR\": 5, \"I\": 4, \"PhI\": 0},"
    //                 "{\"Line\": 2, \"Chn\": 2, \"F\": 60.0, \"UR\": 57.7, \"U\": 57.7, \"PhU\": 120, \"IR\": 5, \"I\": 3, \"PhI\": 120},"
    //                 "{\"Line\": 2, \"Chn\": 3, \"F\": 60.0, \"UR\": 57.7, \"U\": 57.7, \"PhU\": 240, \"IR\": 5, \"I\": 2, \"PhI\": 240},"
    //                 "{\"Line\": 2, \"Chn\": 4, \"F\": 60.0, \"UR\": 57.7, \"U\": 20, \"PhU\": 0, \"IR\": 5, \"I\": 1, \"PhI\": 0}"
    //             "]"
    //         "}"
    //     "}";

    // 3.3.2
    //     const char* command = "{"
    //         "\"FunType\": \"Cmd\","
    //         "\"FunCode\": \"SetACM\","
    //         "\"Data\": ["
    //     		"{\"Line\": 1, \"Chn\": 1, \"UR\": 100.0, \"IR\": 1.1},"
    //     		"{\"Line\": 1, \"Chn\": 2, \"UR\": 57.7,  \"IR\": 2.2},"
    //     		"{\"Line\": 1, \"Chn\": 3, \"UR\": 57.7,  \"IR\": 3.3},"
    //     		"{\"Line\": 1, \"Chn\": 4, \"UR\": 57.7,  \"IR\": 4.4},"
    //     		"{\"Line\": 2, \"Chn\": 1, \"UR\": 100.0, \"IR\": 1.1},"
    //     		"{\"Line\": 2, \"Chn\": 2, \"UR\": 57.7,  \"IR\": 2.2},"
    //     		"{\"Line\": 2, \"Chn\": 3, \"UR\": 57.7,  \"IR\": 3.3},"
    //     		"{\"Line\": 2, \"Chn\": 4, \"UR\": 57.7,  \"IR\": 4.4}"
    //         "]"
    //     "}";

    // 3.3.3
    //	const char* command = "{"
    //	    "\"FunType\": \"Cmd\","
    //	    "\"FunCode\": \"SetHarm\","
    //	    "\"Data\": ["
    //	        "{\"Line\": 1, \"Chn\": 1, \"HN\": 2, \"U\": 0.0, \"PhU\": 0.0, \"I\": 0.1, \"PhI\": 0},"
    //	        "{\"Line\": 1, \"Chn\": 1, \"HN\": 3, \"U\": 0.0, \"PhU\": 60.0, \"I\": 0.1, \"PhI\": 60.0},"
    //			"{\"Line\": 1, \"Chn\": 1, \"HN\": 4, \"U\": 0.0, \"PhU\": 120.0, \"I\": 0.1, \"PhI\": 120.0},"
    //			"{\"Line\": 1, \"Chn\": 1, \"HN\": 5, \"U\": 0.0, \"PhU\": 180.0, \"I\": 0.1, \"PhI\": 180.0},"
    //			"{\"Line\": 1, \"Chn\": 2, \"HN\": 2, \"U\": 0.1, \"PhU\": 0, \"I\": 0.1, \"PhI\": 0},"
    //			"{\"Line\": 1, \"Chn\": 2, \"HN\": 3, \"U\": 0.1, \"PhU\": 60.0, \"I\": 0.1, \"PhI\": 60.0},"
    //			"{\"Line\": 1, \"Chn\": 2, \"HN\": 4, \"U\": 0.1, \"PhU\": 120.0, \"I\": 0.1, \"PhI\": 120.0},"
    //			"{\"Line\": 1, \"Chn\": 2, \"HN\": 5, \"U\": 0.1, \"PhU\": 180.0, \"I\": 0.1, \"PhI\": 180.0},"
    //			"{\"Line\": 1, \"Chn\": 3, \"HN\": 2, \"U\": 0.1, \"PhU\": 0, \"I\": 0.1, \"PhI\": 0},"
    //			"{\"Line\": 1, \"Chn\": 3, \"HN\": 3, \"U\": 0.1, \"PhU\": 60.0, \"I\": 0.1, \"PhI\": 60.0},"
    //			"{\"Line\": 1, \"Chn\": 3, \"HN\": 4, \"U\": 0.1, \"PhU\": 120.0, \"I\": 0.1, \"PhI\": 120.0},"
    //			"{\"Line\": 1, \"Chn\": 3, \"HN\": 5, \"U\": 0.1, \"PhU\": 180.0, \"I\": 0.1, \"PhI\": 180.0},"
    //	        "{\"Line\": 1, \"Chn\": 4, \"HN\": 2, \"U\": 0.1, \"PhU\": 0, \"I\": 0.0, \"PhI\": 0},"
    //	        "{\"Line\": 1, \"Chn\": 4, \"HN\": 3, \"U\": 0.1, \"PhU\": 60.0, \"I\": 0.0, \"PhI\": 0.0},"
    //	        "{\"Line\": 1, \"Chn\": 4, \"HN\": 4, \"U\": 0.1, \"PhU\": 120.0, \"I\": 0.0, \"PhI\": 0.0},"
    //	        "{\"Line\": 1, \"Chn\": 4, \"HN\": 5, \"U\": 0.1, \"PhU\": 180.0, \"I\": 0.3, \"PhI\": 0.0}"
    //	    "]"
    //	"}";

    // 3.3.4
    //	const char* command = "{"
    //			"\"FunType\": \"Cmd\","
    //			"\"FunCode\": \"ClearHarm\""
    //			"}";

    // 3.3.7
    //	const char* command = "{"
    //			"\"FunType\": \"Cmd\","
    //			"\"FunCode\": \"StopAC\""
    //			"}";

    // 3.4
    //	const char* command = "{"
    //			"\"FunType\": \"Cmd\","
    //			"\"FunCode\": \"SetDO\","
    //			"\"Data\": ["
    //			"{\"Chn\": 1, \"val\": 0},"
    //			"{\"Chn\": 2, \"val\": 0},"
    //			"{\"Chn\": 3, \"val\": 0},"
    //			"{\"Chn\": 4, \"val\": 0},"
    //			"{\"Chn\": 5, \"val\": 1},"
    //			"{\"Chn\": 6, \"val\": 1},"
    //			"{\"Chn\": 7, \"val\": 1},"
    //			"{\"Chn\": 8, \"val\": 1}"
    //			"]"
    //			"}";

    // 将字符串写入共享内存

    xil_printf("Command written to shared memory.\n");
}

size_t calculate_dynamic_payload_size(ReportEnableStatus ReportStatus)
{
    size_t payload_size = 0;
    size_t header_size = sizeof(u32) * 2;

    if (ReportStatus.DevState)
    {
        payload_size += header_size + sizeof(DevState);
        //        printf("DevState size: %zu\n", header_size + sizeof(LineDevState));
    }
    else
    {
        payload_size += header_size;
        //        printf("DevState header only: %zu\n", header_size);
    }

    if (ReportStatus.BaseDataAC)
    {

        payload_size += header_size + sizeof(LineAC);
        //        printf("BaseDataAC size: %zu\n", header_size + sizeof(LineAC));
    }
    else
    {
        payload_size += header_size;
        //        printf("BaseDataAC header only: %zu\n", header_size);
    }

    if (ReportStatus.HarmData)
    {
        payload_size += header_size + sizeof(LineHarm);
        //        printf("HarmData size: %zu\n", header_size + sizeof(LineHarm));
    }
    else
    {
        payload_size += header_size;
        //        printf("HarmData header only: %zu\n", header_size);
    }

    if (ReportStatus.DI)
    {
        payload_size += header_size + sizeof(LineDI);
        //        printf("DI size: %zu\n", header_size + sizeof(LineDI));
    }
    else
    {
        payload_size += header_size;
        //        printf("DI header only: %zu\n", header_size);
    }

    if (ReportStatus.DO)
    {
        payload_size += header_size + sizeof(LineDO);
        //        printf("DO size: %zu\n", header_size + sizeof(LineDO));
    }
    else
    {
        payload_size += header_size;
        //        printf("DO header only: %zu\n", header_size);
    }

    if (ReportStatus.DISOE)
    {
        payload_size += header_size + sizeof(LineDisoe);
        //        printf("DISOE size: %zu\n", header_size + sizeof(LineDisoe));
    }
    else
    {
        payload_size += header_size;
        //        printf("DISOE header only: %zu\n", header_size);
    }

    //    printf("Total dynamic payload size: %zu\n", payload_size);
    return payload_size;
}

/*
 * 回报UDP结构体顶层函数
 */
// 初始化使能状态结构体
DevState devState;
LineAC lineAC;
LineHarm lineHarm;
LineDI lineDI;
LineDO lineDO;
LineDisoe lineDisoe;
void ReportUDP_Structure(ReportEnableStatus ReportStatus)
{
    UDPPacket udpPacket;

    // 动态计算 payload 大小
    size_t dynamic_payload_size = calculate_dynamic_payload_size(ReportStatus);

    /*写UDP结构体帧头区*/
    memcpy(udpPacket.syncHeader, (char[]){0xD1, 0xD2, 0xD3, 0xD4}, 4); // 同步头
    udpPacket.dataLength1 = dynamic_payload_size;                      // 数据区总长度
    udpPacket.dataLength2 = dynamic_payload_size;                      // 重复一次 数据区总长度
    udpPacket.versionInfo = 1;                                         // 版本信息
    memset(udpPacket.reserved, 0, 3);                                  // 保留

    /*填充数据区*/
    char *payload_ptr = udpPacket.payload;
    u32 structType;
    u32 structLength;

    // DevState 100
    Xil_DCacheFlushRange((INTPTR)&devState, sizeof(DevState));
    structType = DeviceState;
    structLength = ReportStatus.DevState ? sizeof(DevState) : 0;
    memcpy(payload_ptr, &structType, sizeof(structType));
    payload_ptr += sizeof(structType);
    memcpy(payload_ptr, &structLength, sizeof(structLength));
    payload_ptr += sizeof(structLength);
    if (ReportStatus.DevState)
    {
        memcpy(payload_ptr, &devState, sizeof(DevState));
        payload_ptr += sizeof(DevState);
    }

    // LineAC 101
    // Flush the entire lineAC structure
    Xil_DCacheFlushRange((INTPTR)&lineAC, sizeof(LineAC));
    structType = BaseDataAC;
    structLength = ReportStatus.BaseDataAC ? sizeof(LineAC) : 0;
    memcpy(payload_ptr, &structType, sizeof(structType));
    payload_ptr += sizeof(structType);
    memcpy(payload_ptr, &structLength, sizeof(structLength));
    payload_ptr += sizeof(structLength);
    if (ReportStatus.BaseDataAC)
    {
        memcpy(payload_ptr, &lineAC, sizeof(LineAC));
        payload_ptr += sizeof(LineAC);
    }

    // LineHarm
    Xil_DCacheFlushRange((UINTPTR)&lineHarm, sizeof(LineHarm));
    structType = HarmData;
    structLength = ReportStatus.HarmData ? sizeof(LineHarm) : 0;
    memcpy(payload_ptr, &structType, sizeof(structType));
    payload_ptr += sizeof(structType);
    memcpy(payload_ptr, &structLength, sizeof(structLength));
    payload_ptr += sizeof(structLength);
    if (ReportStatus.HarmData)
    {
        memcpy(payload_ptr, &lineHarm, sizeof(LineHarm));
        payload_ptr += sizeof(LineHarm);
    }

    // LineDI
    structType = DI;
    structLength = ReportStatus.DI ? sizeof(LineDI) : 0;
    memcpy(payload_ptr, &structType, sizeof(structType));
    payload_ptr += sizeof(structType);
    memcpy(payload_ptr, &structLength, sizeof(structLength));
    payload_ptr += sizeof(structLength);
    if (ReportStatus.DI)
    {
        memcpy(payload_ptr, &lineDI, sizeof(LineDI));
        payload_ptr += sizeof(LineDI);
    }

    // LineDO
    structType = DO;
    structLength = ReportStatus.DO ? sizeof(LineDO) : 0;
    memcpy(payload_ptr, &structType, sizeof(structType));
    payload_ptr += sizeof(structType);
    memcpy(payload_ptr, &structLength, sizeof(structLength));
    payload_ptr += sizeof(structLength);
    if (ReportStatus.DO)
    {
        memcpy(payload_ptr, &lineDO, sizeof(LineDO));
        payload_ptr += sizeof(LineDO);
    }

    // LineDisoe
    structType = DISOE;
    structLength = ReportStatus.DISOE ? sizeof(LineDisoe) : 0;
    memcpy(payload_ptr, &structType, sizeof(structType));
    payload_ptr += sizeof(structType);
    memcpy(payload_ptr, &structLength, sizeof(structLength));
    payload_ptr += sizeof(structLength);
    if (ReportStatus.DISOE)
    {
        memcpy(payload_ptr, &lineDisoe, sizeof(LineDisoe));
        payload_ptr += sizeof(LineDisoe);
    }

    /* 写UDP帧尾*/
    memcpy(payload_ptr, (char[]){0xE1, 0xE2, 0xE3, 0xE4}, 4); // 结束符
    payload_ptr += 4 * sizeof(u8);

    // 写 UDP 数据到共享内存
    // 设置共享内存最后一个字节为 1，标记占用
    uint32_t last_byte_addr = UDP_ADDRESS + UDP_MEM_SIZE - 4; // 共享内存最后一个字地址
    Xil_Out32(last_byte_addr, 1);
    Xil_DCacheFlushRange((INTPTR)last_byte_addr, sizeof(u32));

    // 写UDP到共享内存
    // 检查是否超出限制
    if (16 + dynamic_payload_size + 4 > UDP_MEM_SIZE)
    {
        printf("CPU1: ERROR - UDP data too large for shared memory: %zu > %d\n",
               16 + dynamic_payload_size + 4, UDP_MEM_SIZE);
        return; // 数据太大，直接返回不写入
    }

    write_UDP_to_shared_memory(UDP_ADDRESS, &udpPacket, 16 + dynamic_payload_size + 4);

    // 设置共享内存最后一个字节为 0，标记空闲
    Xil_Out32(last_byte_addr, 0);
    Xil_DCacheFlushRange((INTPTR)last_byte_addr, sizeof(u32));

    // 刷新整个UDP
    Xil_DCacheFlushRange((UINTPTR)&udpPacket, sizeof(udpPacket));
    // 打印调试信息
    // printf("lineAC size: %zu bytes\n", sizeof(LineAC));
    // printf("dynamic_payload_size: %zu bytes\n", dynamic_payload_size);
    // printf("UDP_PACKET_SIZE: %d bytes\n", sizeof(udpPacket));
    // printf("CPU1: UDP written to memory at: 0x%X\n", UDP_ADDRESS);
}

// Function to initialize DevState
void initDevState(DevState *devState)
{
    devState->bACMeterMode = 0; // 0=交流源状态;1=交流表状态
    devState->bACRunning = 0;   // 0=停止状态;1=运行状态    //默认停止
    devState->bClosedLoop = 1;  // 0=开环状态;1=闭环状态    //默认闭环
    devState->Reserved3 = 0;
    devState->Reserved4 = 0;
    devState->Reserved5 = 0;
    devState->Reserved6 = 0;
    devState->Reserved7 = 0;
    devState->Reserved8 = 0;
    devState->Reserved9 = 0;
    devState->Reserved10 = 0;
    devState->Reserved11 = 0;
    devState->Reserved12 = 0;
    devState->Reserved13 = 0;
    devState->Reserved14 = 0;
    devState->Reserved15 = 0;
}

// Function to initialize LineAC
void initLineAC(LineAC *lineAC)
{
    for (int i = 0; i < ChnsAC; i++)
    {
        lineAC->ur[i] = 6.5;
        lineAC->ir[i] = 5;
        lineAC->u[i] = 0;
        lineAC->i[i] = 0;
        lineAC->phu[i] = 0;
        lineAC->phi[i] = 0;
        lineAC->p[i] = 0.0;
        lineAC->q[i] = 0.0;
        lineAC->pf[i] = 0.0;
        lineAC->f[i] = 0.0;
        lineAC->thdu[i] = 0.0;
        lineAC->thdi[i] = 0.0;
    }
    lineAC->totalP = 0.0;
    lineAC->totalQ = 0.0;
    lineAC->totalPF = 0.0;
}

// Function to initialize LineHarm
void initLineHarm(LineHarm *lineHarm)
{
    for (int i = 0; i < ChnsAC; i++)
    {
        for (int j = 0; j < HarmNumberMax; j++)
        {
            lineHarm->harm[i].u[j] = 0.0;
            lineHarm->harm[i].i[j] = 0.0;
            lineHarm->harm[i].phu[j] = 0.0;
            lineHarm->harm[i].phi[j] = 0.0;
            lineHarm->harm[i].p[j] = 0.0;
            lineHarm->harm[i].q[j] = 0.0;
        }
        lineHarm->harm[i].totalP = 0.0;
        lineHarm->harm[i].totalQ = 0.0;
    }
}
void initLineDI(LineDI *lineDI)
{
    for (int i = 0; i < ChnsDI; i++)
    {
        lineDI->DI[i].v = 0;
    }
}

void initLineDO(LineDO *lineDO)
{
    for (int i = 0; i < ChnsDO; i++)
    {
        lineDO->DO[i].v = 0;
    }
}

void initLineDisoe(LineDisoe *lineDisoe)
{
    for (int i = 0; i < DisoeMsgNum; i++)
    {
        lineDisoe->DISOE[i].Chn = i;
        lineDisoe->DISOE[i].Val = 0;
        lineDisoe->DISOE[i].MS = 0;
        lineDisoe->DISOE[i].TIME = 0;
    }
}

// Function to write data to shared memory
void write_UDP_to_shared_memory(UINTPTR base_addr, void *data, size_t size)
{
    // 改用字节级别操作，避免对齐问题
    uint8_t *src = (uint8_t *)data;

    // 按字节写入，确保不会因为对齐问题导致数据错位
    for (size_t i = 0; i < size; i += 4)
    {
        // 每次写入4字节
        uint32_t value = 0;
        size_t bytes_to_copy = (i + 4 <= size) ? 4 : (size - i);

        // 按字节构建32位值
        for (size_t j = 0; j < bytes_to_copy; j++)
        {
            value |= ((uint32_t)src[i + j]) << (j * 8);
        }

        // 写入共享内存
        Xil_Out32(base_addr + i, value);
    }

    // 确保刷新整个缓存区域
    Xil_DCacheFlushRange((INTPTR)base_addr, size);
}

// 初始化setACS结构体
void init_setACS()
{
    // 遍历所有通道
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        // 设置线路编号和通道编号
        setACS.Vals[i].Line = i / ChnsAC + 1; // 线路编号从1开始
        setACS.Vals[i].Chn = i % ChnsAC + 1;  // 通道编号从1开始

        // 设置频率为50Hz
        setACS.Vals[i].F = 50.0f;

        // 设置电压相关参数
        setACS.Vals[i].UR = 6.5f;  // 电压档位
        setACS.Vals[i].U = 0.0f;   // 电压幅值
        setACS.Vals[i].PhU = 0.0f; // 电压相位

        // 设置电流相关参数
        setACS.Vals[i].IR = 5.0f;  // 电流档位
        setACS.Vals[i].I_ = 0.0f;  // 电流幅值
        setACS.Vals[i].PhI = 0.0f; // 电流相位
    }

    // 打印初始化完成消息
    // printf("CPU1: setACS.Vals initialization completed.\n");
}

void init_JsonUdp(void)
{
    //  初始化JSON结构体
    init_setACS();

    // 初始化数据结构体
    initDevState(&devState);
    initLineAC(&lineAC);
    initLineHarm(&lineHarm);
    initLineDI(&lineDI);
    initLineDO(&lineDO);
    initLineDisoe(&lineDisoe);
}
