/*
 cyclefifo.h
 Function: cycle FIFO queue, can be used on multiple threads mode.
 Author: suwei
 Version: 0.0.1
 Usage:
    1) Initialize a cycle fifo queue
        eg:
            Cfqueue myQueue;
            cfqInit(&myQueue, 5);	//maximus size of queue is 5
    2) Declare an object, which you want to push into the queue
        eg:
            CString strA = "element 0"
    3) In some threads, push the object into queue
        eg:
            cfqPush(&myQueue, (char *) &strA);
    4) ... (do others)
    5) In some other threads, get the object from the queue
        eg:
            CString strB = (CString *) cfqPop(&myQueue);
    7) ... (do others)
    8) destory the queue (must do this)
        eg:
            cfqDestory(myQueue);
*/

#ifndef CYCLEFIFOQUEUE_H
#define CYCLEFIFOQUEUE_H

#include <pthread.h>
#include <string.h>

#include <malloc.h>

typedef struct cycle_fifo_queue
{
    unsigned int front;   // 队首
    unsigned int rear;    // 队尾
    unsigned int count;   // 元素个数
    pthread_mutex_t lock; // 多线程锁
    void **data;          // 元素存储区
} Cfqueue;

// 队列初始化
int cfqInit(Cfqueue *ptr, int size);
// 队列是否满载，TRUE:满，FALSE:未满
int cfqIsFull(Cfqueue *ptr);
// 队列是否为空，TRUE:空，FALSE:非空
int cfqIsEmpty(Cfqueue *ptr);
// 元素入队
int cfqPush(Cfqueue *ptr, void *item);
// 元素出队
void *cfqPop(Cfqueue *ptr);
// 获取队列长度
int cfqGetQlength(Cfqueue *ptr);
// 销毁队列
void cfqDestory(Cfqueue *ptr);
// 清空队列
void cfqClean(Cfqueue *ptr);

#endif
