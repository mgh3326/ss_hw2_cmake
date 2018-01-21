//
// Created by moon on 1/21/18.
//

#ifndef UNTITLED5_MSGQUEUE_H
#define UNTITLED5_MSGQUEUE_H

#include "Thread.h"

#define MAX_MSG_SIZE (1024)
#define MAX_QCB_SIZE (32)

#define MY_IPC_RMID (100)

typedef struct _Message Message;
typedef struct _Message
{
    long type;
    char data[MAX_MSG_SIZE];
    int size;
    Message *pPrev;
    Message *pNext;
} Message;

typedef struct _Qcb
{
    int msgCount;
    Message *pMsgHead;
    Message *pMsgTail;
    int waitThreadCount;
    Thread *pThreadHead;
    Thread *pThreadTail;
} Qcb;

typedef struct _QcbTblEntry
{
    int key;
    Qcb *pQcb;
} QcbTblEntry;

QcbTblEntry qcbTblEntry[MAX_QCB_SIZE];

void _InitMsgQueue(void);
int mymsgget(int key, int msgflg);
int mymsgsnd(int msqid, const void *msgp, int msgsz, int msgflg);
int mymsgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
int mymsgctl(int msqid, int cmd, void *buf);

#endif //UNTITLED5_MSGQUEUE_H
