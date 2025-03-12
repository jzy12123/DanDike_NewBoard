#include "Msg_que.h"


// ������ʵ��
ssize_t MsgQue_read(char *buf, size_t cnt) {
    uint32_t msg_len = 0;
    // header��
    uint32_t LinuxSendedMsgID = Xil_In32(Linux_Header_Offset);
    uint32_t RtosRecvedMsgID = Xil_In32(Rtos_RecvedMsgID_Offset);

    /*1 ��������Ϣ����*/
    uint32_t new_msg_count = LinuxSendedMsgID - RtosRecvedMsgID;

    while(new_msg_count > 100){//���������100����Ϣû�����Ͳ�Ҫ�ˡ�
        RtosRecvedMsgID += 100;
        Xil_Out32(Rtos_RecvedMsgID_Offset, RtosRecvedMsgID);
        new_msg_count = LinuxSendedMsgID - RtosRecvedMsgID;
    }

    if(new_msg_count > 0){
    	/*2 ��������Ϣ*/
    	uint32_t msg_index = RtosRecvedMsgID % QUEUE_TABLE_SIZE; //��������Ϣ����
    	msg_len = (uint32_t)Xil_In32(Linux_MsgLen_Offset(msg_index)); // ��ȡ��Ϣ����
        if (msg_len > 0 && msg_len < cnt) {
        	uint32_t Linux_data_addr = Linux_Data_Offset + MAX_DATA_LEN * msg_index;
        	memcpy(buf, (void *)Linux_data_addr, msg_len);

        }
        else if (msg_len > cnt){
        	printf("CPU1:Error Message too large\r\n");
        }
        /*3 ����rtos������Ϣ��*/
        RtosRecvedMsgID += 1;
        Xil_Out32(Rtos_RecvedMsgID_Offset, RtosRecvedMsgID);
        // ˢ�»���
        Xil_DCacheFlushRange((INTPTR)Linux_WriteMemorySpace, sizeof(struct msg_queue));

        /*����*/
        printf("CPU1: RtosRecvedMsgID=%d\r\n",(int)RtosRecvedMsgID);
    }

    /*4 ����*/
    return msg_len;

}

// д����ʵ��
ssize_t MsgQue_write(const char *buf, size_t cnt) {
    if (cnt > MAX_DATA_LEN) {
        return -1; // ���ݹ���
    }
    // �ӹ����ڴ��ȡHeader
    uint32_t RtosSendedMsgID = Xil_In32(Rtos_Header_Offset);

    /*1 ��������Ϣ����*/
    uint32_t index = RtosSendedMsgID  % QUEUE_TABLE_SIZE;

    /*2 д�ڴ�׶�*/
    // 2.1 ����Data��
    uint32_t data_addr = Rtos_Data_Offset + MAX_DATA_LEN * index;
    memset((void *)data_addr, 0, MAX_DATA_LEN);
    memcpy((void *)data_addr, buf, cnt);
    // 2.2 ����Table��
    memset((void *)Rtos_MsgAddr_Offset(index), 0, sizeof(uint32_t));
    memset((void *)Rtos_MsgLen_Offset(index), 0, sizeof(uint32_t));
	Xil_Out32((uint32_t)Rtos_MsgAddr_Offset(index), data_addr);  	// MsgAddr
	Xil_Out32((uint32_t)Rtos_MsgLen_Offset(index), cnt);     		// MsgLen
	// 2.3 ����Header��
    RtosSendedMsgID++;
    Xil_Out32(Rtos_Header_Offset, RtosSendedMsgID); // ����SendedMsgID
    // ˢ�»���
    Xil_DCacheFlushRange((INTPTR)Rtos_WriteMemorySpace, sizeof(struct msg_queue));

    /*3 ����*/
    return cnt; // ����д����ֽ���
}

// ��ʼ������
void InitializeQueues() {
    // ��Linux��RTOS���ڴ����ݳ�ʼ��Ϊ0
    for (uint32_t i = 0; i < (10 * 1024 * 1024 / sizeof(uint32_t)); i++) {
        Xil_Out32(Linux_WriteMemorySpace + i * sizeof(uint32_t), 0);
        Xil_Out32(Rtos_WriteMemorySpace + i * sizeof(uint32_t), 0);
    }
    Xil_DCacheFlushRange((INTPTR)Linux_WriteMemorySpace, 0xA00000);
    Xil_DCacheFlushRange((INTPTR)Rtos_WriteMemorySpace, 0xA00000);
}

