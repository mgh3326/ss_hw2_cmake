//
// Created by moon on 1/21/18.
//

#ifndef UNTITLED5_MY_H
#define UNTITLED5_MY_H
#include "Thread.h"
#include <stdlib.h>

#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
pthread_cond_t phylo_wait;
pthread_cond_t pyDE_wait;
int sign; ////avail=0;phylo=1;pyDE=2
pthread_mutex_t room_lock;
int ohoh;
pthread_mutex_t m;
int done;
Thread *Running_Thread;
void *__wrapperFunc(void *arg);
//여기 부터 추가
void thread_wait(thread_t tid);
int Ready_enqueue(pthread_t i);
int Ready_dequeue();
void Ready_print_queue();
int Wait_enqueue(pthread_t i);
int Wait_dequeue();
void Wait_print_queue();
Thread *getThread(thread_t i);
void Ready_remove_element(struct _Thread *d);
Thread *getThread_wait(thread_t i);
void Wait_remove_element(struct _Thread *d);
void Wait_delete_element(struct _Thread *d);
void Ready_delete_element(struct _Thread *d);
void __thread_wait_handler(int signo);
void __thread_wakeup(Thread *pTh);
Thread *Ready_peek();
Thread *Ready_pop();

thread_t thread_head();
void print_queue();
//여기 까지 추가
extern int count;
int Ready_insert(thread_t i, void *arg);
pthread_cond_t run_wait;
int sign; //avail=0;phylo=1;pyDE=2
pthread_mutex_t run_lock;
#endif //UNTITLED5_MY_H
