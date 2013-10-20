#include "CReqQueue.h"

CReqQueue::CReqQueue(int queueSize):
    sizeQueue(queueSize),lput(0),lget(0),nFullThread(0),nEmptyThread(0),nData(0)
{
    pthread_mutex_init(&m_mutex,0);
    pthread_cond_init(&condGet,0);
    pthread_cond_init(&condPut,0);
    buffer=new int[sizeQueue];
}

CReqQueue::~CReqQueue()
{
    delete[] buffer;
}


int CReqQueue::getq()
{
    int data;
    pthread_mutex_lock(&m_mutex);
	//此处循环判断的原因如下：
	//假设2个线程在getq阻塞，然后两者都被激活，而其中一个线程运行比较块，
	//快速消耗了2个数据，另一个线程醒来的时候已经没有新数据可以消耗了。
	//另一点，man pthread_cond_wait可以看到，该函数可以被信号中断返回，此时返回EINTR。
	//为避免以上任何一点，都必须醒来后再次判断睡眠条件。
	//更正：pthread_cond_wait是信号安全的系统调用，不会被信号中断。
    while(lget==lput&&nData==0)
    {
        nEmptyThread++;
        pthread_cond_wait(&condGet,&m_mutex);
        nEmptyThread--;
    }

    data=buffer[lget++];
    nData--;
    if(lget==sizeQueue)
    {
        lget=0;
    }
    if(nFullThread) //必要时才进行signal操作，勿总是signal
    {
        pthread_cond_signal(&condPut);
    }
    pthread_mutex_unlock(&m_mutex);
    return data;
}

void CReqQueue::putq(int data)
{
    pthread_mutex_lock(&m_mutex);
    while(lput==lget&&nData)
    {
        nFullThread++;
        pthread_cond_wait(&condPut,&m_mutex);
        nFullThread--;
    }
    buffer[lput++]=data;
    nData++;
    if(lput==sizeQueue)
    {
        lput=0;
    }
    if(nEmptyThread)
    {
        pthread_cond_signal(&condGet);
    }
    pthread_mutex_unlock(&m_mutex);
}

int CReqQueue::GetQueueSize()
{
    return sizeQueue;
}


