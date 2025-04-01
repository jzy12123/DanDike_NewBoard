/*
 * ���ļ���ĺ�����������linux������JSONָ��
 * ͬʱ���岢�ϱ�UDP�ṹ��
 * ���ṹ�����ɡ�
    (
        ֡ͷ������16�ֽڣ�
            ͬ��ͷ��			4�ֽ�		�̶�Ϊ��D1D2D3D4
            �������ܳ���1��		4�ֽڣ�ֵΪn��
            �������ܳ���2��		4�ֽڣ�ֵΪn���ظ���
            �汾��Ϣ�� 			   1�ֽ�
            ������				       3�ֽ�

        ����������n�ֽڣ�	����ṹ�ɶ���ṹ����ʽ��ɡ�
            �ṹ�����ͱ�ʶ��	4�ֽ�
            �ṹ�����ݳ��ȣ�	4�ֽ�
            �ṹ�����ݣ�		n1�ֽ�

            �ṹ�����ͱ�ʶ��	4�ֽ�
            �ṹ�����ݳ��ȣ�	4�ֽ�
            �ṹ�����ݣ�		n2�ֽ�
            ...

        ֡β������4�ֽڣ�
            ��������			4�ֽ�		�̶�Ϊ��E1E2E3E4
    )
 *
 */
#include "Communications_Protocol.h"

void extractContentBetweenPipes(char *buffer)
{
    int len = strlen(buffer);
    int start = -1, end = -1;

    // �����ַ������ҵ���һ�������һ�� '|' ��λ��
    for (int i = 0; i < len; i++)
    {
        if (buffer[i] == '|')
        {
            if (start == -1)
            {
                start = i; // �ҵ���һ�� '|'
            }
            else
            {
                end = i; // �������һ�� '|' ��λ��
            }
        }
    }

    // ����ҵ������� '|'���������ǲ���ͬһ��λ��
    if (start != -1 && end != -1 && start != end)
    {
        // �����м����ݵĳ���
        int contentLength = end - start - 1;

        // ���м������ƶ��� buffer �Ŀ�ͷ
        memmove(buffer, buffer + start + 1, contentLength);

        // ��ĩβ����ַ���������
        buffer[contentLength] = '\0';

        printf("CPU1: Remove | \n");
    }
    else
    {
        printf("CPU1: NotFound | \n");
    }
}

/*
 * ����JSONָ��㺯��
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
        {"setCalibrateAC", handle_setCalibrateAC},
        {"writeCalibrateAC", handle_writeCalibrateAC}};
    const int funCodeMapSize = sizeof(funCodeMap) / sizeof(funCodeMap[0]);

    // ��������ַ�����β�� '|'
    extractContentBetweenPipes(buffer);

    // ��ӡbuffer�������
    printf("CPU1: Recived_JSON: %s\n", buffer);

    // ʹ��cJSON����JSON�ַ���
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

    // ����Data
    cJSON *data = cJSON_GetObjectItem(json, "Data");

    // ���� FunCode ��ת����Ӧ�Ĵ�����
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
 * @brief ���� GetFunCodeList ����
 *
 * �ú������ڴ��� GetFunCodeList ���󣬲����ذ���֧�ֵ�ָ���б������б�������¼��б�� JSON ��Ӧ��
 *
 * @param data ָ������������ݵ� cJSON �����ָ��
 */
void handle_GetFunCodeList(cJSON *data)
{
    // ���� GetFunCodeList ���߼�
    //    xil_printf("Handling handle_GetFunCodeList...\n");

    // ���� FunCode �б�
    const char *CmdList[] = {
        "GetFunCodeList", "GetDevBaseInfo", "GetDevState", "SetReportEnable",
        "SetDCS", "SetDCM", "SetACS", "SetACM", "SetHarm", "SetInterHarm", "SetDO",
        "StopDCS", "StopACS", "ClearHarm", "ClearInterHarm"};

    const char *ReportList[] = {
        "ProtectEvent", "DISOE", "BaseDataAC", "BaseDataDC", "BaseDataIO", "HarmData", "InterHarmData"};

    const char *TaskEventList[] = {
        "SetSyncMode", "SetProgress"};

    // ��������ָ��� JSON ����
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

    // �����ɵ� JSON ת��Ϊ�ַ���
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(reply);
        return;
    }

    // ���ַ�����β���� '|' �ַ�
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 Ϊ����β '|' �ͽ�����
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // �����ɵ� JSON д�빲���ڴ�
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // ����
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

void handle_GetDevBaseInfo(cJSON *data)
{
    // ���� GetDevBaseInfo ���߼�
    //    xil_printf("Handling handle_GetDevBaseInfo...\n");

    // ��������ָ��� JSON ����
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "GetDevBaseInfo");
    cJSON_AddStringToObject(reply, "Result", "Success");

    cJSON *dataObj = cJSON_CreateObject();
    cJSON_AddStringToObject(dataObj, "Model", "xxx");
    cJSON_AddStringToObject(dataObj, "InnerModel", "DK-34B1");
    cJSON_AddStringToObject(dataObj, "FPGA_Ver", "1.0.1");
    cJSON_AddStringToObject(dataObj, "ARM_Ver", "1.0.1");

    const char *syncModes[] = {"GPS", "BD", "IRIG-B", "SNTP", "Manual"};
    cJSON *syncMode = cJSON_CreateStringArray(syncModes, 5);
    cJSON_AddItemToObject(dataObj, "SyncMode", syncMode);

    // DI ��Ϣ
    cJSON *di = cJSON_CreateObject();
    cJSON_AddNumberToObject(di, "ChnCount", ChnsBI);
    double diResolution[] = {0.1, 1, 10}; // ����ֱ���
    cJSON *diResolutions = cJSON_CreateDoubleArray(diResolution, 3);
    cJSON_AddItemToObject(di, "DI_Resolution", diResolutions);
    cJSON_AddItemToObject(dataObj, "DI", di);

    // DO ��Ϣ
    cJSON *doInfo = cJSON_CreateObject();
    cJSON_AddNumberToObject(doInfo, "ChnCount", ChnsBO);
    cJSON_AddItemToObject(dataObj, "DO", doInfo);

    // AC ��Ϣ
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

    // DCS ��Ϣ
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

    // DCM ��Ϣ
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

    // �����ɵ� JSON ת��Ϊ�ַ���
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(reply);
        return;
    }

    // ���ַ�����β���� '|' �ַ�
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 Ϊ����β '|' �ͽ�����
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // �����ɵ� JSON д�빲���ڴ�
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // ����
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

void handle_GetDevState(cJSON *data)
{
    // ���� handle_GetDevState ���߼�
    //	xil_printf("Handling handle_GetDevState...\n");

    // ��������ָ��� JSON ����
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

    // AC ��Ϣ
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

    // DCS ��Ϣ
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

    // DCM ��Ϣ
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

    // DO ��Ϣ
    cJSON *doInfo = cJSON_CreateArray();
    for (int i = 0; i < ChnsBO; i++)
    {
        cJSON_AddItemToArray(doInfo, cJSON_CreateNumber(lineDO.DO[i].v));
    }
    cJSON_AddItemToObject(dataObj, "DO", doInfo);

    // DI ��Ϣ
    cJSON *diInfo = cJSON_CreateArray();
    for (int i = 0; i < ChnsBI; i++)
    {
        cJSON_AddItemToArray(diInfo, cJSON_CreateNumber(lineDI.DI[i].v));
    }
    cJSON_AddItemToObject(dataObj, "DI", diInfo);

    cJSON_AddItemToObject(reply, "Data", dataObj);

    // ӳ�䵽Ӳ��

    // �����ɵ� JSON ת��Ϊ�ַ���
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(reply);
        return;
    }

    // ���ַ�����β���� '|' �ַ�
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 Ϊ����β '|' �ͽ�����
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_Delete(reply);
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // �����ɵ� JSON д�빲���ڴ�
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // ����
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

ReportEnableStatus reportStatus = {true, true, true, true, true, true, true, true, true, true, true};
/**
 * @brief ���� handle_SetReportEnable ���߼�
 *
 * �ú���������� JSON ��������ȡ���������ʹ��״̬�������µ� reportStatus �ṹ���С�
 * Ȼ�󣬹���һ���ظ� JSON �������а����������͸��º�ı�����ʹ��״̬��
 * ���� JSON ����ת��Ϊ�ַ����������ַ�����β���� '|' �ַ�������ַ���д�빲���ڴ档
 *
 * @param data ����� JSON ���ݣ�������Ҫ���µı�����ʹ��״̬
 */
void handle_SetReportEnable(cJSON *data)
{
    // ���� handle_SetReportEnable ���߼�
    // xil_printf("Handling handle_SetReportEnable...\n");

    // ����ʹ��״̬
    // ��ȡ BaseDataAC ������ʹ��״̬
    cJSON *BaseDataAC = cJSON_GetObjectItem(data, "BaseDataAC");
    if (BaseDataAC != NULL)
    {
        // BaseDataAC ʹ��״̬����
        reportStatus.BaseDataAC = cJSON_IsTrue(BaseDataAC);
    }

    // ��ȡ HarmData ������ʹ��״̬
    cJSON *HarmData = cJSON_GetObjectItem(data, "HarmData");
    if (HarmData != NULL)
    {
        // HarmData ʹ��״̬����
        reportStatus.HarmData = cJSON_IsTrue(HarmData);
    }

    // ��ȡ InterHarmData ������ʹ��״̬
    cJSON *InterHarmData = cJSON_GetObjectItem(data, "InterHarmData");
    if (InterHarmData != NULL)
    {
        // InterHarmData ʹ��״̬����
        reportStatus.InterHarmData = cJSON_IsTrue(InterHarmData);
    }

    // ��ȡ BaseDataDCS ������ʹ��״̬
    cJSON *BaseDataDCS = cJSON_GetObjectItem(data, "BaseDataDCS");
    if (BaseDataDCS != NULL)
    {
        // BaseDataDCS ʹ��״̬����
        reportStatus.BaseDataDCS = cJSON_IsTrue(BaseDataDCS);
    }

    // ��ȡ BaseDataDCM ������ʹ��״̬
    cJSON *BaseDataDCM = cJSON_GetObjectItem(data, "BaseDataDCM");
    if (BaseDataDCM != NULL)
    {
        // BaseDataDCM ʹ��״̬����
        reportStatus.BaseDataDCM = cJSON_IsTrue(BaseDataDCM);
    }

    // ��ȡ DI ������ʹ��״̬
    cJSON *DI = cJSON_GetObjectItem(data, "DI");
    if (DI != NULL)
    {
        // DI ʹ��״̬����
        reportStatus.DI = cJSON_IsTrue(DI);
    }

    // ��ȡ DISOE ������ʹ��״̬
    cJSON *DISOE = cJSON_GetObjectItem(data, "DISOE");
    if (DISOE != NULL)
    {
        // DISOE ʹ��״̬����
        reportStatus.DISOE = cJSON_IsTrue(DISOE);
    }

    // ��ȡ DO ������ʹ��״̬
    cJSON *DO = cJSON_GetObjectItem(data, "DO");
    if (DO != NULL)
    {
        // DO ʹ��״̬����
        reportStatus.DO = cJSON_IsTrue(DO);
    }

    // ��ȡ InnerBattery ������ʹ��״̬
    cJSON *InnerBattery = cJSON_GetObjectItem(data, "InnerBattery");
    if (InnerBattery != NULL)
    {
        // InnerBattery ʹ��״̬����
        reportStatus.InnerBattery = cJSON_IsTrue(InnerBattery);
    }

    // ��ȡ VMData ������ʹ��״̬
    cJSON *VMData = cJSON_GetObjectItem(data, "VMData");
    if (VMData != NULL)
    {
        // VMData ʹ��״̬����
        reportStatus.VMData = cJSON_IsTrue(VMData);
    }

    /*��������ָ��� JSON ����*/
    cJSON *reply = cJSON_CreateObject();
    cJSON_AddStringToObject(reply, "FunType", "Reply");
    cJSON_AddStringToObject(reply, "FunCode", "SetReportEnable");
    cJSON_AddStringToObject(reply, "Result", "Success");

    // ���� dataObj JSON ����
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

    // �� dataObj ��ӵ� reply ��
    cJSON_AddItemToObject(reply, "Data", dataObj);

    // �����ɵ� JSON ת��Ϊ�ַ���
    char *string = cJSON_Print(reply);
    if (string == NULL)
    {
        // ��ӡ������Ϣ
        xil_printf("Failed to print JSON.\n");
        // ɾ�� reply ����
        cJSON_Delete(reply);
        return;
    }

    // ���ַ�����β���� '|' �ַ�
    size_t stringLength = strlen(string);
    char *finalString = (char *)malloc(stringLength + 3); // +3 Ϊ����β '|' �ͽ�����
    if (finalString == NULL)
    {
        // ��ӡ�ڴ����ʧ����Ϣ
        xil_printf("Failed to allocate memory for final JSON string.\n");
        // ɾ�� reply ����
        cJSON_Delete(reply);
        // �ͷ� string �ڴ�
        free(string);
        return;
    }
    snprintf(finalString, stringLength + 3, "|%s|", string);

    // �����ɵ� JSON д�빲���ڴ�
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        // ��ӡд����Ϣ����ʧ����Ϣ
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        // ��ӡд����Ϣ���гɹ���Ϣ
        //		xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, string);
    }

    // ����
    free(finalString);
    cJSON_Delete(reply);
    free(string);
}

void handle_SetDCS(cJSON *data)
{
    // ���� handle_SetDCS ���߼�
    //    xil_printf("CPU1: Handling handle_SetDCS...\r\n");

    SetDCS setDCS;
    cJSON *closedLoop = cJSON_GetObjectItem(data, "ClosedLoop");
    if (closedLoop != NULL)
    {
        setDCS.ClosedLoop = cJSON_IsTrue(closedLoop);
    }
    else
    {
        setDCS.ClosedLoop = true; // Ĭ��ֵ
    }

    // ��ȡChns
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

    // ��ӡ�����������֤
    //     xil_printf("ClosedLoop: %d\r\n", setDCS.ClosedLoop);
    for (int i = 0; i < ChnsDCS; i++)
    {
        printf("CHn: %d, UR: %.2f, U: %.2f, URipple: %.2f, IR: %.2f, I: %.2f, IRipple: %.2f\r\n",
               setDCS.Vals[i].Chn,
               setDCS.Vals[i].UR, setDCS.Vals[i].U, setDCS.Vals[i].URipple,
               setDCS.Vals[i].IR, setDCS.Vals[i].I_, setDCS.Vals[i].IRipple);
    }

    // �ر�JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetDCS");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = true;
    replyData.ClosedLoop = setDCS.ClosedLoop;

    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);
}

void handle_SetDCM(cJSON *data)
{
    // ���� handle_SetDCM ���߼�
    //    xil_printf("CPU1: Handling handle_SetDCM...\r\n");

    SetDCM setDCM;
    // ��ȡChns
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
    // ��ӡ�����������֤

    for (int i = 0; i < ChnsDCM; i++)
    {
        printf("CHn: %d, UR: %.2f,IR: %.2f, \r\n",
               setDCM.Vals[i].Chn,
               setDCM.Vals[i].UR,
               setDCM.Vals[i].IR);
    }

    // �ر�JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetDCM");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);
}

SetACS setACS;
/**
 * @brief ���� SetACS ����
 *
 * �ú������������ JSON ���ݣ����������ݸ�����Ӧ�ı������������������ӡ������
 * ������ɻظ����ݲ�д�빲���ڴ棬ͬʱӳ�����ݵ�Ӳ�������ɽ����źš�
 *
 * @param data ���� SetACS �������ݵ� JSON ����ָ��
 */
void handle_SetACS(cJSON *data)
{
    // ���� handle_SetACS ���߼�
    // xil_printf("CPU1: Handling handle_SetACS...\r\n");

    // ���� ClosedLoop�������������£�������ԭֵ
    cJSON *closedLoop = cJSON_GetObjectItem(data, "ClosedLoop");
    if (closedLoop != NULL)
    {
        // ���� ClosedLoop ��ֵ
        setACS.ClosedLoop = cJSON_IsTrue(closedLoop);
    }
    else
    {
        setACS.ClosedLoop = true; // Ĭ�ϱջ�
    }

    // ��ȡ vals ��
    cJSON *vals = cJSON_GetObjectItem(data, "vals");
    if (vals != NULL)
    {
        // ��ȡ vals ����Ĵ�С
        int valsCount = cJSON_GetArraySize(vals);
        for (int i = 0; i < valsCount; i++)
        {
            // ��ȡ vals �����е�ÿ��Ԫ��
            cJSON *val = cJSON_GetArrayItem(vals, i);

            // ֻ������������Ч����
            // ��ȡ Line ��
            cJSON *line = cJSON_GetObjectItem(val, "Line");
            if (line)
            {
                // ���� Line ��ֵ
                setACS.Vals[i].Line = line->valueint;
            }
            // ��ȡ Chn ��
            cJSON *chn = cJSON_GetObjectItem(val, "Chn");
            if (chn)
            {
                // ���� Chn ��ֵ
                setACS.Vals[i].Chn = chn->valueint;
            }
            // ��ȡ F ��
            cJSON *f = cJSON_GetObjectItem(val, "F");
            if (f)
            {
                // ���� F ��ֵ
                setACS.Vals[i].F = (float)f->valuedouble;
            }
            else
            {
                setACS.Vals[i].F = 50;
            }
            // ��ȡ UR ��
            cJSON *ur = cJSON_GetObjectItem(val, "UR");
            if (ur)
            {
                // ���� UR ��ֵ
                setACS.Vals[i].UR = (float)ur->valuedouble;
            }
            // ��ȡ U ��
            cJSON *u = cJSON_GetObjectItem(val, "U");
            if (u)
            {
                // ���� U ��ֵ
                setACS.Vals[i].U = (float)u->valuedouble;
            }
            else
            {
                setACS.Vals[i].U = 0;
            }
            // ��ȡ PhU ��
            cJSON *phU = cJSON_GetObjectItem(val, "PhU");
            if (phU)
            {
                // ���� PhU ��ֵ
                setACS.Vals[i].PhU = (float)phU->valuedouble;
            }
            else
            {
                setACS.Vals[i].PhU = 0;
            }
            // ��ȡ IR ��
            cJSON *ir = cJSON_GetObjectItem(val, "IR");
            if (ir)
            {
                // ���� IR ��ֵ
                setACS.Vals[i].IR = (float)ir->valuedouble;
            }
            // ��ȡ I ��
            cJSON *i_ = cJSON_GetObjectItem(val, "I");
            if (i_)
            {
                // ���� I ��ֵ
                setACS.Vals[i].I_ = (float)i_->valuedouble;
            }
            else
            {
                setACS.Vals[i].I_ = 0;
            }
            // ��ȡ PhI ��
            cJSON *phI = cJSON_GetObjectItem(val, "PhI");
            if (phI)
            {
                // ���� PhI ��ֵ
                setACS.Vals[i].PhI = (float)phI->valuedouble;
            }
            else
            {
                setACS.Vals[i].PhI = 0;
            }
        }
    }
    else
    {
        // ��ʹû���ҵ� vals �Ҳ�����أ����Ǽ���ִ��
        printf("CPU1: No vals found in JSON, continuing with ClosedLoop change only.\n");
    }

    // ��ӡ�����������֤
    // ��ӡ ClosedLoop ��ֵ
    xil_printf("ClosedLoop: %d\r\n", setACS.ClosedLoop);
    // ���� setACS.Vals ���飬����ӡÿ��Ԫ�ص�ֵ
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        printf("Line: %d, CHn: %d, F: %.2f, UR: %.2f, U: %.2f, PhU: %.2f, IR: %.2f, I: %.2f, PhI: %.2f\n",
               setACS.Vals[i].Line, setACS.Vals[i].Chn, setACS.Vals[i].F,
               setACS.Vals[i].UR, setACS.Vals[i].U, setACS.Vals[i].PhU,
               setACS.Vals[i].IR, setACS.Vals[i].I_, setACS.Vals[i].PhI);
    }

    // �ر�JSON
    // ��ʼ�� ReplyData �ṹ��
    ReplyData replyData;
    // ���� FunCode Ϊ "SetACS"
    strcpy(replyData.FunCode, "SetACS");
    // ���� Result Ϊ "Success"
    strcpy(replyData.Result, "Success");
    // ���� hasClosedLoop Ϊ true
    replyData.hasClosedLoop = true;
    // ���� ClosedLoop ��ֵ
    replyData.ClosedLoop = setACS.ClosedLoop;
    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);

    /*����ӳ�䵽Ӳ�� Ӧ�����ȥ������һ���߳�?*/
    // ���� Wave_Frequency ��ֵ
    Wave_Frequency = setACS.Vals[0].F; // Ƶ��
    // ���Wave_Frequency����45��65Hz����ӱ�����ʾ
    if (Wave_Frequency < 45 || Wave_Frequency > 65)
    {
        xil_printf("Error: Frequency out of range. Expected between 45 and 65 Hz.Set 50Hz\n");
        Wave_Frequency = 50;
    }
    // ���� Phase_shift �����ֵ��ǰ�ĸ�Ϊ��ѹ��λ�����ĸ�Ϊ������λ
    //  ���� setACS.Vals ���飬������ Phase_shift �����ֵ
    for (int i = 0; i < 4; i++)
    {
        Phase_shift[i] = setACS.Vals[i].PhU; // ��λ
        Phase_shift[i + 4] = setACS.Vals[i].PhI;
    }

    // ��� numHarmonics ����
    memset(numHarmonics, 0, sizeof(numHarmonics));
    // ��� harmonics ����
    memset(harmonics, 0, sizeof(harmonics));
    // ��� harmonics_phases ����
    memset(harmonics_phases, 0, sizeof(harmonics_phases));

    // �޸Ķ���DA ���η��� ����
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = (float)(setACS.Vals[i].U / setACS.Vals[i].UR) * 100;
        Wave_Amplitude[i + 4] = (float)(setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }

    // �ж����л����̻����������ã�������л����̾Ͳ����
    uint8_t POWAMP_ENABLE = 0xff;
    for (int i = 0; i < 8; i++)
    {
        if (Wave_Amplitude[i] == 0)
        {
            POWAMP_ENABLE &= ~(1 << i); // �����Ӧλ���رո�ͨ���Ĺ���ʹ��
        }
    }
    if (POWAMP_ENABLE == 0)
    {
        // ��ʹ��ͨ�����
        enable = 0x00;
        // ���ɽ����ź�
        str_wr_bram(PID_OFF);
        // ���ù���
        power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);

        // ����UDP�ṹ��100DevState������״̬
        devState.bACMeterMode = 0;
        devState.bACRunning = 0;
        devState.bClosedLoop = setACS.ClosedLoop;
    }
    else
    {
        // ʹ��ͨ�����
        enable = 0xff;
        // ���ɽ����ź�
        str_wr_bram(PID_OFF);
        // ���ù���
        power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_ON);
        // ����UDP�ṹ��100DevState������״̬
        devState.bACMeterMode = 0;
        devState.bACRunning = 1;
        devState.bClosedLoop = setACS.ClosedLoop;
    }
}

SetACM setACM;
void handle_SetACM(cJSON *data)
{
    // ���� handle_SetACM ���߼�
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
    // ��ӡ�����������֤
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        printf("Line: %d, CHn: %d, UR: %.2f,IR: %.2f, \n",
               setACM.Vals[i].Line,
               setACM.Vals[i].Chn,
               setACM.Vals[i].UR,
               setACM.Vals[i].IR);
    }

    // �ر�JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetACM");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);

    // ӳ�䵽Ӳ��
    // ����UDP�ṹ��100DevState������״̬
    devState.bACMeterMode = 1;
    devState.bACRunning = 1;
}

SetHarm setHarm;
void handle_SetHarm(cJSON *data)
{
    // ��ӡ������Ϣ
    // printf("CPU1: ����г����������...\n");

    // �������������
    int dataCount = cJSON_GetArraySize(data);

    // ���������е�����г��
    for (int i = 0; i < dataCount; i++)
    {
        cJSON *item = cJSON_GetArrayItem(data, i);
        if (item == NULL)
        {
            continue;
        }

        // ��ȡ�����������
        cJSON *line = cJSON_GetObjectItem(item, "Line");
        cJSON *chn = cJSON_GetObjectItem(item, "Chn");
        cJSON *hr = cJSON_GetObjectItem(item, "HN");

        // ���ȱ�ٻ����������������������
        if (!line || !chn || !hr)
        {
            printf("CPU1: Harmonic setting error - Missing required parameters(Line, Chn, HN)\n");
            continue;
        }

        // ��ȡ��ѡ����
        cJSON *u = cJSON_GetObjectItem(item, "U");
        cJSON *phU = cJSON_GetObjectItem(item, "PhU");
        cJSON *iField = cJSON_GetObjectItem(item, "I");
        cJSON *phI = cJSON_GetObjectItem(item, "PhI");

        // ����ͨ����г����������0��ʼ��
        int channelIndex = chn->valueint - 1; // JSON��ͨ����1��ʼ
        int harmonicIndex = hr->valueint - 2; // г��������0��ʼ����Ӧ��2��г��

        // �������߽��Է����
        if (channelIndex >= 0 && channelIndex < CHANNL_MAX &&
            harmonicIndex >= 0 && harmonicIndex < MAX_HARMONICS)
        {
            // �洢��setHarm�����ڼ�¼
            int setHarmIndex = i;
            if (setHarmIndex < LinesAC * ChnsAC * HarmNumberMax)
            {
                setHarm.Vals[setHarmIndex].Line = line->valueint;
                setHarm.Vals[setHarmIndex].Chn = chn->valueint;
                setHarm.Vals[setHarmIndex].HN = hr->valueint;

                // ֻ���ڲ�������ʱ�Ÿ���
                if (u)
                    setHarm.Vals[setHarmIndex].U = (float)u->valuedouble;
                if (phU)
                    setHarm.Vals[setHarmIndex].PhU = (float)phU->valuedouble;
                if (iField)
                    setHarm.Vals[setHarmIndex].I_ = (float)iField->valuedouble;
                if (phI)
                    setHarm.Vals[setHarmIndex].PhI = (float)phI->valuedouble;
            }

            // �����Ҫ���������г������
            if (hr->valueint > numHarmonics[channelIndex])
            {
                numHarmonics[channelIndex] = hr->valueint;
            }

            // ��ѹг�� - ֻ���ڲ�������ʱ�Ÿ���
            if (u)
            {
                harmonics[channelIndex][harmonicIndex] = (float)u->valuedouble;
            }
            if (phU)
            {
                harmonics_phases[channelIndex][harmonicIndex] = (float)phU->valuedouble;
            }

            // ����г�� - ֻ���ڲ�������ʱ�Ÿ���
            if (iField || phI)
            {
                // �����Ҫ���������г������
                if (hr->valueint > numHarmonics[channelIndex + 4])
                {
                    numHarmonics[channelIndex + 4] = hr->valueint;
                }

                if (iField)
                {
                    harmonics[channelIndex + 4][harmonicIndex] = (float)iField->valuedouble;
                }
                if (phI)
                {
                    harmonics_phases[channelIndex + 4][harmonicIndex] = (float)phI->valuedouble;
                }
            }

            // ��ӡ������Ϣ
            printf("CPU1: Set harmonic - Line: %d, Chn: %d, HN: %d",
                   line->valueint, chn->valueint, hr->valueint);

            if (u)
                printf(", U: %.2f", (float)u->valuedouble);
            if (phU)
                printf(", PhU: %.2f", (float)phU->valuedouble);
            if (iField)
                printf(", I: %.2f", (float)iField->valuedouble);
            if (phI)
                printf(", PhI: %.2f", (float)phI->valuedouble);

            printf("\n");
        }
        else
        {
            // ������ͨ����г������������Χ
            printf("CPU1:Harmonic setting error channel: %d, harmonic number: %d out of range\n",
                   chn->valueint, hr->valueint);
        }
    }

    // ʹ�ø��º��г�����ɲ���
    str_wr_bram(PID_OFF);

    // ׼�������ͻظ�
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetHarm");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

void handle_SetInterHarm(cJSON *data)
{
    // ���� handle_SetInterHarm ���߼�
    //    xil_printf("CPU1: Handling handle_SetInterHarm...\r\n");
}

SetDO setDO;
void handle_SetDO(cJSON *data)
{
    // ���� handle_SetDO ���߼�
    //    xil_printf("CPU1: Handling handle_SetDO...\r\n");

    int dataCount = cJSON_GetArraySize(data);
    for (int i = 0; i < dataCount; i++)
    {
        cJSON *Vals = cJSON_GetArrayItem(data, i);

        setDO.Vals[i].Chn = cJSON_GetObjectItem(Vals, "Chn")->valueint;
        setDO.Vals[i].val = cJSON_GetObjectItem(Vals, "val")->valueint;
    }
    // ��ӡ�����������֤

    for (int i = 0; i < ChnsBO; i++)
    {
        printf("CHn: %d, val: %d\n",
               setDO.Vals[i].Chn,
               setDO.Vals[i].val);
    }

    // ӳ�䵽Ӳ��
    // ������ �´���
    //	Write_Read_Switch(bit_8 , 0xf0000000);//��д������ģ�� ������8��16��24��32λ //8λģʽ�£�0xf0000000Ϊ��4λд1

    // �ر�JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "SetDO");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);
}

void handle_StopDCS(cJSON *data)
{
    // ���� handle_StopDCS ���߼�
    //	xil_printf("CPU1: Handling handle_StopDCS...\r\n");

    // �ر�JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "StopDCS");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false; // ����Ҫ ClosedLoop �ֶ�

    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);
}
void handle_StopAC(cJSON *data)
{
    // ������/Դ�رգ�ͬʱ������л�����г������г�������
    //  ���� handle_StopACS ���߼�
    //    xil_printf("CPU1: Handling handle_StopAC...\r\n");

    // �ر�JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "StopAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false; // ����Ҫ ClosedLoop �ֶ�
    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);

    // ӳ�䵽Ӳ��

    Wave_Frequency = 50;                         // Ƶ��
    memset(Phase_shift, 0, sizeof(Phase_shift)); // ��ջ�����λ

    memset(numHarmonics, 0, sizeof(numHarmonics));         // ���г��
    memset(harmonics, 0, sizeof(harmonics));               // ���г����ֵ
    memset(harmonics_phases, 0, sizeof(harmonics_phases)); // ���г����λ
    enable = 0x00;                                         // �ر�ͨ�����
    str_wr_bram(PID_OFF);                                  // ���ɽ����ź�

    power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
    // ����UDP�ṹ��100DevState������״̬
    devState.bACMeterMode = 0;
    devState.bACRunning = 0;
}

void handle_ClearHarm(cJSON *data)
{
    // ���� handle_ClearHarm ���߼�
    //    xil_printf("CPU1: Handling handle_ClearHarm...\r\n");

    // �ر�JSON
    ReplyData replyData;
    strcpy(replyData.FunCode, "ClearHarm");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false; // ����Ҫ ClosedLoop �ֶ�

    // д��ر�ָ������ڴ�
    write_reply_to_shared_memory(&replyData);

    // ӳ�䵽Ӳ��
    memset(numHarmonics, 0, sizeof(numHarmonics)); // ���г��
    memset(harmonics, 0, sizeof(harmonics));
    memset(harmonics_phases, 0, sizeof(harmonics_phases));
    // ���ɽ����ź�
    str_wr_bram(PID_OFF);
}

void handle_ClearInterHarm(cJSON *data)
{
    // ���� handle_ClearInterHarm ���߼�
    //    xil_printf("CPU1: Handling handle_ClearInterHarm...\r\n");
}

/**
 * @brief ��������У׼������
 *
 * �ú������������У׼����������豸�����Ӧ�ķ�ֵ
 * ֧��100%��20%����У׼�㣬�����������һУ׼���򷵻�ʧ��
 *
 * @param data ����У׼�����ݵ� JSON ����
 */
void handle_setCalibrateAC(cJSON *data)
{
    // ��JSON����ȡУ׼����Ϣ
    cJSON *mode = cJSON_GetObjectItem(data, "Mode");
    // ��ȡ����Ϣ
    cJSON *point = cJSON_GetObjectItem(data, "Point");

    if (!mode || !point)
    {
        printf("CPU1: setCalibrateAC: ȱ�� Mode �� Point ����\r\n");

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "setCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // ��ȡ����Ϣ
    cJSON *ur = cJSON_GetObjectItem(point, "UR");
    cJSON *ir = cJSON_GetObjectItem(point, "IR");
    cJSON *u = cJSON_GetObjectItem(point, "U");
    cJSON *i = cJSON_GetObjectItem(point, "I");

    if (!ur || !ir || !u || !i)
    {
        printf("CPU1: setCalibrateAC: ȱ�ٵ�����ֵ\r\n");

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "setCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    float urValue = (float)ur->valuedouble;
    float irValue = (float)ir->valuedouble;
    float uValue = (float)u->valuedouble;
    float iValue = (float)i->valuedouble;

    // �ж�װ���Ƿ���UR IR�����λ
    // ���帡�����Ƚϵ���Χ
    const float epsilon = 0.01f;

    // ����ѹ��λ
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

    // ��������λ
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
        printf("CPU1: setCalibrateAC: ��Ч�� UR �� IR ��λֵ\r\n");
        printf("CPU1: UR=%f (��Чֵ: 6.5, 3.25, 1.876), IR=%f (��Чֵ: 5.0, 1.0, 0.2)\r\n",
               urValue, irValue);

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "setCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // ����Ƿ�Ϊ100%��20%У׼��
    bool is100PercentPoint = fabs(uValue - urValue) < epsilon && fabs(iValue - irValue) < epsilon;
    bool is20PercentPoint = fabs(uValue - urValue * 0.2f) < epsilon && fabs(iValue - irValue * 0.2f) < epsilon;

    if (!is100PercentPoint && !is20PercentPoint)
    {
        printf("CPU1: setCalibrateAC: ������Ч��У׼��\r\n");
        printf("CPU1: У׼�������100%%��(U=%f, I=%f)��20%%��(U=%f, I=%f)\r\n",
               urValue, irValue, urValue * 0.2f, irValue * 0.2f);

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "setCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    printf("CPU1: setCalibrateAC: Set calibration points: UR=%f, IR=%f, U=%f, I=%f (Calibration point type: %s)\r\n",
           urValue, irValue, uValue, iValue, is100PercentPoint ? "100%" : "20%");

    // ��������ͨ�������
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        setACS.Vals[i].UR = urValue;
        setACS.Vals[i].IR = irValue;
        setACS.Vals[i].U = uValue;
        setACS.Vals[i].I_ = iValue;
    }

    // ���²��η��Ⱥͷ�Χ
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = (float)(setACS.Vals[i].U / setACS.Vals[i].UR) * 100;
        Wave_Amplitude[i + 4] = (float)(setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }
    // Ӧ������
    str_wr_bram(PID_OFF);
    power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_ON);

    // ���سɹ���Ӧ
    ReplyData replyData;
    strcpy(replyData.FunCode, "setCalibrateAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

/**
 * @brief �����дУ׼���׼ֵ����
 *
 * �ú��������趨ֵ��ʵ��ֵ����У׼����
 * ֧��100%��20%����У׼�㣬�Լ���λУ׼
 *
 * @param data ����У׼���ݵ� JSON ����
 */
void handle_writeCalibrateAC(cJSON *data)
{
    // ��JSON����ȡУ׼��Ϣ
    cJSON *mode = cJSON_GetObjectItem(data, "Mode");
    cJSON *point = cJSON_GetObjectItem(data, "Point");
    cJSON *chns = cJSON_GetObjectItem(data, "Chns");

    if (!mode || !point || !chns)
    {
        printf("CPU1: writeCalibrateAC: ȱ�� Mode, Point �� Chns \r\n");

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "writeCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // ��ȡ����Ϣ
    cJSON *ur = cJSON_GetObjectItem(point, "UR");
    cJSON *ir = cJSON_GetObjectItem(point, "IR");
    cJSON *u = cJSON_GetObjectItem(point, "U");
    cJSON *i = cJSON_GetObjectItem(point, "I");

    if (!ur || !ir || !u || !i)
    {
        printf("CPU1: writeCalibrateAC: ȱ������\r\n");

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "writeCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    float urValue = (float)ur->valuedouble;
    float irValue = (float)ir->valuedouble;
    float uValue = (float)u->valuedouble;
    float iValue = (float)i->valuedouble;

    // ��ȡУ׼ͨ������
    int chnsCount = cJSON_GetArraySize(chns);
    if (chnsCount <= 0)
    {
        printf("CPU1: writeCalibrateAC: δ�ṩͨ������\r\n");

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "writeCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    // ��ȡ��ѹ�͵�����λ����
    int ur_idx = get_voltage_index_by_value(urValue);
    int ir_idx = get_current_index_by_value(irValue);

    // ����Ƿ�Ϊ100%��20%У׼��
    const float epsilon = 0.01f;
    bool is100PercentPoint = fabs(uValue - urValue) < epsilon && fabs(iValue - irValue) < epsilon;
    bool is20PercentPoint = fabs(uValue - urValue * 0.2f) < epsilon && fabs(iValue - irValue * 0.2f) < epsilon;

    if (!is100PercentPoint && !is20PercentPoint)
    {
        printf("CPU1: writeCalibrateAC: ������Ч��У׼��\r\n");
        printf("CPU1: У׼�������100%%��(U=%f, I=%f)��20%%��(U=%f, I=%f)\r\n",
               urValue, irValue, urValue * 0.2f, irValue * 0.2f);

        // ����ʧ����Ӧ
        ReplyData replyData;
        strcpy(replyData.FunCode, "writeCalibrateAC");
        strcpy(replyData.Result, "Failure");
        replyData.hasClosedLoop = false;
        write_reply_to_shared_memory(&replyData);
        return;
    }

    printf("CPU1: writeCalibrateAC: Calibration point UR=%f, IR=%f, U=%f, I=%f (Calibration point type: %s)\r\n",
           urValue, irValue, uValue, iValue, is100PercentPoint ? "100%" : "20%");

    // ��ÿ��ͨ������У׼
    for (int j = 0; j < chnsCount; j++)
    {
        cJSON *chn = cJSON_GetArrayItem(chns, j);

        if (!chn)
            continue; // ������Ч��

        cJSON *line_json = cJSON_GetObjectItem(chn, "Line");
        cJSON *channel_json = cJSON_GetObjectItem(chn, "Chn");
        cJSON *actual_u_json = cJSON_GetObjectItem(chn, "U");
        cJSON *actual_i_json = cJSON_GetObjectItem(chn, "I");
        cJSON *actual_phu_json = cJSON_GetObjectItem(chn, "PhU");
        cJSON *actual_phi_json = cJSON_GetObjectItem(chn, "PhI");

        if (!line_json || !channel_json || !actual_u_json || !actual_i_json)
        {
            printf("CPU1: writeCalibrateAC: ͨ�� %d ȱ������\r\n", j);
            continue; // ������Ч��
        }

        int line = line_json->valueint;
        int channel = channel_json->valueint;
        float actualU = (float)actual_u_json->valuedouble;
        float actualI = (float)actual_i_json->valuedouble;
        float actualPhU = actual_phu_json ? (float)actual_phu_json->valuedouble : 0.0f;
        float actualPhI = actual_phi_json ? (float)actual_phi_json->valuedouble : 0.0f;

        // ӳ��ͨ������
        int u_idx = channel - 1;     // 0-3 ��Ӧ UA, UB, UC, UX
        int i_idx = channel - 1 + 4; // 4-7 ��Ӧ IA, IB, IC, IX

        printf("CPU1: writeCalibrateAC: Line=%d, Chn=%d, actualU=%f, actualI=%f, actualPhU=%f, actualPhI=%f\r\n",
               line, channel, actualU, actualI, actualPhU, actualPhI);

        if (is100PercentPoint)
        {
            // У׼100%��ĵ�ѹ����
            if (actualU > 0.001) // ������Խӽ����ֵ
            {
                float ratio = uValue / actualU;
                DA_Correct_100[u_idx][ur_idx] = DA_Correct_100[u_idx][ur_idx] * ratio;
                printf("CPU1: Update voltage calibration parameters DA_Correct_100[%d][%d]: %f\r\n",
                       u_idx, ur_idx, DA_Correct_100[u_idx][ur_idx]);
            }

            // У׼100%��ĵ�������
            if (actualI > 0.001) // ������Խӽ����ֵ
            {
                float ratio = iValue / actualI;
                DA_Correct_100[i_idx][ir_idx] = DA_Correct_100[i_idx][ir_idx] * ratio;
                printf("CPU1: Update current calibration parameters DA_Correct_100[%d][%d]: %f\r\n",
                       i_idx, ir_idx, DA_Correct_100[i_idx][ir_idx]);
            }

            // У׼��λ���� (ֻ��100%��У׼��λ)
            if (actual_phu_json)
            {
                // �����ѹ��λУ׼����: �趨��λ - ʵ����λ
                float phaseOffset = Phase_shift[u_idx] - actualPhU;
                DA_CorrectPhase_100[u_idx][ur_idx] = phaseOffset;
                printf("CPU1: Update voltage phase calibration parameters DA_CorrectPhase_100[%d][%d]: %f\r\n",
                       u_idx, ur_idx, DA_CorrectPhase_100[u_idx][ur_idx]);
            }

            if (actual_phi_json)
            {
                // ���������λУ׼����: �趨��λ - ʵ����λ
                float phaseOffset = Phase_shift[i_idx] - actualPhI;
                DA_CorrectPhase_100[i_idx][ir_idx] = phaseOffset;
                printf("CPU1: Update current phase calibration parameters DA_CorrectPhase_100[%d][%d]: %f\r\n",
                       i_idx, ir_idx, DA_CorrectPhase_100[i_idx][ir_idx]);
            }
        }
        else if (is20PercentPoint)
        {
            // У׼20%��ĵ�ѹ����
            if (actualU > 0.001) // ������Խӽ����ֵ
            {
                float ratio = (uValue / actualU);
                DA_Correct_20[u_idx][ur_idx] = DA_Correct_20[u_idx][ur_idx] * ratio;
                printf("CPU1: Update voltage calibration parameters DA_Correct_20[%d][%d]: %f\r\n",
                       u_idx, ur_idx, DA_Correct_20[u_idx][ur_idx]);
            }

            // У׼20%��ĵ�������
            if (actualI > 0.001) // ������Խӽ����ֵ
            {
                float ratio = (iValue / actualI);
                DA_Correct_20[i_idx][ir_idx] = DA_Correct_20[i_idx][ir_idx] * ratio;
                printf("CPU1: Update current calibration parameters DA_Correct_20[%d][%d]: %f\r\n",
                       i_idx, ir_idx, DA_Correct_20[i_idx][ir_idx]);
            }
        }
    }

    // Ӧ�ø��º��У׼����
    // ���²��η��Ⱥͷ�Χ
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = (float)(setACS.Vals[i].U / setACS.Vals[i].UR) * 100;
        Wave_Amplitude[i + 4] = (float)(setACS.Vals[i].I_ / setACS.Vals[i].IR) * 100;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }

    // Ӧ������
    str_wr_bram(PID_OFF);
    power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_ON);

    // ��У׼�������浽EEPROM
    RC64_WriteCalibData();

    // ���سɹ���Ӧ
    ReplyData replyData;
    strcpy(replyData.FunCode, "writeCalibrateAC");
    strcpy(replyData.Result, "Success");
    replyData.hasClosedLoop = false;
    write_reply_to_shared_memory(&replyData);
}

// ����JSON�ַ�����д�빲���ڴ�ĺ���
void write_reply_to_shared_memory(ReplyData *replyData)
{

    // ȷ��д����ɣ�ʹ���ڴ�����
    __sync_synchronize();
    // ����JSON����
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "FunType", "Reply");
    cJSON_AddStringToObject(json, "FunCode", replyData->FunCode);
    cJSON_AddStringToObject(json, "Result", replyData->Result);

    // ����Data����
    cJSON *data = cJSON_CreateObject();
    if (replyData->hasClosedLoop)
    {
        cJSON_AddBoolToObject(data, "ClosedLoop", replyData->ClosedLoop);
    }
    cJSON_AddItemToObject(json, "Data", data);

    // ��JSON����ת��Ϊ�ַ���
    char *jsonString = cJSON_PrintUnformatted(json);
    if (jsonString == NULL)
    {
        xil_printf("Failed to print JSON.\n");
        cJSON_Delete(json);
        return;
    }

    // ���ַ�����β����'|'�ַ�
    size_t jsonStringLength = strlen(jsonString);
    char *finalString = (char *)malloc(jsonStringLength + 3); // +3 ��Ϊ��β '|' ����ֹ�� '\0'
    if (finalString == NULL)
    {
        xil_printf("Failed to allocate memory for final JSON string.\n");
        cJSON_free(jsonString);
        cJSON_Delete(json);
        return;
    }
    snprintf(finalString, jsonStringLength + 3, "|%s|", jsonString);

    // ���ַ���д�빲���ڴ�
    ssize_t bytesWritten = MsgQue_write(finalString, strlen(finalString));
    if (bytesWritten < 0)
    {
        xil_printf("CPU1:Failed to write to message queue: %ld\n", bytesWritten);
    }
    else
    {
        //        xil_printf("CPU1:Successfully wrote %ld bytes to message queue: %s\n", bytesWritten, jsonString);
    }

    // �ͷ�JSON�ַ���
    free(finalString);
    cJSON_free(jsonString);
    // ɾ��JSON����
    cJSON_Delete(json);
}

/**
 * @brief ���汣���¼�
 *
 * ��������Ĺ�����Ϣ�����ɲ����汣���¼���JSON����
 *
 * @param ProectFault u8���ͣ�����������Ϣ���ֽڡ�ÿһλ����һ�����ܵĹ��ϣ�
 *                  0��ʾ���ϴ��ڣ�1��ʾ�޹��ϡ�
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

    // Check for IOPEN
    if ((ProectFault & 0x0F) != 0x0F)
    {
        cJSON *IOPEN = cJSON_CreateArray();

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
                cJSON_AddItemToArray(IOPEN, fault);
            }
        }

        if (cJSON_GetArraySize(IOPEN) > 0)
        {
            cJSON_AddItemToObject(data, "IOPEN", IOPEN);
        }
        else
        {
            cJSON_Delete(IOPEN);
        }
    }

    // Check for USHORT
    if ((ProectFault & 0xF0) != 0xF0)
    {
        cJSON *USHORT = cJSON_CreateArray();

        for (int i = 0; i < 4; i++)
        {
            if ((ProectFault & (1 << (i + 4))) == 0) // Bit is 0 means fault
            {
                cJSON *fault = cJSON_CreateObject();
                cJSON_AddStringToObject(fault, "Type", "AC");
                cJSON_AddNumberToObject(fault, "Line", 1);

                int channelMap[4] = {4, 3, 2, 1}; // UX UC UB UA
                cJSON_AddNumberToObject(fault, "Chn", channelMap[i]);
                cJSON_AddItemToArray(USHORT, fault);
            }
        }

        if (cJSON_GetArraySize(USHORT) > 0)
        {
            cJSON_AddItemToObject(data, "USHORT", USHORT);
        }
        else
        {
            cJSON_Delete(USHORT);
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

    // ��ӡ���յ�JSON�ַ������ڲ���
    // xil_printf("CPU1: Protection event JSON: %s\n", finalString);

    // Ӳ������
    //  �ر�DA
    Wave_Frequency = 50;                                   // Ƶ��
    memset(Phase_shift, 0, sizeof(Phase_shift));           // ��ջ�����λ
    enable = 0x00;                                         // �ر�ͨ�����
    memset(numHarmonics, 0, sizeof(numHarmonics));         // ���г��
    memset(harmonics, 0, sizeof(harmonics));               // ���г����ֵ
    memset(harmonics_phases, 0, sizeof(harmonics_phases)); // ���г����λ
    str_wr_bram(PID_OFF);                                  // ���ɽ����ź�

    // �رչ���
    // �޸Ķ���DA ���η��� ����
    for (int i = 0; i < 4; i++)
    {
        Wave_Amplitude[i] = 0.0;
        Wave_Amplitude[i + 4] = 0.0;
        Wave_Range[i] = voltage_to_output(setACS.Vals[i].UR);
        Wave_Range[i + 4] = current_to_output(setACS.Vals[i].IR);
    }
    power_amplifier_control(Wave_Amplitude, Wave_Range, PID_OFF, POWAMP_OFF);
    // ����UDP�ṹ��100DevState������״̬
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

    // ���ַ���д�빲���ڴ�

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
 * �ر�UDP�ṹ�嶥�㺯��
 */
// ��ʼ��ʹ��״̬�ṹ��
DevState devState;
LineAC lineAC;
LineHarm lineHarm;
LineDI lineDI;
LineDO lineDO;
LineDisoe lineDisoe;
void ReportUDP_Structure(ReportEnableStatus ReportStatus)
{
    UDPPacket udpPacket;

    // ��̬���� payload ��С
    size_t dynamic_payload_size = calculate_dynamic_payload_size(ReportStatus);

    /*дUDP�ṹ��֡ͷ��*/
    memcpy(udpPacket.syncHeader, (char[]){0xD1, 0xD2, 0xD3, 0xD4}, 4); // ͬ��ͷ
    udpPacket.dataLength1 = dynamic_payload_size;                      // �������ܳ���
    udpPacket.dataLength2 = dynamic_payload_size;                      // �ظ�һ�� �������ܳ���
    udpPacket.versionInfo = 1;                                         // �汾��Ϣ
    memset(udpPacket.reserved, 0, 3);                                  // ����

    /*���������*/
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

    /* дUDP֡β*/
    memcpy(payload_ptr, (char[]){0xE1, 0xE2, 0xE3, 0xE4}, 4); // ������
    payload_ptr += 4 * sizeof(u8);

    // д UDP ���ݵ������ڴ�
    // ���ù����ڴ����һ���ֽ�Ϊ 1�����ռ��
    uint32_t last_byte_addr = UDP_ADDRESS + UDP_MEM_SIZE - 4; // �����ڴ����һ���ֵ�ַ
    Xil_Out32(last_byte_addr, 1);
    Xil_DCacheFlushRange((INTPTR)last_byte_addr, sizeof(u32));

    // дUDP�������ڴ�
    // ����Ƿ񳬳�����
    if (16 + dynamic_payload_size + 4 > UDP_MEM_SIZE)
    {
        printf("CPU1: ERROR - UDP data too large for shared memory: %zu > %d\n",
               16 + dynamic_payload_size + 4, UDP_MEM_SIZE);
        return; // ����̫��ֱ�ӷ��ز�д��
    }

    write_UDP_to_shared_memory(UDP_ADDRESS, &udpPacket, 16 + dynamic_payload_size + 4);

    // ���ù����ڴ����һ���ֽ�Ϊ 0����ǿ���
    Xil_Out32(last_byte_addr, 0);
    Xil_DCacheFlushRange((INTPTR)last_byte_addr, sizeof(u32));

    // ˢ������UDP
    Xil_DCacheFlushRange((UINTPTR)&udpPacket, sizeof(udpPacket));
    // ��ӡ������Ϣ
    // printf("lineAC size: %zu bytes\n", sizeof(LineAC));
    // printf("dynamic_payload_size: %zu bytes\n", dynamic_payload_size);
    // printf("UDP_PACKET_SIZE: %d bytes\n", sizeof(udpPacket));
    // printf("CPU1: UDP written to memory at: 0x%X\n", UDP_ADDRESS);
}

// Function to initialize DevState
void initDevState(DevState *devState)
{
    devState->bACMeterMode = 0; // 0=����Դ״̬;1=������״̬
    devState->bACRunning = 0;   // 0=ֹͣ״̬;1=����״̬    //Ĭ��ֹͣ
    devState->bClosedLoop = 1;  // 0=����״̬;1=�ջ�״̬    //Ĭ�ϱջ�
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
    u32 *data_ptr = (u32 *)data;
    size_t words = size / 4;

    for (size_t i = 0; i < words; i++)
    {
        Xil_Out32(base_addr + (i * 4), data_ptr[i]);
    }
    Xil_DCacheFlushRange((INTPTR)UDP_ADDRESS, size);
}

// ��ʼ��setACS�ṹ��
void init_setACS()
{
    // ��������ͨ��
    for (int i = 0; i < LinesAC * ChnsAC; i++)
    {
        // ������·��ź�ͨ�����
        setACS.Vals[i].Line = i / ChnsAC + 1; // ��·��Ŵ�1��ʼ
        setACS.Vals[i].Chn = i % ChnsAC + 1;  // ͨ����Ŵ�1��ʼ

        // ����Ƶ��Ϊ50Hz
        setACS.Vals[i].F = 50.0f;

        // ���õ�ѹ��ز���
        setACS.Vals[i].UR = 6.5f;  // ��ѹ��λ
        setACS.Vals[i].U = 0.0f;   // ��ѹ��ֵ
        setACS.Vals[i].PhU = 0.0f; // ��ѹ��λ

        // ���õ�����ز���
        setACS.Vals[i].IR = 5.0f;  // ������λ
        setACS.Vals[i].I_ = 0.0f;  // ������ֵ
        setACS.Vals[i].PhI = 0.0f; // ������λ
    }

    // ��ӡ��ʼ�������Ϣ
    // printf("CPU1: setACS.Vals initialization completed.\n");
}

void init_JsonUdp(void)
{
    //  ��ʼ��JSON�ṹ��
    init_setACS();

    // ��ʼ�����ݽṹ��
    initDevState(&devState);
    initLineAC(&lineAC);
    initLineHarm(&lineHarm);
    initLineDI(&lineDI);
    initLineDO(&lineDO);
    initLineDisoe(&lineDisoe);
}
