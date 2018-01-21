#include "Init.h"
#include "Thread.h"
#include "my.h"
#include "Scheduler.h"
#include <unistd.h>
#include <stdio.h>
#include "MsgQueue.h"
static pthread_cond_t bcond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t bmutex = PTHREAD_MUTEX_INITIALIZER;
int RunScheduler(void)
{
    while (1)
    {
        pthread_mutex_lock(&run_lock);
        while (sign != 0)
            pthread_cond_wait(&run_wait, &run_lock);

        if (ReadyQHead == NULL) //레디큐가 비었을 경우
        {
            sleep(TIMESLICE);
        }
        else
        {

            if (Running_Thread == NULL) //러닝 스레드가 비어있는 경우
            {
                Running_Thread = Ready_pop();
                __thread_wakeup(Running_Thread);
                sleep(TIMESLICE);
            }
            else
            {
                __ContextSwitch(Running_Thread, Ready_pop());
                sleep(TIMESLICE);
            }
        }
        pthread_mutex_unlock(&run_lock);
    }
}
void __ContextSwitch(Thread *pCurThread, Thread *pNewThread)
{

    Running_Thread->bRunnable = 0;
    if (ReadyQHead == NULL) //레디큐팝 한 이후에 레디큐가 빈 경우
    {
        ReadyQHead = Running_Thread;
    }
    else
    {
        ReadyQTail->pNext = Running_Thread;
        Running_Thread->pPrev = ReadyQTail;
    }
    ReadyQTail = Running_Thread;
    pthread_kill(Running_Thread->tid, SIGUSR1);
    Running_Thread = pNewThread;
    __thread_wakeup(Running_Thread);
}