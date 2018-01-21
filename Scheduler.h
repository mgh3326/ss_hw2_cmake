//
// Created by moon on 1/21/18.
//

#ifndef UNTITLED5_SCHEDULER_H
#define UNTITLED5_SCHEDULER_H
#include "Thread.h"
#include "my.h"
int RunScheduler(void);
void __ContextSwitch(Thread *pCurThread, Thread *pNewThread);

#endif //UNTITLED5_SCHEDULER_H
