#include "Init.h"
#include "MsgQueue.h"
#include "my.h"
void Init()
{
    Running_Thread = NULL; //러닝 스레드 초기화
    _InitMsgQueue();
    // ohoh=0;
}
