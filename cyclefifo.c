#include "cyclefifo.h"

static int qSize = 0;

// 初始化队列
int cfqInit(Cfqueue *ptr, int size)
{
    qSize = size;
    ptr->data = malloc(qSize);
    if (ptr->data == NULL)
    {
        return -1;
    }
    memset(ptr->data, 0, sizeof(ptr->data));
    ptr->front = 0;
    ptr->rear = -1;
    ptr->count = 0;
    pthread_mutex_init(&ptr->lock, NULL);
    return 0;
}

// 检查队列是否已满
int cfqIsFull(Cfqueue *ptr)
{
    return ptr->count >= qSize;
}

// 检查队列是否为空
int cfqIsEmpty(Cfqueue *ptr)
{
    return ptr->count <= 0;
}

// 入队
int cfqPush(Cfqueue *ptr, void *item)
{
    pthread_mutex_lock(&ptr->lock);
    if (cfqIsFull(ptr))
    {
        pthread_mutex_unlock(&ptr->lock);
        return -1; // ERR_CYCLEFIFOQUEUE_ISFULL;
    }
    else
    {
        ptr->count++;
        ptr->rear = (ptr->rear + 1) % qSize;
        ptr->data[ptr->rear] = item;
    }
    pthread_mutex_unlock(&ptr->lock);
    return 0;
}

// 出队
void *cfqPop(Cfqueue *ptr)
{
    void *ret = NULL;
    pthread_mutex_lock(&ptr->lock);
    if (cfqIsEmpty(ptr))
    {
        pthread_mutex_unlock(&ptr->lock);
        return NULL;
    }
    else
    {
        ret = ptr->data[ptr->front];
        ptr->count--;
        ptr->front = (ptr->front + 1) % qSize;
    }
    pthread_mutex_unlock(&ptr->lock);
    return ret;
}

// 返回队列长度
int cfqGetQlength(Cfqueue *ptr)
{
    return ptr->count;
}

// 清空队列
void cfqClean(Cfqueue *ptr)
{
    pthread_mutex_lock(&ptr->lock);
    if (cfqIsEmpty(ptr))
    {
        pthread_mutex_unlock(&ptr->lock);
        return;
    }
    else
    {
        memset(ptr->data, 0, sizeof(ptr->data));
        ptr->front = 0;
        ptr->rear = -1;
        ptr->count = 0;
    }
    pthread_mutex_unlock(&ptr->lock);
    return;
}

// 销毁
void cfqDestory(Cfqueue *ptr)
{
    pthread_mutex_lock(&ptr->lock);
    if (ptr->data)
    {
        free(ptr->data);
        ptr->data = NULL;
        ptr->rear = ptr->front = 0;
    }
    pthread_mutex_unlock(&ptr->lock);
    pthread_mutex_destroy(&ptr->lock);
    return;
}
