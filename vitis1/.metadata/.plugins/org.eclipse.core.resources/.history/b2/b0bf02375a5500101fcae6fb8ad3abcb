#include "Msg_que.h"


// 读函数实现
ssize_t MsgQue_read(char *buf, size_t cnt) {
    uint32_t msg_len = 0;
    // header段
    uint32_t LinuxSendedMsgID = Xil_In32(Linux_Header_Offset);
    uint32_t RtosRecvedMsgID = Xil_In32(Rtos_RecvedMsgID_Offset);

    /*1 计算新消息数量*/
    uint32_t new_msg_count = LinuxSendedMsgID - RtosRecvedMsgID;

    while(new_msg_count > 100){//如果连续有100条消息没读，就不要了。
        RtosRecvedMsgID += 100;
        Xil_Out32(Rtos_RecvedMsgID_Offset, RtosRecvedMsgID);
        new_msg_count = LinuxSendedMsgID - RtosRecvedMsgID;
    }

    if(new_msg_count > 0){
    	/*2 读回新消息*/
    	uint32_t msg_index = RtosRecvedMsgID % QUEUE_TABLE_SIZE; //计算新消息索引
    	msg_len = (uint32_t)Xil_In32(Linux_MsgLen_Offset(msg_index)); // 获取消息长度
        if (msg_len > 0 && msg_len < cnt) {
        	uint32_t Linux_data_addr = Linux_Data_Offset + MAX_DATA_LEN * msg_index;
        	memcpy(buf, (void *)Linux_data_addr, msg_len);

        }
        else if (msg_len > cnt){
        	printf("CPU1:Error Message too large\r\n");
        }
        /*3 更新rtos接收消息数*/
        RtosRecvedMsgID += 1;
        Xil_Out32(Rtos_RecvedMsgID_Offset, RtosRecvedMsgID);
        // 刷新缓存
        Xil_DCacheFlushRange((INTPTR)Linux_WriteMemorySpace, sizeof(struct msg_queue));

        /*调试*/
        printf("CPU1: RtosRecvedMsgID=%d\r\n",(int)RtosRecvedMsgID);
    }

    /*4 结束*/
    return msg_len;

}

// 写函数实现
ssize_t MsgQue_write(const char *buf, size_t cnt) {
    if (cnt > MAX_DATA_LEN) {
        return -1; // 数据过大
    }
    // 从共享内存读取Header
    uint32_t RtosSendedMsgID = Xil_In32(Rtos_Header_Offset);

    /*1 计算新消息索引*/
    uint32_t index = RtosSendedMsgID  % QUEUE_TABLE_SIZE;

    /*2 写内存阶段*/
    // 2.1 更新Data段
    uint32_t data_addr = Rtos_Data_Offset + MAX_DATA_LEN * index;
    memset((void *)data_addr, 0, MAX_DATA_LEN);
    memcpy((void *)data_addr, buf, cnt);
    // 2.2 更新Table段
    memset((void *)Rtos_MsgAddr_Offset(index), 0, sizeof(uint32_t));
    memset((void *)Rtos_MsgLen_Offset(index), 0, sizeof(uint32_t));
	Xil_Out32((uint32_t)Rtos_MsgAddr_Offset(index), data_addr);  	// MsgAddr
	Xil_Out32((uint32_t)Rtos_MsgLen_Offset(index), cnt);     		// MsgLen
	// 2.3 更新Header段
    RtosSendedMsgID++;
    Xil_Out32(Rtos_Header_Offset, RtosSendedMsgID); // 更新SendedMsgID
    // 刷新缓存
    Xil_DCacheFlushRange((INTPTR)Rtos_WriteMemorySpace, sizeof(struct msg_queue));

    /*3 结束*/
    return cnt; // 返回写入的字节数
}

// 初始化队列
void InitializeQueues() {
    // 将Linux和RTOS的内存内容初始化为0
    for (uint32_t i = 0; i < (10 * 1024 * 1024 / sizeof(uint32_t)); i++) {
        Xil_Out32(Linux_WriteMemorySpace + i * sizeof(uint32_t), 0);
        Xil_Out32(Rtos_WriteMemorySpace + i * sizeof(uint32_t), 0);
    }
    Xil_DCacheFlushRange((INTPTR)Linux_WriteMemorySpace, 0xA00000);
    Xil_DCacheFlushRange((INTPTR)Rtos_WriteMemorySpace, 0xA00000);
}

