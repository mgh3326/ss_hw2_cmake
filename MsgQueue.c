#include "Thread.h"
#include "MsgQueue.h"
#include "my.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0
/* 반드시 구현할 필요는 없음. 만일 구현했다면, Init.c에 있는 Init()에 추가해야 함.*/
void _InitMsgQueue(void)
{
    for (int i = 0; i < MAX_QCB_SIZE; i++)
    {
        qcbTblEntry[i].key = -1;
    }
}

int mymsgget(int key, int msgflg)
{
    //printf("mymsgget 시작 (%d)",key);
    for (int i = 0; i < MAX_QCB_SIZE; i++)
        if (key == qcbTblEntry[i].key)
            return i;

    for (int i = 0; i < MAX_QCB_SIZE; i++)
    {
        if (qcbTblEntry[i].key == -1)
        {
            qcbTblEntry[i].key = key;
            Qcb *p = malloc(1 * sizeof *p);
            p->pMsgHead = NULL;
            p->pMsgTail = NULL;
            // Message *d = qcbTblEntry[i].pQcb->pMsgHead;

            p->pThreadHead = NULL;
            p->pThreadTail = NULL;
            p->msgCount = 0;
            p->waitThreadCount = 0;
            qcbTblEntry[i].pQcb = p;

            return i;
        }
    }

    return 0;
}

int mymsgsnd(int msqid, const void *msgp, int msgsz, int msgflg)
{

    Message *p = malloc(1 * sizeof *p);
    p->size = msgsz;
    p->type = ((Message *)msgp)->type;
    for (int i = 0; i < msgsz; i++)
    {
        p->data[i] = ((Message *)msgp)->data[i];
    }

    if (qcbTblEntry[msqid].pQcb->pMsgHead == NULL && qcbTblEntry[msqid].pQcb->pMsgTail == NULL)
    {
        p->pPrev = p->pNext = NULL;
        qcbTblEntry[msqid].pQcb->pMsgHead = qcbTblEntry[msqid].pQcb->pMsgTail = p;
    }
    else if (NULL == qcbTblEntry[msqid].pQcb->pMsgHead || NULL == qcbTblEntry[msqid].pQcb->pMsgTail)
    {

        return -1;
    }
    else
    {
        p->pPrev = p->pNext = NULL;
        qcbTblEntry[msqid].pQcb->pMsgTail->pNext = p;
        p->pPrev = qcbTblEntry[msqid].pQcb->pMsgTail;
        qcbTblEntry[msqid].pQcb->pMsgTail = p;
    }

    if (qcbTblEntry[msqid].pQcb != NULL)
    {
        Thread *q = qcbTblEntry[msqid].pQcb->pThreadHead;
        while (q)
        {

            if (q->type == p->type)
            { //레디큐로 다시 올려줘야되

                if (NULL == ReadyQHead && NULL == ReadyQTail)
                {
                    //Wait_remove_element(getThread_wait(tid));

                    if (NULL == q->pNext && (NULL == qcbTblEntry[msqid].pQcb->pThreadHead->pNext && NULL == qcbTblEntry[msqid].pQcb->pThreadTail->pNext)) /* only one element in queue */
                    {
                        qcbTblEntry[msqid].pQcb->pThreadHead = qcbTblEntry[msqid].pQcb->pThreadTail = NULL;
                    }
                    else if ((NULL == q->pNext) && q->pPrev) /* removing pTail */
                    {
                        qcbTblEntry[msqid].pQcb->pThreadTail = q->pPrev;
                        q->pPrev->pNext = NULL;
                    }
                    else if (q->pNext && (NULL == q->pPrev)) /* removing qcbTblEntry[msqid].pQcb->pThreadHead */
                    {
                        qcbTblEntry[msqid].pQcb->pThreadHead = q->pNext;
                        qcbTblEntry[msqid].pQcb->pThreadHead->pPrev = NULL;
                    }
                    else /* removing from center or somewhere */
                    {
                        q->pPrev->pNext = q->pNext;
                        q->pNext->pPrev = q->pPrev;
                    }

                    ReadyQHead = ReadyQTail = q;
                    q->pNext = q->pPrev = NULL;

                    ReadyQHead->status = THREAD_STATUS_READY;
                    ReadyQHead->bRunnable = 0;
                }
                else if (NULL == ReadyQHead || NULL == ReadyQTail)
                {
                    return -1;
                }
                else
                {
                    //Wait_remove_element(getThread_wait(tid));//삭제하는 부분

                    if (NULL == q->pNext && (NULL == qcbTblEntry[msqid].pQcb->pThreadHead->pNext && NULL == qcbTblEntry[msqid].pQcb->pThreadTail->pNext)) /* only one element in queue */
                    {
                        qcbTblEntry[msqid].pQcb->pThreadHead = qcbTblEntry[msqid].pQcb->pThreadTail = NULL;
                    }
                    else if ((NULL == q->pNext) && q->pPrev) /* removing pTail */
                    {
                        qcbTblEntry[msqid].pQcb->pThreadTail = q->pPrev;
                        q->pPrev->pNext = NULL;
                    }
                    else if (q->pNext && (NULL == q->pPrev)) /* removing qcbTblEntry[msqid].pQcb->pThreadHead */
                    {
                        qcbTblEntry[msqid].pQcb->pThreadHead = q->pNext;
                        qcbTblEntry[msqid].pQcb->pThreadHead->pPrev = NULL;
                    }
                    else /* removing from center or somewhere */
                    {
                        q->pPrev->pNext = q->pNext;
                        q->pNext->pPrev = q->pPrev;
                    }

                    ReadyQTail->pNext = q;
                    q->pPrev = ReadyQTail;
                    ReadyQTail = q;
                    ReadyQTail->pNext = NULL;

                    ReadyQTail->status = THREAD_STATUS_READY;
                    ReadyQTail->bRunnable = 0;
                }
                break;
            }
            q = q->pNext;
        }
    }

    return 0; //for문이 다 끝나면 에러가 나오는게 맞음
}

int mymsgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
    while (1)
    {
        int i = msqid;
        if(	qcbTblEntry[msqid].key == -1)//메세지큐가 삭제된 경우
        {
            return -1;
        }
        //printf("test\n");
        Message *p = qcbTblEntry[msqid].pQcb->pMsgHead;
        for (; p; p = p->pNext)
        {

            if (msgtyp == p->type) //메세지 큐가 있는지 확인
            {
                for (int index = 0; index < msgsz; index++)
                {
                    ((Message *)msgp)->data[index] = p->data[index];
                }
                ((Message *)msgp)->type = p->type;
                //있다면 메세지큐 삭제
                if (NULL == p->pNext && (NULL == qcbTblEntry[i].pQcb->pMsgHead->pNext && NULL == qcbTblEntry[i].pQcb->pMsgTail->pNext)) /* only one element in queue */
                {
                    qcbTblEntry[i].pQcb->pMsgHead = qcbTblEntry[i].pQcb->pMsgTail = NULL;
                }
                else if ((NULL == p->pNext) && p->pPrev) /* removing pTail */
                {
                    qcbTblEntry[i].pQcb->pMsgTail = p->pPrev;
                    p->pPrev->pNext = NULL;
                }
                else if (p->pNext && (NULL == p->pPrev)) /* removing qcbTblEntry[i].pQcb->pMsgHead */
                {
                    qcbTblEntry[i].pQcb->pMsgHead = p->pNext;
                    qcbTblEntry[i].pQcb->pMsgHead->pPrev = NULL;
                }
                else /* removing from center or somewhere */
                {
                    p->pPrev->pNext = p->pNext;
                    p->pNext->pPrev = p->pPrev;
                }
                free(p); //필요?

                return strlen(((Message *)msgp)->data); //이렇게 되면 정상적으로 삭제가 되네
            }
        }

        //for문 끝나고 이제 waitingqueue 구현
        Thread *thread_p = Running_Thread;
        thread_p->type = msgtyp;

        if (NULL == qcbTblEntry[i].pQcb->pThreadHead && NULL == qcbTblEntry[i].pQcb->pThreadTail)
        {

            Running_Thread = NULL;

            qcbTblEntry[i].pQcb->pThreadHead = qcbTblEntry[i].pQcb->pThreadTail = thread_p;

            thread_p->pNext = thread_p->pPrev = NULL;
            qcbTblEntry[i].pQcb->pThreadHead->status = THREAD_STATUS_BLOCKED;
            qcbTblEntry[i].pQcb->pThreadHead->bRunnable = 0;
        }
        else if (NULL == qcbTblEntry[i].pQcb->pThreadHead || NULL == qcbTblEntry[i].pQcb->pThreadTail)
        {
            return -1;
        }
        else
        {

            Running_Thread = NULL;

            qcbTblEntry[i].pQcb->pThreadTail->pNext = thread_p;
            thread_p->pPrev = qcbTblEntry[i].pQcb->pThreadTail;
            qcbTblEntry[i].pQcb->pThreadTail = thread_p;
            qcbTblEntry[i].pQcb->pThreadTail->pNext = NULL;
            qcbTblEntry[i].pQcb->pThreadTail->status = THREAD_STATUS_BLOCKED;
            qcbTblEntry[i].pQcb->pThreadTail->bRunnable = 0;
        }

        thread_p->bRunnable == FALSE; //잠재워 지는 부분
        pthread_mutex_lock(&(thread_p->readyMutex));
        while (thread_p->bRunnable == FALSE)
            pthread_cond_wait(&(thread_p->readyCond), &(thread_p->readyMutex));
        pthread_mutex_unlock(&(thread_p->readyMutex));
    }

    return -1; //실패의 경우
}

int mymsgctl(int msqid, int cmd, void *buf)
{
    if (qcbTblEntry[msqid].pQcb->pMsgHead != NULL)
        return -1;
    if (qcbTblEntry[msqid].pQcb->pThreadHead != NULL)
        return -1;
    // for (int i = 0; i < MAX_QCB_SIZE; i++)
    // {
    // 	if (qcbTblEntry[i].key == -1)
    // 		continue;

    // 	if (qcbTblEntry[i].pQcb->pThreadHead != NULL)
    // 		return -1;
    // }

    //Message *p_massage = qcbTblEntry[msqid].pQcb->pMsgHead;
    // Thread *p_thread = qcbTblEntry[msqid].pQcb->pThreadHead;
    qcbTblEntry[msqid].key = -1;

    // while(p_massage)
    // {
    // 		Message *q_massage = p_massage;
    // 	p_massage=p_massage->pNext;
    // 	free(q_massage);
    // 	q_massage=NULL;
    // }
    // while (p_thread)
    // {
    // 	Thread *q_thread = p_thread;
    // 	p_thread = p_thread->pNext;
    // 	free(q_thread);
    // 	q_thread = NULL;
    // }
    free(qcbTblEntry[msqid].pQcb);
    qcbTblEntry[msqid].pQcb = NULL;

    //printf("mymsgctl msqid : (%d)\n", msqid);
    return 0;
}