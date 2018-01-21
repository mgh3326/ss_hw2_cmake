#include "Thread.h"
#include "Init.h"
#include "Scheduler.h"
#include "MsgQueue.h"
#include "my.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
int d = 0;
#define TRUE 1
#define FALSE 0
static pthread_cond_t bcond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t bmutex = PTHREAD_MUTEX_INITIALIZER;

void *__wrapperFunc(void *arg)
{
    //printf("        test %d\n",(int)arg);
    //printf("ReadQ insert tid : %u\n",(unsigned int)thread_self());

    // child sleeps until TCB is initialized

    // child is ready to run
    Ready_enqueue(thread_self()); //arg를 나중에 받도록 해야겠다. (arg 입력용 함수 구현 필요)
    __thread_wait_handler(0);	 //다시 재움
    WrapperArg *pArg = (WrapperArg *)arg;

    void *funcPtr = pArg->funcPtr;
    void *funcArg = pArg->funcArg;
    //if((pArg->funcArg)!=NULL)
    //printf("test : %d\n",*(int*)pArg->funcArg);
    void *ret;

    ret = (*pArg->funcPtr)(funcArg);

    return ret;
}

int thread_create(thread_t *thread, thread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    sign = 1;

    pthread_mutex_lock(&run_lock);

    WrapperArg *wrapperArg = (WrapperArg *)malloc(sizeof(WrapperArg));

    //if(arg!=NULL)
    //printf("test : %d\n",*(int*)arg);

    wrapperArg->funcPtr = start_routine;
    wrapperArg->funcArg = arg;
    sigset_t set;
    int retSig;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    //sigwait(&set, &retSig);
    signal(SIGUSR1, __thread_wait_handler); //핸들러 등록

    pthread_create(thread, attr, __wrapperFunc, wrapperArg);
    while (getThread(*thread) == NULL)
    {
        usleep(1);
    }
    //printf("스레드 하나 생성\n");
    getThread(*thread)->parentTid = thread_self();
    getThread(*thread)->pExitCode = arg;

    sign = 0;
    pthread_cond_signal(&run_wait);

    pthread_mutex_unlock(&run_lock);
}

int thread_join(thread_t thread, void **retval) //이게 머지
{
    sleep(1);
    int ret;
    sign = 1;

    pthread_mutex_lock(&run_lock);
    if (getThread(thread) == NULL)
    {
        Thread *cp = getThread_wait(thread);

        (*retval) = cp->pExitCode;
        Wait_delete_element(cp);
    }
    else
    {

        Thread *cp = getThread(thread);
        (*retval) = cp->pExitCode;

        Thread *p = ReadyQHead;
        sign = 0;

        pthread_cond_signal(&run_wait);

        pthread_mutex_unlock(&run_lock);
        thread_suspend(p->tid);

        WaitQHead->status = 2;
        p->status = 2;

        WaitQHead->bRunnable = 0;

        Thread *pTh;
        if (getThread_wait(thread_self()) == NULL)
        {
            pTh = cp;
        }
        pTh = getThread_wait(thread_self()); // child에서 TCB가 초기화 안되었는데, 이 함수가 호출되어도 되나 ?
        pTh->bRunnable == FALSE;

        pthread_mutex_lock(&(pTh->readyMutex));
        //

        while (pTh->bRunnable == FALSE)
            pthread_cond_wait(&(pTh->readyCond), &(pTh->readyMutex));

        pthread_mutex_unlock(&(pTh->readyMutex));

        thread_resume(p->tid);

        Ready_delete_element(p); //제거
    }

    return ret;
}
int thread_exit(void *retval)
{
    sign = 1;
    pthread_mutex_lock(&run_lock);

    Thread *cp = getThread(thread_self());
    Thread *pp = getThread_wait(cp->parentTid);
    sign = 0;
    pthread_cond_signal(&run_wait);
    pthread_mutex_unlock(&run_lock);
    thread_suspend(thread_self());
    sign = 1;
    pthread_mutex_lock(&run_lock);
    cp->status = THREAD_STATUS_ZOMBIE;

    cp->pExitCode = retval;

    if (NULL != getThread_wait(cp->parentTid))
    {

        __thread_wakeup(pp);
    }

    sign = 0;
    pthread_cond_signal(&run_wait);

    pthread_mutex_unlock(&run_lock);

    return 0;
}

int thread_suspend(thread_t tid)
{
    sign = 1;

    pthread_mutex_lock(&run_lock);

    int ret;
    Thread *p = getThread(tid);

    if (NULL == WaitQHead && NULL == WaitQTail)
    {
        Ready_remove_element(getThread(tid));
        WaitQHead = WaitQTail = p;
        p->pNext = p->pPrev = NULL;
        WaitQHead->status = THREAD_STATUS_BLOCKED;
        WaitQHead->bRunnable = 0;
    }
    else if (NULL == WaitQHead || NULL == WaitQTail)
    {
        fprintf(stderr, "IN: %s @%d: Serious error.", __FILE__, __LINE__);
        fprintf(stderr, "List one of the list's ReadyQHead/pTail is null while other is not\n");
        ret = -1;
    }
    else
    {

        Ready_remove_element(getThread(tid));

        WaitQTail->pNext = p;
        p->pPrev = WaitQTail;
        WaitQTail = p;
        WaitQTail->pNext = NULL;
        WaitQTail->status = THREAD_STATUS_BLOCKED;
        WaitQTail->bRunnable = 0;
        ret = 0;
    }
    sign = 0;
    pthread_cond_signal(&run_wait);

    pthread_mutex_unlock(&run_lock);
}

int thread_resume(thread_t tid)
{
    sign = 1;
    pthread_mutex_lock(&run_lock);

    int ret;
    Thread *p = getThread_wait(tid);
    if (NULL == ReadyQHead && NULL == ReadyQTail)
    {
        Wait_remove_element(getThread_wait(tid));
        ReadyQHead = ReadyQTail = p;
        p->pNext = p->pPrev = NULL;

        ReadyQHead->status = THREAD_STATUS_READY;
        ReadyQHead->bRunnable = 0;
    }
    else if (NULL == ReadyQHead || NULL == ReadyQTail)
    {
        fprintf(stderr, "IN: %s @%d: Serious error.", __FILE__, __LINE__);
        fprintf(stderr, "List one of the list's WaitQHead/pTail is null while other is not\n");
        ret = -1;
    }
    else

    {
        Wait_remove_element(getThread_wait(tid));

        ReadyQTail->pNext = p;
        p->pPrev = ReadyQTail;
        ReadyQTail = p;
        ReadyQTail->pNext = NULL;

        ReadyQTail->status = THREAD_STATUS_READY;
        ReadyQTail->bRunnable = 0;
        ret = 0;
    }
    sign = 0;
    pthread_cond_signal(&run_wait);

    pthread_mutex_unlock(&run_lock);
}

thread_t thread_self()
{
    return pthread_self();
}

//추가함`
void __thread_wait_handler(int signo)
{

    // Ready_print_queue();
    //     Wait_print_queue();

    Thread *pTh;
    pTh->bRunnable == FALSE;
    // __getThread()는 tid로 linked list의 TCB를 찾아서 반환한다.
    pTh = getThread(pthread_self()); // child에서 TCB가 초기화 안되었는데, 이 함수가 호출되어도 되나 ?
    // printf(" wait tid %u\n",pTh->tid);
    if (pTh == NULL)
        pTh = Running_Thread;
    pthread_mutex_lock(&(pTh->readyMutex));
    //         printf("__thread_wait_handler start %u\n",pTh->tid);

    while (pTh->bRunnable == FALSE)
        pthread_cond_wait(&(pTh->readyCond), &(pTh->readyMutex));
    pthread_mutex_unlock(&(pTh->readyMutex));
    //printf("__thread_wait_handler end %u\n",pTh->tid);
}
void thread_wait(thread_t tid)
{
    pthread_kill(tid, SIGUSR1);
    // printf("\nthread_wait\n");
}
void __thread_wakeup(Thread *pTh)
{
    pthread_mutex_lock(&(pTh->readyMutex));

    pTh->bRunnable = TRUE;
    //printf("__thread_wakeup start%u\n",pTh->tid);

    pthread_cond_signal(&(pTh->readyCond));
    pthread_mutex_unlock(&(pTh->readyMutex));
    //printf("__thread_wakeup end%u\n",pTh->tid);

    //printf("\n__thread_wakeup\n");
}

int Ready_enqueue(thread_t i)
{
    int ret;

    if (NULL == ReadyQHead && NULL == ReadyQTail)
    {
        struct _Thread *p = malloc(1 * sizeof *p);
        if (NULL == p)
        {
            fprintf(stderr, "IN: %s @%d: Out of Memory\n", __FILE__, __LINE__);
            ret = -1;
        }
        else
        {
            p->status = THREAD_STATUS_READY; //status
            p->tid = i;
            //p->parentTid = pthread_self();
            p->pPrev = p->pNext = NULL;
            // p->readyCond = bcond;
            // p->readyMutex = bmutex;
            ReadyQHead = ReadyQTail = p;
            ret = 0;
        }
    }
    else if (NULL == ReadyQHead || NULL == ReadyQTail)
    {
        fprintf(stderr, "IN: %s @%d: Serious error.", __FILE__, __LINE__);
        fprintf(stderr, "List one of the list's ReadyQHead/pTail is null while other is not\n");
        ret = -1;
    }
    else
    {
        struct _Thread *p = malloc(1 * sizeof *p);
        if (NULL == p)
        {
            fprintf(stderr, "IN: %s @%d: Out of Memory\n", __FILE__, __LINE__);
            ret = -1;
        }
        else
        {
            p->tid = i;
            //p->parentTid = pthread_self();

            p->status = THREAD_STATUS_READY; //status
            p->pPrev = p->pNext = NULL;
            // p->readyCond = bcond;
            // p->readyMutex = bmutex;
            ReadyQTail->pNext = p;
            p->pPrev = ReadyQTail;
            ReadyQTail = p;
            ret = 0;
        }
    }

    return ret;
}

int Ready_dequeue()
{
    int ret;

    if (NULL == ReadyQHead && NULL == ReadyQTail)
    {
        printf("Nothing to Dequeue()\n");
        ret = 0;
    }
    else if (NULL == ReadyQHead || NULL == ReadyQTail)
    {
        fprintf(stderr, "IN: %s @%d: Serious error.", __FILE__, __LINE__);
        fprintf(stderr, "List one of the list's ReadyQHead/pTail is null while other is not\n");
        ret = -1;
    }
    else
    {
        struct _Thread *p = ReadyQHead;
        if (NULL == ReadyQHead->pNext && NULL == ReadyQTail->pNext) /* if last element */
        {
            ReadyQHead = ReadyQTail = NULL;
        }
        else
        {
            //ReadyQHead->bRunnable=THREAD_STATUS_READY;
            // ReadyQTail->bRunnable=THREAD_STATUS_READY;

            ReadyQTail->pNext = ReadyQHead;
            ReadyQHead->pPrev = ReadyQTail;
            ReadyQTail = ReadyQHead;
            // ReadyQTail->bRunnable=THREAD_STATUS_READY;

            ReadyQHead = ReadyQHead->pNext;
            ReadyQHead->pPrev = NULL;
            ReadyQTail->pNext = NULL;
            //ReadyQHead->pPrev=NULL;
            //ReadyQHead->pNext->pPrev=NULL;//형린이랑 추가함
        }

        //free(p);

        ret = 0;
    }

    return ret;
}

Thread *getThread(thread_t i)
{
    if (NULL == ReadyQHead && NULL == ReadyQTail)
    {
        //printf("Nothing to\n");
        return NULL;
    }
    else if (NULL == ReadyQHead || NULL == ReadyQTail)
    {
        fprintf(stderr, "IN: %s @%d: Serious error.", __FILE__, __LINE__);
        fprintf(stderr, "List one of the list's ReadyQHead/pTail is null while other is not\n");
    }
    else
    {
        struct _Thread *p = ReadyQHead;
        //printf("ReadyQoutput\n");
        for (; p; p = p->pNext)
        {
            if (i == p->tid)
            {

                return p;
            }
        }
        return NULL; //실패?
    }
}
Thread *getThread_wait(thread_t i)
{
    if (NULL == WaitQHead && NULL == WaitQTail)
    {
        return NULL; //실패?
        printf("Nothing to\n");
    }
    else if (NULL == WaitQHead || NULL == WaitQTail)
    {
        fprintf(stderr, "IN: %s @%d: Serious error.", __FILE__, __LINE__);
        fprintf(stderr, "List one of the list's ReadyQHead/pTail is null while other is not\n");
    }
    else
    {
        struct _Thread *p = WaitQHead;
        //printf("ReadyQoutput\n");
        for (; p; p = p->pNext)
        {
            if (i == p->tid)
            {
                return p;
            }
        }
        return NULL; //실패?
    }
}
void Ready_remove_element(struct _Thread *d)
{

    if (NULL == d->pNext && (NULL == ReadyQHead->pNext && NULL == ReadyQTail->pNext)) /* only one element in queue */
    {

        ReadyQHead = ReadyQTail = NULL;
    }
    else if ((NULL == d->pNext) && d->pPrev) /* removing pTail */
    {
        ReadyQTail = d->pPrev;
        d->pPrev->pNext = NULL;
    }
    else if (d->pNext && (NULL == d->pPrev)) /* removing ReadyQHead */
    {
        ReadyQHead = d->pNext;
        ReadyQHead->pPrev = NULL;
        ReadyQHead->bRunnable = 1;
    }
    else /* removing from center or somewhere */
    {

        d->pPrev->pNext = d->pNext;
        d->pNext->pPrev = d->pPrev;
    }

    //free(d);
}
void Wait_remove_element(struct _Thread *d)
{
    if (NULL == d->pNext && (NULL == WaitQHead->pNext && NULL == WaitQTail->pNext)) /* only one element in queue */
    {
        WaitQHead = WaitQTail = NULL;
    }
    else if ((NULL == d->pNext) && d->pPrev) /* removing pTail */
    {
        WaitQTail = d->pPrev;
        d->pPrev->pNext = NULL;
    }
    else if (d->pNext && (NULL == d->pPrev)) /* removing WaitQHead */
    {
        WaitQHead = d->pNext;
        WaitQHead->pPrev = NULL;
    }
    else /* removing from center or somewhere */
    {
        d->pPrev->pNext = d->pNext;
        d->pNext->pPrev = d->pPrev;
    }

    //free(d);
}
void Wait_delete_element(struct _Thread *d)
{
    if (NULL == d->pNext && (NULL == WaitQHead->pNext && NULL == WaitQTail->pNext)) /* only one element in queue */
    {
        WaitQHead = WaitQTail = NULL;
    }
    else if ((NULL == d->pNext) && d->pPrev) /* removing pTail */
    {
        WaitQTail = d->pPrev;
        d->pPrev->pNext = NULL;
    }
    else if (d->pNext && (NULL == d->pPrev)) /* removing WaitQHead */
    {
        WaitQHead = d->pNext;
        WaitQHead->pPrev = NULL;
    }
    else /* removing from center or somewhere */
    {
        d->pPrev->pNext = d->pNext;
        d->pNext->pPrev = d->pPrev;
    }

    free(d);
}
void Ready_delete_element(struct _Thread *d)
{

    if (NULL == d->pNext && (NULL == ReadyQHead->pNext && NULL == ReadyQTail->pNext)) /* only one element in queue */
    {

        ReadyQHead = ReadyQTail = NULL;
    }
    else if ((NULL == d->pNext) && d->pPrev) /* removing pTail */
    {
        ReadyQTail = d->pPrev;
        d->pPrev->pNext = NULL;
    }
    else if (d->pNext && (NULL == d->pPrev)) /* removing ReadyQHead */
    {
        ReadyQHead = d->pNext;
        ReadyQHead->pPrev = NULL;
        ReadyQHead->bRunnable = 1;
    }
    else /* removing from center or somewhere */
    {

        d->pPrev->pNext = d->pNext;
        d->pNext->pPrev = d->pPrev;
    }

    free(d);
}
Thread *Ready_peek()
{
    Thread *temp;

    if (ReadyQHead == NULL)
        return NULL;
    else if (ReadyQHead->pNext == NULL)
    {
        temp = ReadyQHead;
        ReadyQHead = NULL;
        ReadyQTail = NULL;
    }
    else
    {
        temp = ReadyQHead;
        ReadyQHead = ReadyQHead->pNext;
        ReadyQHead->pPrev = NULL;
        temp->pNext = NULL;
    }

    return temp;
}
Thread *Ready_pop()
{
    Thread *temp;

    if (ReadyQHead == NULL)
        return NULL;
    else if (ReadyQHead->pNext == NULL)
    {
        temp = ReadyQHead;
        ReadyQHead = NULL;
        ReadyQTail = NULL;
    }
    else
    {
        temp = ReadyQHead;
        ReadyQHead = ReadyQHead->pNext;
        ReadyQHead->pPrev = NULL;
        temp->pNext = NULL;
    }

    return temp;
}
thread_t thread_head()
{
    return ReadyQHead->tid;
}
void print_queue()
{
    printf("-------\n");

    printf("Running tid = (%lu) run = (%d) type = (%ld) status = (%d)\n", Running_Thread->tid, Running_Thread->bRunnable, Running_Thread->type, Running_Thread->status);
    Thread *p = ReadyQHead;
    int i = 0;
    while (p)
    {

        printf("Ready(%d)tid = (%lu) run = (%d) type = (%ld) status = (%d)\n", i, p->tid, p->bRunnable, p->type, p->status);
        i++;
        p = p->pNext;
    }
    if (qcbTblEntry[0].pQcb != NULL)
    {
        //printf("Test\n");
        Thread *q = qcbTblEntry[0].pQcb->pThreadHead;
        i = 0;
        printf("-------\n");
        while (q)
        {

            printf("Wait[0](%d)tid = (%lu) run = (%d) type = (%ld) status = (%d)\n", i, q->tid, q->bRunnable, q->type, q->status);
            i++;
            q = q->pNext;
        }
        Message *r = qcbTblEntry[0].pQcb->pMsgHead;
        i = 0;
        printf("______\n");
        while (r)
        {

            printf("Message(%d)type = (%ld)\n", i, r->type);
            i++;
            r = r->pNext;
        }
    }
}