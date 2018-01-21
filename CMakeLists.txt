cmake_minimum_required(VERSION 3.8)
project(untitled5)
find_package (Threads)
set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES Init.c Init.h Init.c Init.h MsgQueue.c MsgQueue.h my.h Scheduler.c Scheduler.h Thread.c Thread.h TestCase1.c TestCase1.h TestCase2.c TestCase2.h TestCase3.c TestCase3.h TestCase4.c TestCase4.h TestMain.c)
add_executable(untitled5 ${SOURCE_FILES})
target_link_libraries (untitled5 ${CMAKE_THREAD_LIBS_INIT})