
#include "Communications_Protocol.h"

/*
 *版本信息
 */
const char FPGA_Ver_Full[] = "[Ver]=V1.251205.1504";
const char ARM_Ver_Full[] = "[Ver]=V1.251210.1023";

volatile bool udp_data_changed_flag = true;              // 初始化为1，确保第一次会发送
volatile bool dac_parameters_updated_by_command = false; // JSon指令修改了参数

// --- Static global variables for Pause/Run state ---
uint8_t paused_bClosedLoop_state;
uint8_t target_powamp_enable_state_after_pause = POWAMP_OFF;

uint32_t g_do_output_state = 0; // 存储开出硬件状态
int g_harm_number_thd = 31;     //  THD谐波次数全局变量定义与初始化

// 为中断安全读取的“影子”变量提供定义和初始值。
volatile double g_safe_total_p_for_isr = 0.0;
volatile double g_safe_total_q_for_isr = 0.0;

// 定义全局变量以存储恒定模式的状态，默认为"Total"，总有效值恒定还是基波恒定
char g_constant_mode[16] = "Total";

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

        // printf("CPU1: Remove | \r\n");
    }
    else
    {
        printf("CPU1: NotFound | \r\n");
    }
}

/*
 * 解析JSON指令顶层函数
 */
int Parse_JsonCommand(char *buffer)
{
    // 更新指令映射表
    FunCodeMap funCodeMap[] = {
        {"GetFunCodeList", handle_GetFunCodeList},
        {"GetDevBaseInfo", handle_GetDevBaseInfo},
        {"GetDevState", handle_GetDevState},
        {"SetReportEnable", handle_SetReportEnable},

        // 源/表 设置
        {"SetDCS", handle_SetDCS},
        {"SetDCM", handle_SetDCM},
        {"SetACS", handle_SetACS},
        {"SetACM", handle_SetACM},

        // 谐波/间谐波
        {"SetHarm", handle_SetHarm},

        // 状态控制 (合并后)
        {"SetACStatus", handle_SetACStatus},
        // {"SetHarmStatus", handle_SetHarmStatus}, // 已合并到 SetACStatus
        // {"SetInharmStatus", handle_SetACStatus}, // 已合并到 SetACStatus

        // 统一参数设置 (替代 SetDI, SetPulseOut 等)
        {"SetDevState", handle_SetDevState},
        // {"SetDI", handle_SetDI},             // 已移除
        // {"SetPulseOut", handle_SetPulseOut}, // 已移除

        {"SetDO", handle_SetDO},
        {"StopDCS", handle_StopDCS},

        // 校准与事务
        {"SetCalibrateAC", handle_SetCalibrateAC},
        {"WriteCalibrateAC", handle_WriteCalibrateAC},
        {"RestoreCalibrateDefault", handle_RestoreCalibrateDefault},
        {"SetSysTimeSyncMode", handle_SetSysTimeSyncMode},
        {"SetTaskEnergyTest", handle_SetTaskEnergyTest},
        {"TerminateRunningTask", handle_TerminateRunningTask},
        {"StateSequence", handle_StateSequence}};

    const int funCodeMapSize = sizeof(funCodeMap) / sizeof(funCodeMap[0]);

    // 检查输入字符串首尾的 '|'
    extractContentBetweenPipes(buffer);

    // 打印buffer里的数据
    printf("CPU1: Recived_JSON: %s\r\n", buffer);

    // 使用cJSON解析JSON字符串
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        xil_printf("Failed to parse JSON.\r\n");
        return 1;
    }

    cJSON *funCodeItem = cJSON_GetObjectItem(json, "FunCode");
    if (funCodeItem == NULL)
    {
        xil_printf("No FunCode found in JSON.\r\n");
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
    const char *normalCmds[] = {
        "GetFunCodeList", "GetDevBaseInfo", "GetDevState", "SetServerReportEnable",
        "SetACS", "SetACStatus", "SetHarm", "SetHarmStatus", "SetInterHarm",
        "SetInterHarmStatus", "SetDCS", "StopDCS", "SetDCM", "SetACM",
        "SetDO"};
    const char *taskCmds[] = {
        "SetSysTimeSyncMode", "SetProgress", "SetTaskClockTest", "SetTaskEnergyTest",
        "SetTaskGradualChange", "WaveRecord", "WaveReplay", "StateSequence"};
    const char *reportListItems[] = {
        "ProtectEvent", "DISOE", "WaveRecordStart", "WaveRecordComplete"};

    const int structListItems[] = {
        DeviceState,
        BaseDataAC,
        HarmData,
        InterHarmData,
        BaseDataDCS,
        BaseDataDCM,
        DI,
        DO,
        InnerBattery,
        VMData};

    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "GetFunCodeList");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *dataObj = cJSON_CreateObject();
    cJSON *cmdListObj = cJSON_CreateObject();

    cJSON_AddItemToObject(cmdListObj, "Normal", cJSON_CreateStringArray(normalCmds, sizeof(normalCmds) / sizeof(char *)));
    cJSON_AddItemToObject(cmdListObj, "Task", cJSON_CreateStringArray(taskCmds, sizeof(taskCmds) / sizeof(char *)));
    cJSON_AddItemToObject(dataObj, "CmdList", cmdListObj);

    cJSON_AddItemToObject(dataObj, "ReportList", cJSON_CreateStringArray(reportListItems, sizeof(reportListItems) / sizeof(char *)));
    cJSON_AddItemToObject(dataObj, "StructList", cJSON_CreateIntArray(structListItems, sizeof(structListItems) / sizeof(int)));

    cJSON_AddItemToObject(reply, "Data", dataObj);

    // 将生成的 JSON 转换为字符串
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\r\n");
        cJSON_Delete(reply);
        return;
    }

    // 在字符串首尾加上 '|' 字符
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 为了首尾 '|' 和结束符
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\r\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // 将生成的 JSON 写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\r\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\r\n", bytesWritten, string);
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
    //    xil_printf("Handling handle_GetDevBaseInfo...\r\n");

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
    cJSON_AddItemToObject(di, "Resolution", diResolutions); // 修改节点名为 "Resolution"
    cJSON_AddItemToObject(dataObj, "DI", di);

    // DO 信息
    cJSON *doInfo = cJSON_CreateObject();
    cJSON_AddNumberToObject(doInfo, "ChnCount", ChnsBO);
    cJSON_AddItemToObject(dataObj, "DO", doInfo);

    // AC 信息
    cJSON *ac = cJSON_CreateObject();
    const char *acModes[] = {"S", "M"};
    cJSON_AddItemToObject(ac, "Mode", cJSON_CreateStringArray(acModes, 2));

    const char *calcMethods[] = {"Subgroup", "Group"};
    cJSON_AddItemToObject(ac, "HarmCalcMethod", cJSON_CreateStringArray(calcMethods, 2));
    const char *phaseRefs[] = {"FundUa", "FundChn"};
    cJSON_AddItemToObject(ac, "HarmPhaseRef", cJSON_CreateStringArray(phaseRefs, 2));
    cJSON_AddBoolToObject(ac, "InharmEnable", false); // 暂时不支持间谐波，填false

    cJSON_AddNumberToObject(ac, "HarmNumberMax", HarmNumberMax);
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
    cJSON_AddNumberToObject(ac, "HarmNumberTHD", g_harm_number_thd);

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

        double urValues[] = {6.5, 3.25, 1.876}; // 示例量程
        cJSON *urArray = cJSON_CreateDoubleArray(urValues, 3);
        cJSON_AddItemToObject(chnObj, "URs", urArray);

        double irValues[] = {5, 1, 0.2}; // 示例量程
        cJSON *irArray = cJSON_CreateDoubleArray(irValues, 3);
        cJSON_AddItemToObject(chnObj, "IRs", irArray);

        cJSON_AddItemToArray(dcmChns, chnObj);
    }
    cJSON_AddItemToObject(dcm, "Chns", dcmChns);
    cJSON_AddItemToObject(dataObj, "DCM", dcm);

    // 新增：EnergyPulse 信息
    cJSON *energyPulse = cJSON_CreateObject();
    cJSON_AddNumberToObject(energyPulse, "ChnCountIn", 2);  // 电能脉冲输入通道数
    cJSON_AddNumberToObject(energyPulse, "ChnCountOut", 2); // 电能脉冲输出通道数

    cJSON_AddItemToObject(dataObj, "EnergyPulse", energyPulse);

    // 新增：ClockPulse 信息 PPS信号
    cJSON *clockPulse = cJSON_CreateObject();
    cJSON_AddNumberToObject(clockPulse, "ChnCountIn", 2);  // 示例值
    cJSON_AddNumberToObject(clockPulse, "ChnCountOut", 2); // 示例值
    const char *clockPulseOutModes[] = {"Seconds", "Minutes"};
    cJSON *outModeArray = cJSON_CreateStringArray(clockPulseOutModes, sizeof(clockPulseOutModes) / sizeof(clockPulseOutModes[0]));
    cJSON_AddItemToObject(clockPulse, "OutMode", outModeArray);
    cJSON_AddItemToObject(dataObj, "ClockPulse", clockPulse);

    cJSON_AddItemToObject(reply, "Data", dataObj);

    // 将生成的 JSON 转换为字符串
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\r\n");
        cJSON_Delete(reply);
        return;
    }

    // 在字符串首尾加上 '|' 字符
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 为了首尾 '|' 和结束符
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\r\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // 将生成的 JSON 写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\r\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\r\n", bytesWritten, string);
    }

    // 清理
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

void handle_GetDevState(cJSON *data)
{
    // 处理 handle_GetDevState 的逻辑
    //	xil_printf("Handling handle_GetDevState...\r\n");

    // 创建上行指令的 JSON 对象
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "GetDevState");
    cJSON_AddStringToObject(reply, "Result", "Success");

    // 1.1获取当前时间
    In_CurrTime soft_time_read;
    read_current_time(&soft_time_read); // 从软时钟IP核读取
    char dev_time_str[40];
    // 中文注释: 格式化时间字符串为 "YYYY-MM-DD HH:MM:SS.ms"
    // 软时钟的亚秒单位是 10ns, 1ms = 100,000 * 10ns, 所以除以 100000 得到毫秒
    sprintf(dev_time_str, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            soft_time_read.curr_year, soft_time_read.curr_month, soft_time_read.curr_day,
            soft_time_read.curr_hour, soft_time_read.curr_minute, soft_time_read.curr_second,
            (unsigned int)(soft_time_read.curr_subsec / 100000));
    // 1.2 获取开入分辨率
    double di_resolution = g_debounce_time_ms; // 从 Amplifier_Switch 模块获取全局防抖时间(ms)

    cJSON *dataObj = cJSON_CreateObject();
    cJSON_AddStringToObject(dataObj, "DevTime", dev_time_str);
    cJSON_AddNumberToObject(dataObj, "Temperature", 24); // 示例温度

    // 1.3 根据对时管理器的当前模式决定回复的SyncMode字符串
    const char *sync_mode_str = "Manual"; // 默认值
    if (g_TimeSyncManager.current_mode == SYNC_MODE_GPS)
        sync_mode_str = "BD";
    else if (g_TimeSyncManager.current_mode == SYNC_MODE_IRIGB)
        sync_mode_str = "IRIG-B";
    cJSON_AddStringToObject(dataObj, "SyncMode", sync_mode_str); // 示例同步模式
    cJSON_AddNumberToObject(dataObj, "Longitude", (double)gpsx.longitude / 100000.0);
    cJSON_AddNumberToObject(dataObj, "Latitude", (double)gpsx.latitude / 100000.0);
    cJSON_AddNumberToObject(dataObj, "DI_Resolution", di_resolution); // 示例开入分辨率

    // 中文注释: 在回复中增加 ConstantMode(基波恒定还是有效值恒定)节点
    cJSON_AddStringToObject(dataObj, "ConstantMode", g_constant_mode);

    // todo 任务型还没有记录开始时间
    cJSON *task = cJSON_CreateObject();
    cJSON_AddStringToObject(task, "FunCode", ""); // 任务型指令运行状态
    cJSON_AddStringToObject(task, "StartTime", "2024-01-31 00:00:00.123");
    cJSON_AddItemToObject(dataObj, "Task", task);

    // [更新] 添加 HarmCalcMethod, HarmPhaseRef 属性 目前装置不支持，返回默认值
    cJSON_AddStringToObject(dataObj, "ConstantMode", g_constant_mode);

    // AC 信息
    cJSON *ac = cJSON_CreateObject();
    cJSON_AddStringToObject(ac, "Mode", "S");
    cJSON_AddBoolToObject(ac, "ClosedLoop", devState.bClosedLoop == 1 ? true : false);

    // [更新] AC节点下增加属性
    cJSON_AddStringToObject(ac, "HarmCalcMethod", "Subgroup"); // 默认
    cJSON_AddStringToObject(ac, "HarmPhaseRef", "FundUa");     // 默认
    cJSON_AddNumberToObject(ac, "HarmNumberTHD", g_harm_number_thd);

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
    // 中文注释: 在GetDevState回复的AC节点中增加HarmNumberTHD字段
    cJSON_AddNumberToObject(ac, "HarmNumberTHD", g_harm_number_thd);
    cJSON_AddItemToObject(dataObj, "AC", ac);

    // DCS 信息
    cJSON *dcs = cJSON_CreateArray();
    for (int chn = 1; chn <= ChnsDCS; chn++)
    {
        cJSON *chnObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(chnObj, "Chn", chn);
        cJSON_AddNumberToObject(chnObj, "UR", 0.0); // 示例：需要从实际DCS设置状态获取
        cJSON_AddNumberToObject(chnObj, "IR", 0.0);
        cJSON_AddItemToArray(dcs, chnObj);
    }
    cJSON_AddItemToObject(dataObj, "DCS", dcs);

    // DCM 信息
    cJSON *dcm = cJSON_CreateArray();
    for (int chn = 1; chn <= ChnsDCM; chn++)
    {
        cJSON *chnObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(chnObj, "Chn", chn);
        cJSON_AddNumberToObject(chnObj, "UR", 0.0); // 示例：需要从实际DCS设置状态获取
        cJSON_AddNumberToObject(chnObj, "IR", 0.0);
        cJSON_AddItemToArray(dcm, chnObj);
    }
    cJSON_AddItemToObject(dataObj, "DCM", dcm);

    // 根据全局配置获取当前DI/DO的有效位数
    int num_bits;
    switch (g_onoff_bit_width)
    {
    case bit_16:
        num_bits = 16;
        break;
    case bit_24:
        num_bits = 24;
        break;
    case bit_32:
        num_bits = 32;
        break;
    case bit_8:
    default:
        num_bits = 8;
        break;
    }
    // DO 信息
    cJSON *doInfo = cJSON_CreateArray();
    for (int i = 0; i < num_bits; i++) // 使用动态的num_bits
    {
        cJSON_AddItemToArray(doInfo, cJSON_CreateNumber(lineDO.DO[i].v));
    }
    cJSON_AddItemToObject(dataObj, "DO", doInfo);

    // DI 信息
    cJSON *diInfo = cJSON_CreateArray();
    for (int i = 0; i < num_bits; i++)
    {
        cJSON_AddItemToArray(diInfo, cJSON_CreateNumber(lineDI.DI[i].v));
    }
    cJSON_AddItemToObject(dataObj, "DI", diInfo);

    // 新增：EnergyPulse 信息
    cJSON *energyPulse = cJSON_CreateObject();
    cJSON_AddNumberToObject(energyPulse, "OutConstantP", g_PowerPulse.pulseConstantP); // 示例值
    cJSON_AddNumberToObject(energyPulse, "OutConstantQ", g_PowerPulse.pulseConstantQ); // 示例值
    cJSON_AddItemToObject(dataObj, "EnergyPulse", energyPulse);

    // 新增：ClockPulse 信息
    cJSON *clockPulse = cJSON_CreateObject();
    cJSON_AddStringToObject(clockPulse, "OutMode", "Seconds"); // 示例值，应从实际状态获取
    cJSON_AddItemToObject(dataObj, "ClockPulse", clockPulse);

    cJSON_AddItemToObject(reply, "Data", dataObj);
    // 将生成的 JSON 转换为字符串
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\r\n");
        cJSON_Delete(reply);
        return;
    }

    // 在字符串首尾加上 '|' 字符
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 为了首尾 '|' 和结束符
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\r\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // 将生成的 JSON 写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\r\n", bytesWritten);
    }

    // 清理
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

/**
 * @brief 处理 SetDevState 指令 (新增 20251115)
 * @details 替代了 SetDI, SetPulseOut，并包含了 HarmNumberTHD 和 ConstantMode
 */
void handle_SetDevState(cJSON *data)
{
    if (data == NULL)
        return;

    // 1. 解析 AC 部分
    cJSON *ac_item = cJSON_GetObjectItem(data, "AC");
    if (ac_item)
    {
        // ConstantMode
        cJSON *const_mode = cJSON_GetObjectItem(ac_item, "ConstantMode");
        if (const_mode && cJSON_IsString(const_mode))
        {
            if (strcmp(const_mode->valuestring, "Total") == 0 || strcmp(const_mode->valuestring, "Fund") == 0)
            {
                strncpy(g_constant_mode, const_mode->valuestring, sizeof(g_constant_mode) - 1);
                printf("CPU1: SetDevState -> ConstantMode: %s\r\n", g_constant_mode);
            }
        }

        // HarmNumberTHD
        cJSON *thd_item = cJSON_GetObjectItem(ac_item, "HarmNumberTHD");
        if (thd_item && cJSON_IsNumber(thd_item))
        {
            int val = thd_item->valueint;
            if (val >= 2 && val < HarmNumberMax)
            {
                g_harm_number_thd = val;
                printf("CPU1: SetDevState -> HarmNumberTHD: %d\r\n", g_harm_number_thd);
            }
        }

        // HarmCalcMethod 和 HarmPhaseRef (仅解析，暂不处理逻辑)
        cJSON *method = cJSON_GetObjectItem(ac_item, "HarmCalcMethod");
        if (method && cJSON_IsString(method))
        {
            printf("CPU1: SetDevState -> HarmCalcMethod: %s (Ignored)\r\n", method->valuestring);
        }
        cJSON *ref = cJSON_GetObjectItem(ac_item, "HarmPhaseRef");
        if (ref && cJSON_IsString(ref))
        {
            printf("CPU1: SetDevState -> HarmPhaseRef: %s (Ignored)\r\n", ref->valuestring);
        }
    }

    // 2. 解析 DI 部分 (原 SetDI)
    cJSON *di_item = cJSON_GetObjectItem(data, "DI");
    if (di_item)
    {
        cJSON *res_item = cJSON_GetObjectItem(di_item, "Resolution");
        if (res_item && cJSON_IsNumber(res_item))
        {
            double val = res_item->valuedouble;
            if (val >= 0.09 && val <= 100.01)
            {
                g_debounce_time_ms = val;
                printf("CPU1: SetDevState -> DI Resolution: %.1f ms\r\n", g_debounce_time_ms);
            }
        }
    }

    // 3. 解析 EnergyOut 部分 (原 SetPulseOut 部分)
    cJSON *energy_item = cJSON_GetObjectItem(data, "EnergyOut");
    if (energy_item)
    {
        cJSON *p_const = cJSON_GetObjectItem(energy_item, "PulseConstantP");
        if (p_const && cJSON_IsNumber(p_const))
        {
            g_PowerPulse.pulseConstantP = p_const->valueint;
            printf("CPU1: SetDevState -> PulseConstantP: %lu\r\n", g_PowerPulse.pulseConstantP);
        }
        cJSON *q_const = cJSON_GetObjectItem(energy_item, "PulseConstantQ");
        if (q_const && cJSON_IsNumber(q_const))
        {
            g_PowerPulse.pulseConstantQ = q_const->valueint;
            printf("CPU1: SetDevState -> PulseConstantQ: %lu\r\n", g_PowerPulse.pulseConstantQ);
        }
    }

    // 4. 解析 ClockOut 部分 (暂不支持，仅日志)
    cJSON *clock_item = cJSON_GetObjectItem(data, "ClockOut");
    if (clock_item)
    {
        cJSON *mode = cJSON_GetObjectItem(clock_item, "PulseMode");
        if (mode && cJSON_IsString(mode))
        {
            printf("CPU1: SetDevState -> ClockOut PulseMode: %s (Ignored)\r\n", mode->valuestring);
        }
    }

    // 回复 (回填当前状态)
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "SetDevState");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *dataObj = cJSON_CreateObject();
    // 构造 AC 回复
    cJSON *acObj = cJSON_CreateObject();
    cJSON_AddStringToObject(acObj, "ConstantMode", g_constant_mode);
    cJSON_AddStringToObject(acObj, "HarmCalcMethod", "Subgroup"); // 默认
    cJSON_AddStringToObject(acObj, "HarmPhaseRef", "FundUa");     // 默认
    cJSON_AddNumberToObject(acObj, "HarmNumberTHD", g_harm_number_thd);
    cJSON_AddItemToObject(dataObj, "AC", acObj);

    // 构造 DI 回复
    cJSON *diObj = cJSON_CreateObject();
    cJSON_AddNumberToObject(diObj, "Resolution", g_debounce_time_ms);
    cJSON_AddItemToObject(dataObj, "DI", diObj);

    // 构造 EnergyOut 回复
    cJSON *eoObj = cJSON_CreateObject();
    cJSON_AddNumberToObject(eoObj, "PulseConstantP", g_PowerPulse.pulseConstantP);
    cJSON_AddNumberToObject(eoObj, "PulseConstantQ", g_PowerPulse.pulseConstantQ);
    cJSON_AddItemToObject(dataObj, "EnergyOut", eoObj);

    // 构造 ClockOut 回复
    cJSON *coObj = cJSON_CreateObject();
    cJSON_AddStringToObject(coObj, "PulseMode", "Seconds");
    cJSON_AddItemToObject(dataObj, "ClockOut", coObj);

    cJSON_AddItemToObject(reply, "Data", dataObj);

    // 发送
    char *string = cJSON_PrintUnformatted(reply);
    if (string)
    {
        size_t len = strlen(string);
        char *finalStr = malloc(len + 3);
        if (finalStr)
        {
            snprintf(finalStr, len + 3, "|%s|", string);
            MsgQue_write(finalStr, strlen(finalStr));
            free(finalStr);
        }
        free(string);
    }
    cJSON_Delete(reply);
}

ReportEnableStatus reportStatus = {true, true, true, true, true, true, true, true, true, true};
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
    // xil_printf("Handling handle_SetReportEnable...\r\n");

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
        xil_printf("Failed to print JSON.\r\n");
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
        xil_printf("Failed to allocate memory for final JSON string.\r\n");
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
        xil_printf("CPU1:Failed to write to message queue: %ld\r\n", bytesWritten);
    }
    else
    {
        // 打印写入消息队列成功信息
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\r\n", bytesWritten, string);
    }

    // 清理
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

void handle_SetDCS(cJSON *data)
{
    // 处理 handle_SetDCS 的逻辑
    //    xil_printf("CPU1: Handling handle_SetDCS...\r\r\n");

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
        xil_printf("No Chns found in JSON.\r\n");
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
    // [新增] 抢占逻辑：如果有新的 AC 源指令，强制退出状态序列模式
    StateSequence_QuitMode();

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
                // 如果ur不为0
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
                // 如果ir不为0
                if (ir->valuedouble != 0)
                {
                    setACS.Vals[i].IR = (float)ir->valuedouble;
                    // printf("CPU1: Debug: Field 'IR' found in vals[%d].\r\n", i);
                }
            }

            // 获取 I 项 (注意 JSON 键名为 "I")
            cJSON *i_json = cJSON_GetObjectItem(val, "I");
            if (i_json && cJSON_IsNumber(i_json))
            {
                setACS.Vals[i].I_ = (float)i_json->valuedouble;
                onlyRangeFieldsFound = false; // 发现非量程字段 I
                // printf("CPU1: Debug: Field 'I' found in vals[%d].\r\n", i);
            }

            // 获取 PhI 项
            cJSON *phI = cJSON_GetObjectItem(val, "PhI");
            if (phI && cJSON_IsNumber(phI))
            {
                setACS.Vals[i].PhI = (float)phI->valuedouble;
                onlyRangeFieldsFound = false; // 发现非量程字段 PhI
                // printf("CPU1: Debug: Field 'PhI' found in vals[%d].\r\n", i);
            }
        }
    }
    else
    {
        // printf("CPU1: Debug: 'vals' array not found in JSON.\r\n");
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
        xil_printf("CPU1 Warning: Frequency out of range (%.2f Hz). Expected 45-65 Hz. Setting to 50 Hz.\r\n", Wave_Frequency);
        Wave_Frequency = 50.0f;
        // 同步修正 setACS 内部的值，以便下次读取时一致
        for (int i = 0; i < LinesAC * ChnsAC; i++)
        {
            setACS.Vals[i].F = 50.0f;
        }
    }

    // 更新相位、幅值、量程
    for (int i = 0; i < 4; i++)
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
                                      // xil_printf("CPU1 Warning: Voltage range UR for channel %d is zero or close to zero.\r\n", i);
        }

        if (fabs(setACS.Vals[i].IR) > 1e-6)
        {
            Wave_Amplitude[i + 4] = (setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100.0f;
        }
        else
        {
            Wave_Amplitude[i + 4] = 0.0f; // 如果档位为0，幅值百分比也为0
                                          // xil_printf("CPU1 Warning: Current range IR for channel %d is zero or close to zero.\r\n", i);
        }

        // 量程代码转换
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }

    // --- 新的量程切换/输出控制逻辑 ---
    if (valsPresent && onlyRangeFieldsFound) // 确保 vals 存在且只包含量程字段
    {
        // 判定为仅切换量程指令
        printf("CPU1: Range switching command detected. Disabling output.\r\n");
        enable = 0x00; // 关闭所有通道输出
        // str_wr_bram(PID_OFF);                                                     // 确保PID关闭 (即使不开环也要生成波形数据到 BRAM)
        // power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF); // 关闭功放

        // 更新设备状态 (UDP 结构体)
        devState.bACMeterMode = 0;                // 量程切换通常意味着处于源模式
        devState.nStatusFund = 0;                 // 切换量程时停止运行状态
        devState.bClosedLoop = setACS.ClosedLoop; // 保持 JSON 中指定的 (或之前的) 闭环状态

        // 清空UDP结构体
        initLineAC(&lineAC);
        initLineHarm(&lineHarm);
        for (int i = 0; i < 4; i++)
        {
            lineAC.ur[i] = setACS.Vals[i].UR;
            lineAC.ir[i] = setACS.Vals[i].IR;
        }
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
            // printf("CPU1: Normal ACS setting. Enabling output based on amplitude. Enable Mask: 0x%X\r\n", enable);
            // 根据 setACS.ClosedLoop 决定 PID 状态
            // PID_STATE pid_state = setACS.ClosedLoop ? PID_ON : PID_OFF;
            // str_wr_bram(pid_state);
            // power_amplifier_control(Wave_Amplitude, Wave_Range, pid_state, powamp_state);

            // 更新设备状态 (UDP 结构体)
            devState.bACMeterMode = 0; // 正常设置通常是源模式
            devState.nStatusFund = 1;  // 运行状态
            devState.bClosedLoop = setACS.ClosedLoop;
        }
        else
        {
            // 所有通道幅值都为 0 或指令无效/仅改变ClosedLoop且幅值为0
            printf("CPU1: Normal ACS setting but all amplitudes are zero. Disabling output.\r\n");
            enable = 0x00;
            // str_wr_bram(PID_OFF);
            // power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);

            // 更新设备状态 (UDP 结构体)
            devState.bACMeterMode = 0;
            devState.nStatusFund = 0; // 停止运行状态
            devState.bClosedLoop = setACS.ClosedLoop;

            // 清空UDP结构体
            initLineAC(&lineAC);
            initLineHarm(&lineHarm);
            for (int i = 0; i < 4; i++)
            {
                lineAC.ur[i] = setACS.Vals[i].UR;
                lineAC.ir[i] = setACS.Vals[i].IR;
            }
        }
    }
    udp_data_changed_flag = true;             // 更新UDP标志
    dac_parameters_updated_by_command = true; // 更新硬件操作标志
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
        printf("Line: %d, CHn: %d, UR: %.2f,IR: %.2f, \r\n",
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
    devState.nStatusFund = 1;

    udp_data_changed_flag = true;             // 更新UDP标志
    dac_parameters_updated_by_command = true; // 更新硬件操作标志
}

SetHarm setHarm;
/**
 * @brief 处理 SetHarm 请求
 */
void handle_SetHarm(cJSON *data)
{
    StateSequence_QuitMode();
    if (data == NULL)
    {
        printf("CPU1: SetHarm Error: Missing Data object.\r\n");
        return;
    }

    // 1. 处理 Reset
    cJSON *reset_item = cJSON_GetObjectItem(data, "Reset");
    bool reset_harmonics = false;
    if (reset_item != NULL && cJSON_IsTrue(reset_item))
    {
        reset_harmonics = true;
    }

    if (reset_harmonics)
    {
        printf("CPU1: SetHarm: Resetting all harmonic settings.\r\n");
        memset(numHarmonics, 0, sizeof(numHarmonics));
        memset(harmonics, 0, sizeof(harmonics));
        memset(harmonics_phases, 0, sizeof(harmonics_phases));
        memset(&setHarm, 0, sizeof(SetHarm));
        // 注意：不要清空 lineHarm，它是UDP回读用的，应该由主循环计算更新
    }

    // 2. 解析谐波数组 vals
    cJSON *vals_array = cJSON_GetObjectItem(data, "vals");
    if (vals_array != NULL && cJSON_IsArray(vals_array))
    {
        // int dataCount = cJSON_GetArraySize(vals_array);
        cJSON *item;

        cJSON_ArrayForEach(item, vals_array)
        {
            if (item == NULL)
                continue;

            // 获取 Key
            cJSON *line_item = cJSON_GetObjectItem(item, "Line");
            cJSON *chn_item = cJSON_GetObjectItem(item, "Chn");
            cJSON *hn_item = cJSON_GetObjectItem(item, "HN");

            if (!cJSON_IsNumber(line_item) || !cJSON_IsNumber(chn_item) || !cJSON_IsNumber(hn_item))
                continue;

            int line = line_item->valueint;
            int chn = chn_item->valueint;
            int hn = hn_item->valueint;

            // 获取 Value
            cJSON *u_item = cJSON_GetObjectItem(item, "U");
            cJSON *phU_item = cJSON_GetObjectItem(item, "PhU");
            cJSON *i_item = cJSON_GetObjectItem(item, "I");
            cJSON *phI_item = cJSON_GetObjectItem(item, "PhI");

            // 计算索引 (保留您原有的逻辑)
            int channelIndex = chn - 1;
            int harmonicIndex = hn - 2;

            if (channelIndex < 0 || channelIndex >= ChnsAC || harmonicIndex < 0 || harmonicIndex >= MAX_HARMONICS)
                continue;

            // 记录到 setHarm 结构体 (用于回读)
            int setHarmIndex = ((line - 1) * ChnsAC * HarmNumberMax) + ((chn - 1) * HarmNumberMax) + (hn - 1);
            if (setHarmIndex >= 0 && setHarmIndex < (LinesAC * ChnsAC * HarmNumberMax))
            {
                setHarm.Vals[setHarmIndex].Line = line;
                setHarm.Vals[setHarmIndex].Chn = chn;
                setHarm.Vals[setHarmIndex].HN = hn;
                if (u_item)
                    setHarm.Vals[setHarmIndex].U = (float)u_item->valuedouble;
                if (phU_item)
                    setHarm.Vals[setHarmIndex].PhU = (float)phU_item->valuedouble;
                if (i_item)
                    setHarm.Vals[setHarmIndex].I_ = (float)i_item->valuedouble;
                if (phI_item)
                    setHarm.Vals[setHarmIndex].PhI = (float)phI_item->valuedouble;
            }

            // 更新硬件数组
            if (u_item != NULL)
            {
                if (hn > numHarmonics[channelIndex])
                    numHarmonics[channelIndex] = hn;
                harmonics[channelIndex][harmonicIndex] = (float)u_item->valuedouble / 100.0f;
                if (phU_item)
                    harmonics_phases[channelIndex][harmonicIndex] = (float)phU_item->valuedouble;
            }

            int currChIdx = channelIndex + 4;
            if (i_item != NULL && currChIdx < CHANNL_MAX)
            {
                if (hn > numHarmonics[currChIdx])
                    numHarmonics[currChIdx] = hn;
                harmonics[currChIdx][harmonicIndex] = (float)i_item->valuedouble / 100.0f;
                if (phI_item)
                    harmonics_phases[currChIdx][harmonicIndex] = (float)phI_item->valuedouble;
            }
        }
    }

    // 3. 状态更新：设置完谐波参数后，自动将谐波状态置为运行
    devState.nStatusHarm = 1; // 【关键修改】使用 nStatusHarm

    // 4. 触发更新
    dac_parameters_updated_by_command = true;
    udp_data_changed_flag = true;

    // 5. 回复
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetHarm");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

SetDO setDO;
/**
 * @brief 处理 SetDO (开出设置) 请求
 * @param data 包含开出设置数据的 cJSON 对象
 */
void handle_SetDO(cJSON *data)
{
    cJSON *val_item = NULL;

    // 中文注释: 从Data对象中获取Chns数组，而不是直接将data视为数组
    cJSON *chns_array = cJSON_GetObjectItem(data, "Chns");

    // 检查Chns数组是否存在且有效
    if (!cJSON_IsArray(chns_array))
    {
        xil_printf("CPU1: SetDO Error: Data.Chns is not an array.\r\n");
        return;
    }

    // 根据全局配置获取当前DI/DO的有效位数
    int num_bits;
    switch (g_onoff_bit_width)
    {
    case bit_16:
        num_bits = 16;
        break;
    case bit_24:
        num_bits = 24;
        break;
    case bit_32:
        num_bits = 32;
        break;
    case bit_8:
    default:
        num_bits = 8;
        break;
    }

    // 遍历JSON数组中的每一个设置项
    cJSON_ArrayForEach(val_item, chns_array)
    {
        cJSON *chn_item = cJSON_GetObjectItem(val_item, "Chn");
        cJSON *val_num_item = cJSON_GetObjectItem(val_item, "val");

        if (chn_item && cJSON_IsNumber(chn_item) && val_num_item && cJSON_IsNumber(val_num_item))
        {
            int chn = chn_item->valueint;             // 获取通道号 (例如 1-8)
            bool val = (val_num_item->valueint != 0); // 获取设置值 (0 或 1)

            // 检查通道号是否在当前模式的有效范围内
            if (chn >= 1 && chn <= num_bits)
            {
                // 根据新的硬件观察结果，Chn:1 -> bit 24, Chn:8 -> bit 31
                // 公式为: bit_position = 23 + chn
                // 注意: 此公式目前仅根据8位模式的行为推断，如需扩展到16/24/32位，可能需要更复杂的映射
                int bit_position = 23 + chn;

                // 根据val的值来设置或清除相应的比特位
                if (val)
                {
                    g_do_output_state |= (1 << bit_position); // 设置位
                }
                else
                {
                    g_do_output_state &= ~(1 << bit_position); // 清除位
                }

                // 更新用于UDP回报的结构体，其索引仍为 chn-1
                lineDO.DO[chn - 1].v = val;
            }
            else
            {
                // 如果通道号超出范围，则打印警告
                xil_printf("CPU1: SetDO Warning: Channel %d is out of range for current %d-bit mode.\r\n", chn, num_bits);
            }
        }
    }

    // 将更新后的32位状态字写入硬件
    OnOff_Write_Continuous(g_do_output_state);

    // 打印日志，显示写入硬件的最终值
    printf("CPU1: SetDO: Wrote 0x%08lX to hardware DO register for %d-bit mode.\r\n", g_do_output_state, num_bits);

    // 准备并发送成功的回报
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetDO");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);

    // 标记UDP数据需要更新
    udp_data_changed_flag = true;
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

/**
 * @brief 辅助函数：处理状态字符串转数字
 * @return 0=Stop, 1=Run, 2=Pause, -1=Error
 */
static int parse_status_string(const char *str)
{
    if (!str)
        return -1;
    if (strcmp(str, "Stop") == 0)
        return 0;
    if (strcmp(str, "Run") == 0)
        return 1;
    if (strcmp(str, "Pause") == 0)
        return 2;
    return -1;
}

/**
 * @brief 辅助函数：状态数字转字符串
 */
static const char *get_status_string(int status)
{
    switch (status)
    {
    case 0:
        return "Stop";
    case 1:
        return "Run";
    case 2:
        return "Pause";
    default:
        return "Stop";
    }
}
/**
 * @brief 处理 SetACStatus 指令 (协议20251115: 合并了SetACStatus, SetHarmStatus, SetInharmStatus)
 * @details 独立控制 Fund(基波), Harm(谐波), Inharm(间谐波) 的状态 (0=Stop, 1=Run, 2=Pause)
 */
void handle_SetACStatus(cJSON *data)
{
    // [新增] 抢占逻辑
    StateSequence_QuitMode();

    if (data == NULL)
        return;

    // --------------------------------------------------------
    // 1. 处理基波 (Fund)
    // --------------------------------------------------------
    cJSON *fund_item = cJSON_GetObjectItem(data, "Fund");
    if (fund_item && cJSON_IsString(fund_item))
    {
        int status = parse_status_string(fund_item->valuestring); // 辅助函数: "Stop"->0, "Run"->1, "Pause"->2
        if (status != -1)
        {
            devState.nStatusFund = status; // 【修改】使用新的状态变量

            if (status == 0) // Stop
            {
                // 基波停止：清零幅值，复位频率和相位
                Wave_Frequency = 50.0f;
                memset(Phase_shift, 0, sizeof(Phase_shift));
                memset(Wave_Amplitude, 0, sizeof(Wave_Amplitude));
                // 注意：这里仅清除基波参数，保留谐波参数
            }
            printf("CPU1: SetACStatus -> Fund: %s (State=%d)\r\n", fund_item->valuestring, status);
        }
    }

    // --------------------------------------------------------
    // 2. 处理谐波 (Harm) - 【原 SetHarmStatus 的逻辑移到这里】
    // --------------------------------------------------------
    cJSON *harm_item = cJSON_GetObjectItem(data, "Harm");
    if (harm_item && cJSON_IsString(harm_item))
    {
        int status = parse_status_string(harm_item->valuestring);
        if (status != -1)
        {
            devState.nStatusHarm = status; // 【修改】使用新的状态变量

            if (status == 0) // Stop
            {
                // 谐波停止：清空谐波数组
                memset(numHarmonics, 0, sizeof(numHarmonics));
                memset(harmonics, 0, sizeof(harmonics));
                memset(harmonics_phases, 0, sizeof(harmonics_phases));
            }
            printf("CPU1: SetACStatus -> Harm: %s (State=%d)\r\n", harm_item->valuestring, status);
        }
    }

    // --------------------------------------------------------
    // 3. 处理间谐波 (Inharm)
    // --------------------------------------------------------
    cJSON *inharm_item = cJSON_GetObjectItem(data, "Inharm");
    if (inharm_item && cJSON_IsString(inharm_item))
    {
        int status = parse_status_string(inharm_item->valuestring);
        if (status != -1)
        {
            devState.nStatusInharm = status; // 【修改】使用新的状态变量

            if (status == 0) // Stop
            {
                // 间谐波停止逻辑 (预留)
            }
            printf("CPU1: SetACStatus -> Inharm: %s (State=%d)\r\n", inharm_item->valuestring, status);
        }
    }

    // 触发硬件更新 (主循环会检测此标志)
    dac_parameters_updated_by_command = true;
    udp_data_changed_flag = true;

    // 构建回复
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "SetACStatus");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *dataObj = cJSON_CreateObject();
    cJSON_AddStringToObject(dataObj, "Fund", get_status_string(devState.nStatusFund));
    cJSON_AddStringToObject(dataObj, "Harm", get_status_string(devState.nStatusHarm));
    cJSON_AddStringToObject(dataObj, "Inharm", get_status_string(devState.nStatusInharm));
    cJSON_AddItemToObject(reply, "Data", dataObj);

    char *string = cJSON_PrintUnformatted(reply);
    if (string)
    {
        size_t len = strlen(string);
        char *finalStr = malloc(len + 3);
        if (finalStr)
        {
            snprintf(finalStr, len + 3, "|%s|", string);
            MsgQue_write(finalStr, strlen(finalStr));
            free(finalStr);
        }
        free(string);
    }
    cJSON_Delete(reply);
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
        // 新增：强制设置校准时的基准相位
        // printf输出保持英文
        // printf("Setting phase for channel %d\n", setACS.Vals[i].Chn);
        switch (setACS.Vals[i].Chn)
        {
        case 1: // A相通道
            setACS.Vals[i].PhU = 0.0f;
            setACS.Vals[i].PhI = 0.0f;
            break;
        case 2: // B相通道
            setACS.Vals[i].PhU = 240.0f;
            setACS.Vals[i].PhI = 240.0f;
            break;
        case 3: // C相通道
            setACS.Vals[i].PhU = 120.0f;
            setACS.Vals[i].PhI = 120.0f;
            break;
        case 4: // X通道 (通常为零相)
        default:
            setACS.Vals[i].PhU = 0.0f;
            setACS.Vals[i].PhI = 0.0f;
            break;
        }
    }

    // 更新波形幅度和范围
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = (float)(setACS.Vals[i].U / setACS.Vals[i].UR) * 100;
        Wave_Amplitude[i + 4] = (float)(setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
        Phase_shift[i] = setACS.Vals[i].PhU;
        Phase_shift[i + 4] = setACS.Vals[i].PhI;
    }
    // 应用设置
    enable = 0xff;
    devState.bACMeterMode = 0;                     // 源模式
    devState.nStatusFund = 1;                      // AC运行
    devState.bClosedLoop = 0;                      // 开环
    memset(numHarmonics, 0, sizeof(numHarmonics)); // 清除谐波
    memset(harmonics, 0, sizeof(harmonics));
    memset(harmonics_phases, 0, sizeof(harmonics_phases));
    // str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);
    // power_amplifier_control(Wave_Amplitude, Wave_Range, devState.bClosedLoop == 1 ? PID_ON : PID_OFF, POWAMP_ON);

    // 返回成功响应
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetCalibrateAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);

    udp_data_changed_flag = true;             // 更新UDP标志
    dac_parameters_updated_by_command = true; // 更新硬件操作标志
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
    devState.bACMeterMode = 0;                     // 源模式
    devState.nStatusFund = 1;                      // AC运行
    devState.bClosedLoop = 0;                      // 开环
    memset(numHarmonics, 0, sizeof(numHarmonics)); // 清除谐波
    memset(harmonics, 0, sizeof(harmonics));
    memset(harmonics_phases, 0, sizeof(harmonics_phases));
    // str_wr_bram(devState.bClosedLoop == 1 ? PID_ON : PID_OFF);
    // power_amplifier_control(Wave_Amplitude, Wave_Range, devState.bClosedLoop == 1 ? PID_ON : PID_OFF, POWAMP_ON);
    // 将校准参数保存到EEPROM
    RC64_WriteCalibData();

    // 返回成功响应
    ReplyData replyData;
    strcpy(replyData.FunCode, "WriteCalibrateAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);

    udp_data_changed_flag = true;             // 更新UDP标志
    dac_parameters_updated_by_command = true; // 更新硬件操作标志
}

/**
 * @brief 处理恢复校准默认值的请求
 *
 * 该函数解析传入的JSON对象，根据其中的Type和AllRange字段执行相应的恢复逻辑，
 * 如果参数被实际修改，则将恢复的校准数据写回EEPROM，并准备和发送上行回复JSON。
 *
 * @param data_json_obj 传入的JSON对象，包含恢复校准所需的Type和AllRange字段
 */
void handle_RestoreCalibrateDefault(cJSON *data_json_obj)
{
    char *result_status = "Success";           // 初始化结果状态为成功
    const char *requested_type_str = "";       // 用于回复中反映请求的Type
    bool requested_all_range = false;          // 用于回复中反映请求的AllRange
    bool calibration_actually_changed = false; // 标记是否有校准参数被实际修改

    // 1. 解析下行指令中的Data对象
    if (!data_json_obj || !cJSON_IsObject(data_json_obj))
    {
        xil_printf("CPU1: RestoreCalibrateDefault: Data field is missing or not an object.\r\n"); // 中文注释：Data字段缺失或不是一个对象。
        result_status = "Failure";
        goto send_reply_restore;
    }

    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(data_json_obj, "Type");
    cJSON *all_range_item = cJSON_GetObjectItemCaseSensitive(data_json_obj, "AllRange");

    if (!cJSON_IsString(type_item) || (type_item->valuestring == NULL))
    {
        xil_printf("CPU1: RestoreCalibrateDefault: 'Type' field is missing or not a string.\r\n"); // 中文注释：'Type'字段缺失或不是字符串。
        result_status = "Failure";
        if (all_range_item && cJSON_IsBool(all_range_item))
            requested_all_range = cJSON_IsTrue(all_range_item); // 尝试填充AllRange用于回复
        goto send_reply_restore;
    }
    requested_type_str = type_item->valuestring;

    if (!all_range_item || !cJSON_IsBool(all_range_item))
    {
        xil_printf("CPU1: RestoreCalibrateDefault: 'AllRange' field is missing or not a boolean.\r\n"); // 中文注释：'AllRange'字段缺失或不是布尔值。
        result_status = "Failure";
        // requested_type_str 已经获取
        goto send_reply_restore;
    }
    requested_all_range = cJSON_IsTrue(all_range_item);

    xil_printf("CPU1: RestoreCalibrateDefault: Request to restore Type='%s', AllRange=%s.\r\n", requested_type_str, requested_all_range ? "true" : "false"); // 中文注释：请求恢复校准参数，类型='%s'，所有量程=%s。

    // 2. 根据Type和AllRange执行恢复逻辑
    if (strcmp(requested_type_str, "AC") == 0 || strcmp(requested_type_str, "ALL") == 0)
    {
        // printf("CPU1: Processing AC calibration restore.\r\n"); //中文注释：处理交流校准恢复。
        if (requested_all_range)
        {
            // 恢复AC类型的所有量程
            printf("CPU1: Restoring all ranges for AC.\r\n"); // 中文注释：恢复交流所有量程。
            memcpy(DA_Correct_100, DA_CorrectConst_100, sizeof(DA_CorrectConst_100));
            memcpy(DA_Correct_20, DA_CorrectConst_20, sizeof(DA_CorrectConst_20));
            memcpy(DA_CorrectPhase_100, DA_CorrectPhaseConst_100, sizeof(DA_CorrectPhaseConst_100));
            memcpy(AD_Correct, ADConst_Correct, sizeof(ADConst_Correct));
            calibration_actually_changed = true;
        }
        else
        {
            // 恢复AC类型的当前量程
            // printf("CPU1: Restoring current range for AC.\r\n"); //中文注释：恢复交流当前量程。
            for (int ch = 0; ch < ROWS; ++ch) // ROWS 通常是8个通道
            {
                int range_idx = -1;
                bool current_channel_changed = false;

                if (ch < 4)
                { // 电压通道 UA, UB, UC, UX (对应 setACS.Vals[ch])
                    // 使用setACS中存储的当前电压档位值来获取档位索引
                    range_idx = get_voltage_index_by_value(setACS.Vals[ch].UR);
                    if (range_idx != -1)
                    {                                                                                                                   // 确保获取到有效的量程索引
                        printf("CPU1: Restoring ch %d (Voltage), range_idx %d based on UR: %f\r\n", ch, range_idx, setACS.Vals[ch].UR); // 中文注释：恢复通道 %d (电压)，量程索引 %d，基于 UR: %f
                        DA_Correct_100[ch][range_idx] = DA_CorrectConst_100[ch][range_idx];
                        DA_Correct_20[ch][range_idx] = DA_CorrectConst_20[ch][range_idx];
                        DA_CorrectPhase_100[ch][range_idx] = DA_CorrectPhaseConst_100[ch][range_idx];
                        AD_Correct[ch][range_idx] = ADConst_Correct[ch][range_idx];
                        current_channel_changed = true;
                    }
                    else
                    {
                        printf("CPU1: Warning: Could not determine current voltage range index for channel %d (UR: %f).\r\n", ch, setACS.Vals[ch].UR); // 中文注释：警告：无法确定通道 %d 的当前电压量程索引 (UR: %f)。
                    }
                }
                else
                { // 电流通道 IA, IB, IC, IX (对应 setACS.Vals[ch-4])
                    // 使用setACS中存储的当前电流量程值来获取档位索引
                    range_idx = get_current_index_by_value(setACS.Vals[ch - 4].IR);
                    if (range_idx != -1)
                    {
                        printf("CPU1: Restoring ch %d (Current), range_idx %d based on IR: %f\r\n", ch, range_idx, setACS.Vals[ch - 4].IR); // 中文注释：恢复通道 %d (电流)，量程索引 %d，基于 IR: %f
                        DA_Correct_100[ch][range_idx] = DA_CorrectConst_100[ch][range_idx];
                        DA_Correct_20[ch][range_idx] = DA_CorrectConst_20[ch][range_idx];
                        DA_CorrectPhase_100[ch][range_idx] = DA_CorrectPhaseConst_100[ch][range_idx];
                        AD_Correct[ch][range_idx] = ADConst_Correct[ch][range_idx];
                        current_channel_changed = true;
                    }
                    else
                    {
                        printf("CPU1: Warning: Could not determine current current range index for channel %d (IR: %f).\r\n", ch, setACS.Vals[ch - 4].IR); // 中文注释：警告：无法确定通道 %d 的当前电流量程索引 (IR: %f)。
                    }
                }
                if (current_channel_changed)
                {
                    calibration_actually_changed = true;
                }
            }
        }
    }

    // TODO: 实现 DCS 和 DCM 的恢复逻辑 (如果它们有校准参数和出厂默认值)
    if (strcmp(requested_type_str, "DCS") == 0 || strcmp(requested_type_str, "ALL") == 0)
    {
        xil_printf("CPU1: Restoring DCS calibration data (NotImplementedYet).\r\n"); // 中文注释：恢复直流源校准数据 (尚未实现)。
        // 示例： if (requested_all_range) { memcpy(DCS_Calib, DCS_CalibConst, sizeof(DCS_CalibConst)); calibration_actually_changed = true;}
        // else { // 恢复DCS当前量程... calibration_actually_changed = true; }
    }
    if (strcmp(requested_type_str, "DCM") == 0 || strcmp(requested_type_str, "ALL") == 0)
    {
        xil_printf("CPU1: Restoring DCM calibration data (NotImplementedYet).\r\n"); // 中文注释：恢复直流表校准数据 (尚未实现)。
    }

    // 检查是否是未知类型
    if (strcmp(requested_type_str, "AC") != 0 && strcmp(requested_type_str, "DCS") != 0 &&
        strcmp(requested_type_str, "DCM") != 0 && strcmp(requested_type_str, "ALL") != 0)
    {
        xil_printf("CPU1: RestoreCalibrateDefault: Unknown Type '%s'.\r\n", requested_type_str); // 中文注释：未知的类型 '%s'。
        result_status = "Failure";
    }
    else if (!calibration_actually_changed && strcmp(result_status, "Success") == 0)
    {
        // 如果请求的类型是已知的，但没有实际参数被更改
        // (例如，请求恢复当前量程，但当前量程的校准参数已经是默认值，或者无法确定当前量程索引)
        xil_printf("CPU1: RestoreCalibrateDefault: No calibration parameters were effectively changed for Type '%s', AllRange '%s'.\r\n", requested_type_str, requested_all_range ? "true" : "false"); // 中文注释：对于类型 '%s'，所有量程 '%s'，没有校准参数被有效更改。
        // 这种情况通常也算Success，表示指令已执行但无效果。
    }

    // 3. 如果有参数被修改且之前的操作都是成功的，则写回EEPROM
    if (calibration_actually_changed && strcmp(result_status, "Success") == 0)
    {
        // xil_printf("CPU1: Writing restored default calibration data to EEPROM...\r\n"); //中文注释：将恢复的默认校准数据写入EEPROM...
        if (RC64_WriteCalibData() != XST_SUCCESS)
        {
            xil_printf("CPU1: RestoreCalibrateDefault: Failed to write restored calibration data to EEPROM.\r\n"); // 中文注释：将恢复的校准数据写入EEPROM失败。
            result_status = "Failure";
        }
        else
        {
            xil_printf("CPU1: Restored calibration data for Type '%s', AllRange '%s' successfully written to EEPROM.\r\n", requested_type_str, requested_all_range ? "true" : "false"); // 中文注释：类型 '%s'，所有量程 '%s' 的恢复校准数据成功写入EEPROM。
        }
    }

send_reply_restore:;
    // 4. 准备并发送上行回复JSON
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "RestoreCalibrateDefault");
    cJSON_AddStringToObject(reply, "Result", result_status);

    cJSON *reply_data_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(reply_data_obj, "Type", requested_type_str);    // 回复中包含请求的Type
    cJSON_AddBoolToObject(reply_data_obj, "AllRange", requested_all_range); // 回复中包含请求的AllRange
    cJSON_AddItemToObject(reply, "Data", reply_data_obj);

    char *string_to_send = cJSON_PrintUnformatted(reply);
    if (string_to_send == NULL)
    {
        xil_printf("CPU1: RestoreCalibrateDefault: Failed to print JSON reply.\r\n"); // 中文注释：生成JSON回复失败。
        cJSON_Delete(reply);
        return;
    }

    size_t stringLength = strlen(string_to_send);
    char *finalString = (char *)malloc(stringLength + 3); // 为首尾 '|' 和结束符分配空间
    if (finalString == NULL)
    {
        xil_printf("CPU1: RestoreCalibrateDefault: Failed to allocate memory for final JSON string.\r\n"); // 中文注释：为最终JSON字符串分配内存失败。
        free(string_to_send);
        cJSON_Delete(reply);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string_to_send);

    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1: RestoreCalibrateDefault: Failed to write reply to message queue: %ld\r\n", bytesWritten); // 中文注释：将回复写入消息队列失败。
    }
    else
    {
        // xil_printf("CPU1: RestoreCalibrateDefault: Successfully wrote reply: %s\r\n", finalString); //中文注释：成功写入回复。
    }

    free(finalString);
    free(string_to_send);
    cJSON_Delete(reply);
}

void handle_SetSysTimeSyncMode(cJSON *data)
{
    char *result_str = "Failure";
    char *current_mode_str = "Unknown";
    SyncModeType mode_to_start = SYNC_MODE_NONE;

    if (!data || !cJSON_IsObject(data))
    {
        xil_printf("CPU1: SetSysTimeSyncMode: Data field is missing or not an object.\r\n");
        goto send_reply;
    }

    cJSON *sync_mode_item = cJSON_GetObjectItem(data, "SyncMode");
    if (!sync_mode_item || !cJSON_IsString(sync_mode_item))
    {
        xil_printf("CPU1: SetSysTimeSyncMode: SyncMode is missing or not a string.\r\n");
        goto send_reply;
    }

    const char *requested_mode = sync_mode_item->valuestring;
    current_mode_str = (char *)requested_mode; // 用于回复

    if (strcmp(requested_mode, "BD") == 0)
    {
        mode_to_start = SYNC_MODE_GPS;
    }
    else if (strcmp(requested_mode, "IRIG-B") == 0)
    {
        mode_to_start = SYNC_MODE_IRIGB;
    }
    else if (strcmp(requested_mode, "Manual") == 0)
    {
        mode_to_start = SYNC_MODE_MANUAL;
    }
    else if (strcmp(requested_mode, "SNTP") == 0)
    {
        mode_to_start = SYNC_MODE_SNTP;
    }
    else
    {
        xil_printf("CPU1: SetSysTimeSyncMode: Unknown SyncMode value '%s'.\r\n", requested_mode);
        goto send_reply;
    }

    if (StartSystemSync(mode_to_start, data) == 0)
    {
        // 对于异步任务，回复 "Doing"
        if (mode_to_start == SYNC_MODE_GPS || mode_to_start == SYNC_MODE_IRIGB)
        {
            result_str = "Doing";
        }
        else
        { // 对于同步任务 (Manual), 成功就是成功
            result_str = "Success";
        }
    }
    else
    {
        // StartSystemSync 失败 (例如，任务已在进行中)
        result_str = "Failure";
    }

send_reply:;
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "SetSysTimeSyncMode");
    cJSON_AddStringToObject(reply, "Result", result_str);

    cJSON *reply_data = cJSON_CreateObject();
    cJSON_AddStringToObject(reply_data, "SyncMode", current_mode_str);
    cJSON_AddItemToObject(reply, "Data", reply_data);

    char *string = cJSON_PrintUnformatted(reply);
    if (string)
    {
        size_t stringLength = strlen(string);
        char *finalString = (char *)malloc(stringLength + 3);
        if (finalString)
        {
            snprintf(finalString, stringLength + 3, "|%s|", string);
            MsgQue_write(finalString, strlen(finalString));
            free(finalString);
        }
        free(string);
    }
    cJSON_Delete(reply);
}

/**
 * @brief (已重构) 检查并上报挂起的电能误差测试状态
 * @details 此函数应在主循环中被周期性调用。
 * 它检查全局报告标志位 (report_pending)，如果为true，则根据 "信箱" 中的数据快照
 * 打包并发送JSON报告，然后清除标志位。
 */
void check_and_report_energy_test_status(void)
{
    // 中文注释: 检查是否有挂起的报告任务。这是一个非阻塞的快速检查。
    if (!g_energy_report_data.report_pending)
    {
        return;
    }

    // 中文注释: 打印调试信息，表明主循环已捕获到报告请求
    // printf("CPU1: [DEBUG] Main loop is processing a pending energy test report.\n");

    // --- 开始构建上报的JSON对象 ---
    cJSON *report = cJSON_CreateObject();
    cJSON_AddStringToObject(report, "FunType", "TaskEvent");
    cJSON_AddStringToObject(report, "FunCode", "SetTaskEnergyTest");
    cJSON_AddStringToObject(report, "Result", g_energy_report_data.result);

    // 中文注释: 创建Data对象，它将包含Chns数组
    cJSON *data_obj = cJSON_CreateObject();
    cJSON *chns_array = cJSON_CreateArray();

    // 中文注释: 检查是否是最终的成功报告，这会影响我们如何判断一个通道是否应该被包含
    bool is_final_report = (strcmp(g_energy_report_data.result, "Success") == 0);
    printf("CPU1: [DEBUG] Energy test result is %s\n", g_energy_report_data.result);

    // --- 动态构建Data数组 ---
    // 中文注释: 遍历所有测试通道的快照
    for (int i = 0; i < NUM_ENERGY_CHANNELS; i++)
    {
        // 中文注释: 上报条件: 测试仍在进行中或这是最终报告且该通道参与了测试
        if (g_energy_report_data.test_snapshot[i].isActive || (is_final_report && g_energy_report_data.test_snapshot[i].currentTestNum > 0))
        {
            cJSON *chn_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(chn_obj, "Chn", i + 1); // 通道号是 1-based
            cJSON_AddNumberToObject(chn_obj, "Round", g_energy_report_data.test_snapshot[i].currentPulseCount);
            cJSON_AddNumberToObject(chn_obj, "TestedTimes", g_energy_report_data.test_snapshot[i].currentTestNum);

            cJSON *errs_array = cJSON_CreateArray();
            for (int j = 0; j < g_energy_report_data.test_snapshot[i].currentTestNum; j++)
            {
                cJSON_AddItemToArray(errs_array, cJSON_CreateNumber(g_energy_report_data.test_snapshot[i].errs[j]));
            }
            cJSON_AddItemToObject(chn_obj, "Errs", errs_array);
            cJSON_AddItemToArray(chns_array, chn_obj);
        }
    }

    cJSON_AddItemToObject(data_obj, "Chns", chns_array); // 中文注释: 将Chns数组添加到Data对象
    cJSON_AddItemToObject(report, "Data", data_obj);     // 中文注释: 将Data对象添加到主报告

    // --- JSON字符串转换与发送 ---
    char *string_to_send = cJSON_PrintUnformatted(report);
    if (string_to_send)
    {
        // 中文注释: 打印将要发送的JSON内容，用于调试
        // printf("CPU1: report energy_test : |%s|\n", string_to_send);

        size_t stringLength = strlen(string_to_send);
        char *finalString = (char *)malloc(stringLength + 3);
        if (finalString)
        {
            snprintf(finalString, stringLength + 3, "|%s|", string_to_send);
            MsgQue_write(finalString, strlen(finalString));
            free(finalString);
        }
        free(string_to_send);
    }
    else
    {
        printf("CPU1: Error: Failed to print energy test report JSON.\n");
    }

    cJSON_Delete(report);

    // --- 关键步骤: 清除标志位，防止重复发送 ---
    g_energy_report_data.report_pending = false;
}

/**
 * @brief 处理 "SetTaskEnergyTest" JSON指令 (已重构)
 * @param data 指向JSON中"Data"字段的cJSON对象
 */
void handle_SetTaskEnergyTest(cJSON *data)
{
    char *result = "Failure"; // 中文注释: 默认回复为失败
    bool is_any_task_started = false;

    if (!data)
    {
        printf("CPU1: SetTaskEnergyTest Error: Missing 'Data' object.\n");
        goto send_reply_energy_test;
    }

    // 中文注释: 1. 解析 TestMode
    cJSON *test_mode_item = cJSON_GetObjectItem(data, "TestMode");
    if (!cJSON_IsString(test_mode_item) || (test_mode_item->valuestring == NULL) || (strlen(test_mode_item->valuestring) != 1))
    {
        printf("CPU1: SetTaskEnergyTest Error: Missing or invalid 'TestMode' parameter.\n");
        goto send_reply_energy_test;
    }
    char test_mode = test_mode_item->valuestring[0];
    if (test_mode != 'P' && test_mode != 'Q')
    {
        printf("CPU1: SetTaskEnergyTest Error: 'TestMode' must be 'P' or 'Q'.\n");
        goto send_reply_energy_test;
    }

    // --- 2. 解析通用测试参数 ---
    cJSON *pulse_const_item = cJSON_GetObjectItem(data, "PulseConstant");
    cJSON *freq_div_item = cJSON_GetObjectItem(data, "FreqDivFactor");
    cJSON *round_item = cJSON_GetObjectItem(data, "Round");
    cJSON *test_times_item = cJSON_GetObjectItem(data, "TestTimes");

    // 中文注释: 校验核心参数是否存在且为数字
    if (!cJSON_IsNumber(pulse_const_item) || !cJSON_IsNumber(freq_div_item) || !cJSON_IsNumber(round_item))
    {
        printf("CPU1: SetTaskEnergyTest Error: Missing or invalid core parameters (PulseConstant, FreqDivFactor, Round).\n");
        goto send_reply_energy_test;
    }

    // 中文注释: 提取并校验数值
    uint32_t pulse_constant = pulse_const_item->valueint;
    uint32_t freq_div_factor = freq_div_item->valueint;
    uint32_t target_rounds = round_item->valueint;
    uint32_t target_times = cJSON_IsNumber(test_times_item) ? test_times_item->valueint : 1;

    if (pulse_constant == 0 || freq_div_factor == 0 || target_rounds == 0 || target_times == 0 || target_times > 99)
    {
        printf("CPU1: SetTaskEnergyTest Error: Numerical parameters cannot be zero or out of range.\n");
        goto send_reply_energy_test;
    }

    // --- 3. 解析 "Chns" 节点并调用辅助函数启动相应测试 ---
    cJSON *chns_array = cJSON_GetObjectItem(data, "Chns");
    if (chns_array != NULL && cJSON_IsArray(chns_array))
    {
        // 中文注释: 如果 "Chns" 数组存在，则按需启动
        cJSON *chn_item;
        cJSON_ArrayForEach(chn_item, chns_array)
        {
            if (cJSON_IsNumber(chn_item))
            {
                if (start_energy_test_for_channel(chn_item->valueint, test_mode, pulse_constant, freq_div_factor, target_rounds, target_times))
                {
                    is_any_task_started = true;
                }
            }
        }
    }
    else
    {
        // 中文注释: 如果 "Chns" 节点不存在，则默认启动所有通道
        printf("CPU1: SetTaskEnergyTest: 'Chns' not provided, starting all channels (P and Q).\n");
        bool started = false;
        for (int i = 1; i <= NUM_ENERGY_CHANNELS; i++)
        {
            if (start_energy_test_for_channel(i, test_mode, pulse_constant, freq_div_factor, target_rounds, target_times))
            {
                started = true;
            }
        }
        if (started)
        {
            is_any_task_started = true;
        }
    }

    // --- 4. 准备并发送回复 ---
    if (is_any_task_started)
    {
        result = "Success";
    }

send_reply_energy_test:
    printf("CPU1: SetTaskEnergyTest Reply: %s\n", result);
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetTaskEnergyTest");
    strcpy(replyData.Result, result);
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

/**
 * @brief 处理 "TerminateRunningTask" JSON 指令 (已根据新需求重构)
 * @param data 包含要终止的任务列表的 cJSON 对象
 * @details
 * - 解析 "Tasks" 数组。
 * - 如果数组存在，则根据数组成员终止指定的任务。
 * - 如果数组不存在，则终止所有当前已知的可终止任务。
 */
void handle_TerminateRunningTask(cJSON *data)
{
    char *result_status = "Success"; // 默认指令执行成功
    cJSON *tasks_array = NULL;

    if (data != NULL)
    {
        tasks_array = cJSON_GetObjectItem(data, "Tasks");
    }

    // 中文注释: 检查 "Tasks" 节点是否存在且是一个数组
    if (tasks_array != NULL && cJSON_IsArray(tasks_array))
    {
        // --- 逻辑分支 1: "Tasks" 节点存在，按需终止 ---
        printf("CPU1: [INFO] Terminating specific tasks as requested.\n");
        cJSON *task_item;
        cJSON_ArrayForEach(task_item, tasks_array)
        {
            if (cJSON_IsString(task_item) && (task_item->valuestring != NULL))
            {
                const char *task_name = task_item->valuestring;
                printf("CPU1: [INFO]   - Processing termination for: %s\n", task_name);

                if (strcmp(task_name, "SetTaskEnergyTest") == 0)
                {
                    PowerPulse_TerminateTest(); // 调用电能测试模块的终止函数
                }
                // --- 未来扩展占位符 ---
                // else if (strcmp(task_name, "SetTaskClockTest") == 0) {
                //     ClockTest_Terminate(); // 假设的时钟测试终止函数
                // }
                if (strcmp(task_name, "StateSequence") == 0)
                {
                    StateSequence_Stop();
                }
            }
        }
    }
    else
    {
        // --- 逻辑分支 2: "Tasks" 节点不存在，终止所有任务 ---
        printf("CPU1: [INFO] 'Tasks' node not found. Terminating all running tasks.\n");

        // 终止电能误差测试
        PowerPulse_TerminateTest();
        // 终止状态序列
        StateSequence_Stop();
    }

    // 中文注释: 准备并发送上行回复
    ReplyData replyData;
    strcpy(replyData.FunCode, "TerminateRunningTask");
    strcpy(replyData.Result, result_status);
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
        xil_printf("Failed to print JSON.\r\n");
        cJSON_Delete(json);
        return;
    }

    // 在字符串首尾加上'|'字符
    size_t jsonStringLength = strlen(jsonString);
    char *finalString = (char *)malloc(jsonStringLength + 3); // +3 是为首尾 '|' 和终止符 '\0'
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\r\n");
        cJSON_free(jsonString);
        cJSON_Delete(json);
        return;
    }
    snprintf(finalString, jsonStringLength + 3, "|%s|", jsonString);

    // 将字符串写入共享内存
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\r\n", bytesWritten);
    }
    else
    {
        //        xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\r\n", bytesWritten, jsonString);
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
    char *string = cJSON_PrintUnformatted(report);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\r\n");
        cJSON_Delete(report);
        return;
    }

    // Add the pipe characters
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 for the two '|' and null terminator
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\r\n");
        cJSON_Delete(report);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // Write to message queue
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1: Failed to write to message queue: %ld\r\n", bytesWritten);
    }
    else
    {
        xil_printf("CPU1: Protection event reported successfully\r\n");
    }

    // 打印最终的JSON字符串用于测试
    // xil_printf("CPU1: Protection event JSON: %s\r\n", finalString);

    // 硬件操作
    //  关闭DA
    Wave_Frequency = 50;                                   // 频率
    memset(Phase_shift, 0, sizeof(Phase_shift));           // 清空基波相位
    enable = 0x00;                                         // 关闭通道输出
    memset(numHarmonics, 0, sizeof(numHarmonics));         // 清除谐波
    memset(harmonics, 0, sizeof(harmonics));               // 清除谐波幅值
    memset(harmonics_phases, 0, sizeof(harmonics_phases)); // 清除谐波相位
    // str_wr_bram(PID_OFF);                                  // 生成交流信号

    // 关闭功放
    // 修改二级DA 波形幅度 量程
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = 0.0;
        Wave_Amplitude[i + 4] = 0.0;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }
    // power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
    // 更改UDP结构体100DevState的运行状态
    devState.bACMeterMode = 0;
    devState.nStatusFund = 0;

    // Clean up
    free(finalString);
    cJSON_Delete(report);
    free(string);
    // 清空UDP结构体
    initLineAC(&lineAC);
    initLineHarm(&lineHarm);
    for (int i = 0; i < 4; i++)
    {
        lineAC.ur[i] = setACS.Vals[i].UR;
        lineAC.ir[i] = setACS.Vals[i].IR;
    }
    udp_data_changed_flag = true;             // 更新UDP标志
    dac_parameters_updated_by_command = true; // 更新硬件操作标志
}
// 全局变量定义
Struct_StateSequence g_StateSequenceTask;
void handle_StateSequence(cJSON *data)
{
    cJSON *dataObj = data;

    if (!dataObj)
    {
        xil_printf("CPU1: no_dataObj\r\n");
        return;
    }

    // 1. 解析基本参数
    cJSON *item = cJSON_GetObjectItem(dataObj, "StartMode");
    if (item)
        g_StateSequenceTask.StartMode = item->valueint;

    item = cJSON_GetObjectItem(dataObj, "StartTime");
    if (item && item->valuestring)
        strncpy(g_StateSequenceTask.StartTime, item->valuestring, 31);

    item = cJSON_GetObjectItem(dataObj, "RepeatCount");
    if (item)
        g_StateSequenceTask.RepeatCount = item->valueint;

    item = cJSON_GetObjectItem(dataObj, "RecStartState");
    if (item)
        g_StateSequenceTask.RecStartState = item->valueint;

    item = cJSON_GetObjectItem(dataObj, "RecMS");
    if (item)
        g_StateSequenceTask.RecMS = item->valueint;

    item = cJSON_GetObjectItem(dataObj, "RecSamp");
    if (item)
        g_StateSequenceTask.RecSamp = item->valueint;

    // 2. 解析 States 数组
    cJSON *states = cJSON_GetObjectItem(dataObj, "States");
    if (states && cJSON_IsArray(states))
    {
        int stepCount = cJSON_GetArraySize(states);
        if (stepCount > MAX_SEQ_STEPS)
            stepCount = MAX_SEQ_STEPS;
        g_StateSequenceTask.StepCount = stepCount;

        for (int i = 0; i < stepCount; i++)
        {
            cJSON *state = cJSON_GetArrayItem(states, i);
            Struct_Seq_Step *pStep = &g_StateSequenceTask.Steps[i];

            // 默认初始化
            memset(pStep, 0, sizeof(Struct_Seq_Step));

            item = cJSON_GetObjectItem(state, "MaxDuration");
            if (item)
                pStep->MaxDuration = item->valueint;

            item = cJSON_GetObjectItem(state, "JumpTo");
            if (item)
                pStep->JumpTo = item->valueint;

            item = cJSON_GetObjectItem(state, "TrigLogic");
            if (item)
                pStep->TrigLogic = item->valueint;
            else
                pStep->TrigLogic = -1; // 默认无触发

            // 解析 TrigDI
            cJSON *trigDIs = cJSON_GetObjectItem(state, "TrigDI");
            if (trigDIs && cJSON_IsArray(trigDIs))
            {
                int count = cJSON_GetArraySize(trigDIs);
                if (count > 8)
                    count = 8;
                pStep->TrigDICount = count;
                for (int j = 0; j < count; j++)
                {
                    cJSON *tdi = cJSON_GetArrayItem(trigDIs, j);
                    cJSON *chn = cJSON_GetObjectItem(tdi, "Chn");
                    cJSON *val = cJSON_GetObjectItem(tdi, "val");
                    if (chn)
                        pStep->TrigDIs[j].Chn = chn->valueint;
                    if (val)
                        pStep->TrigDIs[j].Val = val->valueint;
                }
            }

            // 解析 AC
            cJSON *acs = cJSON_GetObjectItem(state, "AC");
            if (acs && cJSON_IsArray(acs))
            {
                int count = cJSON_GetArraySize(acs);
                if (count > 8)
                    count = 8;
                pStep->ACCount = count;
                for (int j = 0; j < count; j++)
                {
                    cJSON *ac = cJSON_GetArrayItem(acs, j);
                    Struct_Seq_AC *pAC = &pStep->ACs[j];

                    item = cJSON_GetObjectItem(ac, "Line");
                    if (item)
                        pAC->Line = item->valueint;
                    item = cJSON_GetObjectItem(ac, "Chn");
                    if (item)
                        pAC->Chn = item->valueint;
                    item = cJSON_GetObjectItem(ac, "U");
                    if (item)
                        pAC->U = (float)item->valuedouble;
                    item = cJSON_GetObjectItem(ac, "PhU");
                    if (item)
                        pAC->PhU = (float)item->valuedouble;
                    item = cJSON_GetObjectItem(ac, "I");
                    if (item)
                        pAC->I_ = (float)item->valuedouble;
                    item = cJSON_GetObjectItem(ac, "PhI");
                    if (item)
                        pAC->PhI = (float)item->valuedouble;
                    item = cJSON_GetObjectItem(ac, "F");
                    if (item)
                        pAC->F = (float)item->valuedouble;

                    // 解析 Harm
                    cJSON *harms = cJSON_GetObjectItem(ac, "Harm");
                    if (harms && cJSON_IsArray(harms))
                    {
                        int hCount = cJSON_GetArraySize(harms);
                        if (hCount > MAX_SEQ_HARMS)
                            hCount = MAX_SEQ_HARMS;
                        pAC->HarmCount = hCount;
                        for (int k = 0; k < hCount; k++)
                        {
                            cJSON *harm = cJSON_GetArrayItem(harms, k);
                            Struct_Seq_Harm *pHarm = &pAC->Harms[k];
                            item = cJSON_GetObjectItem(harm, "HN");
                            if (item)
                                pHarm->HN = item->valueint;
                            item = cJSON_GetObjectItem(harm, "U");
                            if (item)
                                pHarm->U = (float)item->valuedouble;
                            item = cJSON_GetObjectItem(harm, "PhU");
                            if (item)
                                pHarm->PhU = (float)item->valuedouble;
                            item = cJSON_GetObjectItem(harm, "I");
                            if (item)
                                pHarm->I_ = (float)item->valuedouble;
                            item = cJSON_GetObjectItem(harm, "PhI");
                            if (item)
                                pHarm->PhI = (float)item->valuedouble;
                        }
                    }
                }
            }

            // 解析 DO
            cJSON *dos = cJSON_GetObjectItem(state, "DO");
            if (dos && cJSON_IsArray(dos))
            {
                int count = cJSON_GetArraySize(dos);
                if (count > 8)
                    count = 8;
                pStep->DOCount = count;
                for (int j = 0; j < count; j++)
                {
                    cJSON *d = cJSON_GetArrayItem(dos, j);
                    cJSON *chn = cJSON_GetObjectItem(d, "Chn");
                    cJSON *val = cJSON_GetObjectItem(d, "val");
                    if (chn)
                        pStep->DOs[j].Chn = chn->valueint;
                    if (val)
                        pStep->DOs[j].Val = val->valueint;
                }
            }
        }
    }

    // ============================================================
    // 2.5 [新增] 安全校验：检查所有步骤的幅值是否超过装置极限
    //     Max Voltage = 6.5 V
    //     Max Current = 5.0 A
    // ============================================================
    for (int i = 0; i < g_StateSequenceTask.StepCount; i++)
    {
        Struct_Seq_Step *step = &g_StateSequenceTask.Steps[i];
        for (int j = 0; j < step->ACCount; j++)
        {
            Struct_Seq_AC *ac = &step->ACs[j];

            // 检查电压 (U > 6.5)
            if (ac->U > 6.5f + 0.001f)
            { // 加一点容差防止浮点精度误判
                cJSON *reply = cJSON_CreateObject();
                cJSON_AddStringToObject(reply, "FunType", "Reply");
                cJSON_AddStringToObject(reply, "FunCode", "StateSequence");
                cJSON_AddStringToObject(reply, "Result", "Failure");

                cJSON *replyData = cJSON_CreateObject();
                char errInfo[64];
                // 构造错误信息，告知具体的超限值
                snprintf(errInfo, sizeof(errInfo), "Step %d Chn %d U=%.2f exceeds limit 6.5V", i + 1, ac->Chn, ac->U);
                cJSON_AddStringToObject(replyData, "ErrInfo", errInfo);
                // 发生错误时 AC 列表为空或不回
                cJSON_AddItemToObject(reply, "Data", replyData);

                char *string = cJSON_PrintUnformatted(reply);
                if (string)
                {
                    printf("CPU1: [DEBUG] Validation Failed: %s\r\n", string);
                    size_t len = strlen(string);
                    char *finalStr = malloc(len + 3);
                    if (finalStr)
                    {
                        snprintf(finalStr, len + 3, "|%s|", string);
                        MsgQue_write(finalStr, strlen(finalStr));
                        free(finalStr);
                    }
                    free(string);
                }
                cJSON_Delete(reply);
                return; // 直接退出，不执行后续逻辑
            }

            // 检查电流 (I > 5.0)
            if (ac->I_ > 5.0f + 0.001f)
            {
                cJSON *reply = cJSON_CreateObject();
                cJSON_AddStringToObject(reply, "FunType", "Reply");
                cJSON_AddStringToObject(reply, "FunCode", "StateSequence");
                cJSON_AddStringToObject(reply, "Result", "Failure");

                cJSON *replyData = cJSON_CreateObject();
                char errInfo[64];
                snprintf(errInfo, sizeof(errInfo), "Step %d Chn %d I=%.2f exceeds limit 5.0A", i + 1, ac->Chn, ac->I_);
                cJSON_AddStringToObject(replyData, "ErrInfo", errInfo);
                cJSON_AddItemToObject(reply, "Data", replyData);

                char *string = cJSON_PrintUnformatted(reply);
                if (string)
                {
                    printf("CPU1: [DEBUG] Validation Failed: %s\r\n", string);
                    size_t len = strlen(string);
                    char *finalStr = malloc(len + 3);
                    if (finalStr)
                    {
                        snprintf(finalStr, len + 3, "|%s|", string);
                        MsgQue_write(finalStr, strlen(finalStr));
                        free(finalStr);
                    }
                    free(string);
                }
                cJSON_Delete(reply);
                return; // 直接退出
            }
        }
    }
    // ============================================================
    // 3. 量程自动计算与锁定 (Auto Range Calculation)
    // 逻辑：扫描所有步骤，找出每个通道的最大 U 和 I，确定该通道在整个序列中的固定量程

    // 假设 Line 1 有 4 个通道
    float maxU[4] = {0};
    float maxI[4] = {0};

    // 3.1 第一遍扫描：找最大值
    for (int i = 0; i < g_StateSequenceTask.StepCount; i++)
    {
        Struct_Seq_Step *step = &g_StateSequenceTask.Steps[i];
        for (int j = 0; j < step->ACCount; j++)
        {
            Struct_Seq_AC *ac = &step->ACs[j];
            // 只处理 Line 1, Chn 1-4
            if (ac->Line == 1 && ac->Chn >= 1 && ac->Chn <= 4)
            {
                int idx = ac->Chn - 1;
                if (ac->U > maxU[idx])
                    maxU[idx] = ac->U;
                if (ac->I_ > maxI[idx])
                    maxI[idx] = ac->I_;
            }
        }
    }

    // 3.2 准备回复 JSON 对象
    cJSON *replyACArray = cJSON_CreateArray();

    // 3.3 第二遍：确定量程，回写 Task 结构体，并填充 Reply
    for (int chn = 1; chn <= 4; chn++)
    {
        int idx = chn - 1;
        float finalUR, finalIR;

        // 计算电压量程
        if (maxU[idx] > 3.25f)
            finalUR = 6.5f;
        else if (maxU[idx] > 1.876f)
            finalUR = 3.25f;
        else
            finalUR = 1.876f;

        // 计算电流量程
        if (maxI[idx] > 1.0f)
            finalIR = 5.0f;
        else if (maxI[idx] > 0.2f)
            finalIR = 1.0f;
        else
            finalIR = 0.2f;

        // 添加到回复 JSON
        cJSON *chnObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(chnObj, "Line", 1);
        cJSON_AddNumberToObject(chnObj, "Chn", chn);
        cJSON_AddNumberToObject(chnObj, "UR", finalUR);
        cJSON_AddNumberToObject(chnObj, "IR", finalIR);
        cJSON_AddItemToArray(replyACArray, chnObj);

        // 回写到全局任务结构体的每一个步骤中
        // 这样 Precalculate_HwParams 就能读到正确的 UR/IR 来计算系数
        for (int i = 0; i < g_StateSequenceTask.StepCount; i++)
        {
            Struct_Seq_Step *step = &g_StateSequenceTask.Steps[i];
            for (int j = 0; j < step->ACCount; j++)
            {
                if (step->ACs[j].Line == 1 && step->ACs[j].Chn == chn)
                {
                    step->ACs[j].UR = finalUR;
                    step->ACs[j].IR = finalIR;
                }
            }
        }
    }

    // 4. 构建并发送回复
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "StateSequence");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *replyData = cJSON_CreateObject();
    cJSON_AddItemToObject(replyData, "AC", replyACArray); // 添加计算好的 AC 量程信息
    cJSON_AddItemToObject(reply, "Data", replyData);

    char *string = cJSON_PrintUnformatted(reply);
    if (string)
    {
        // printf("CPU1: StateSequence Reply: %s\n", string);
        size_t stringLength = strlen(string);
        char *finalString = (char *)malloc(stringLength + 3);
        if (finalString)
        {
            snprintf(finalString, stringLength + 3, "|%s|", string);
            MsgQue_write(finalString, strlen(finalString));
            free(finalString);
        }
        free(string);
    }
    cJSON_Delete(reply);

    // 5. 启动逻辑
    if (g_StateSequenceTask.StartMode == 0)
    {
        xil_printf("CPU1: Cmd StateSequence -> Start Immediately.\r\n");
        StateSequence_PrepareAndStart();
    }
    else if (g_StateSequenceTask.StartMode == 1)
    {
        xil_printf("CPU1: Cmd StateSequence -> Schedule Start at %s.\r\n", g_StateSequenceTask.StartTime);
        // SoftTimer_SetAlarm(g_StateSequenceTask.StartTime); // 待实现
    }
}

size_t calculate_dynamic_payload_size(ReportEnableStatus ReportStatus)
{
    size_t payload_size = 0;
    size_t header_size = sizeof(u32) * 2;

    if (ReportStatus.DevState)
    {
        payload_size += header_size + sizeof(DevState);
        //        printf("DevState size: %zu\r\n", header_size + sizeof(LineDevState));
    }
    else
    {
        payload_size += header_size;
        //        printf("DevState header only: %zu\r\n", header_size);
    }

    if (ReportStatus.BaseDataAC)
    {

        payload_size += header_size + sizeof(LineAC);
        //        printf("BaseDataAC size: %zu\r\n", header_size + sizeof(LineAC));
    }
    else
    {
        payload_size += header_size;
        //        printf("BaseDataAC header only: %zu\r\n", header_size);
    }

    if (ReportStatus.HarmData)
    {
        payload_size += header_size + sizeof(LineHarm);
        //        printf("HarmData size: %zu\r\n", header_size + sizeof(LineHarm));
    }
    else
    {
        payload_size += header_size;
        //        printf("HarmData header only: %zu\r\n", header_size);
    }

    if (ReportStatus.DI)
    {
        payload_size += header_size + sizeof(LineDI);
        //        printf("DI size: %zu\r\n", header_size + sizeof(LineDI));
    }
    else
    {
        payload_size += header_size;
        //        printf("DI header only: %zu\r\n", header_size);
    }

    if (ReportStatus.DO)
    {
        payload_size += header_size + sizeof(LineDO);
        //        printf("DO size: %zu\r\n", header_size + sizeof(LineDO));
    }
    else
    {
        payload_size += header_size;
        //        printf("DO header only: %zu\r\n", header_size);
    }

    //    printf("Total dynamic payload size: %zu\r\n", payload_size);
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
void ReportUDP_Structure(ReportEnableStatus ReportStatus)
{
    if (udp_data_changed_flag == 0)
    {
        // 如果数据未发生变化，返回
        return;
    }
    udp_data_changed_flag = false;

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
        printf("CPU1: ERROR - UDP data too large for shared memory: %zu > %d\r\n",
               16 + dynamic_payload_size + 4, UDP_MEM_SIZE);
        return; // 数据太大，直接返回不写入
    }

    write_UDP_to_shared_memory(UDP_ADDRESS, &udpPacket, 16 + dynamic_payload_size + 4);

    // 设置共享内存最后一个字节为 0，标记空闲
    Xil_Out32(last_byte_addr, 0);
    Xil_DCacheFlushRange((INTPTR)last_byte_addr, sizeof(u32));

    // 当前发送计数器
    static uint32_t udpReportCount = 0; // Read from shared memory
    udpReportCount++;
    if (udpReportCount >= 0xFFFFFFFF)
    { // Check for overflow (max uint32_t value)
        udpReportCount = 0;
    }
    // 更新计数器
    Xil_Out32(UDP_ADDRESS + UDP_MEM_SIZE - 8, udpReportCount);                        // Write back to shared memory
    Xil_DCacheFlushRange((INTPTR)(UDP_ADDRESS + UDP_MEM_SIZE - 8), sizeof(uint32_t)); // Flush cache
    // printf("CPU1:UDP Report Count: %ld\r\n", Xil_In32(UDP_ADDRESS + UDP_MEM_SIZE - 8));
    // // 刷新整个UDP
    // Xil_DCacheFlushRange((UINTPTR)&udpPacket, sizeof(udpPacket));
    // 打印调试信息
    // printf("lineAC size: %zu bytes\r\n", sizeof(LineAC));
    // printf("dynamic_payload_size: %zu bytes\r\n", dynamic_payload_size);
    // printf("UDP_PACKET_SIZE: %d bytes\r\n", sizeof(udpPacket));
    // printf("CPU1: UDP written to memory at: 0x%X\r\n", UDP_ADDRESS);
}

// Function to initialize DevState
void initDevState(DevState *devState)
{
    devState->bACMeterMode = 0;  // 0=交流源状态;1=交流表状态
    devState->bClosedLoop = 1;   // 0=开环状态;1=闭环状态    //默认闭环
    devState->nStatusFund = 0;   // 原 nStatusFund
    devState->nStatusHarm = 0;   // 原 bHarmRunning
    devState->nStatusInharm = 0; // 新增
    devState->bAutoRange = 0;    // 新增
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
    udp_data_changed_flag = true; // 更新UDP标志
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
    udp_data_changed_flag = true; // 更新UDP标志
}

// Function to initialize LineHarm
void initLineHarm(LineHarm *lineHarm)
{
    for (int i = 0; i < ChnsAC; i++)
    {
        for (int j = 0; j <= HarmNumberMax; j++)
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
    udp_data_changed_flag = true; // 更新UDP标志
}
void initLineDI(LineDI *lineDI)
{
    for (int i = 0; i < ChnsDI; i++)
    {
        lineDI->DI[i].v = 0;
    }
    udp_data_changed_flag = true; // 更新UDP标志
}

void initLineDO(LineDO *lineDO)
{
    for (int i = 0; i < ChnsDO; i++)
    {
        lineDO->DO[i].v = 0;
    }
    udp_data_changed_flag = true; // 更新UDP标志
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

    udp_data_changed_flag = true; // 更新UDP标志
    // 打印初始化完成消息
    // printf("CPU1: setACS.Vals initialization completed.\r\n");
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
    udp_data_changed_flag = true; // 更新UDP标志
}
