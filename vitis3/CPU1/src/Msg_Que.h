#ifndef MSG_QUE_H
#define MSG_QUE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "xil_cache.h"
#include "xil_io.h" // 包含 Xilinx IO 函数

#define QUEUE_TABLE_SIZE   100
#define MAX_DATA_LEN       40960
/*  消息队列地址定义*/
#define Linux_WriteMemorySpace         0x3AC00000      // 10MB
#define Rtos_WriteMemorySpace           0x3B600000      // 10MB

#define Linux_Header_Offset            (Linux_WriteMemorySpace)
#define Linux_RecvedMsgID_Offset       (Linux_WriteMemorySpace + sizeof(uint32_t))
#define Linux_QueueEntry_Offset        (Linux_WriteMemorySpace + sizeof(struct msg_queue_header))
#define Linux_Data_Offset              (Linux_WriteMemorySpace + sizeof(struct msg_queue_header) + sizeof(struct msg_queue_entry) * QUEUE_TABLE_SIZE)
#define Linux_MsgAddr_Offset(index)    (Linux_QueueEntry_Offset + sizeof(struct msg_queue_entry) * (index))
#define Linux_MsgLen_Offset(index)     (Linux_MsgAddr_Offset(index) + sizeof(uint32_t))


#define Rtos_Header_Offset             Rtos_WriteMemorySpace
#define Rtos_RecvedMsgID_Offset        (Rtos_WriteMemorySpace + sizeof(uint32_t))
#define Rtos_QueueEntry_Offset         (Rtos_WriteMemorySpace + sizeof(struct msg_queue_header))
#define Rtos_Data_Offset               (Rtos_WriteMemorySpace + sizeof(struct msg_queue_header) + sizeof(struct msg_queue_entry) * QUEUE_TABLE_SIZE)
#define Rtos_MsgAddr_Offset(index)     (Rtos_QueueEntry_Offset + sizeof(struct msg_queue_entry) * (index))
#define Rtos_MsgLen_Offset(index)      (Rtos_MsgAddr_Offset(index) + sizeof(uint32_t))


struct msg_queue_header {
    uint32_t SendedMsgID;
    uint32_t RecvedMsgID;
};

struct msg_queue_entry {
    uint32_t MsgAddr;
    uint32_t MsgLen;
};

struct msg_queue {
    struct msg_queue_header header;
    struct msg_queue_entry queue_table[QUEUE_TABLE_SIZE];
    char data[QUEUE_TABLE_SIZE][MAX_DATA_LEN];
};

ssize_t MsgQue_read(char *buf, size_t cnt);
ssize_t MsgQue_write(const char *buf, size_t cnt);
void InitializeQueues();


#endif
